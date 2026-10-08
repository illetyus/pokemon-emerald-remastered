from __future__ import annotations

import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

from tools.build_r7_environment_package import ROOT, build_package, digest, encoded, family_hint, package_documents, resolve, strict_json
from tools.validate_r7_local_assets import obj_geometry, png_dimension, validate_assets

SOURCE = ROOT / "vendor/vanillaplus"


class EnvironmentPackageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.docs = package_documents(SOURCE)

    def test_full_pinned_world_coverage(self):
        audit = self.docs["map-audit.json"]
        self.assertEqual((audit["map_count"], audit["layout_count"]), (518, 441))
        self.assertEqual(audit["unresolved_identity_count"], 0)
        self.assertTrue(all(not item["unresolved_identities"] for item in audit["maps"]))
        self.assertTrue(all(not item["descriptor_fallbacks"] for item in audit["maps"]))

    def test_source_holes_are_explicit_unused_layout_fallbacks(self):
        manifest = self.docs["manifest.json"]
        self.assertEqual(manifest["source_descriptor_identity_count"], 18318)
        self.assertEqual(manifest["descriptor_fallback_identity_count"], 85)
        self.assertEqual(self.docs["map-audit.json"]["descriptor_fallback_layouts"], ["LAYOUT_UNUSED_OUTDOOR_AREA"])
        gaps = [item for item in manifest["identities"] if item["model_status"] == "descriptor_missing"]
        self.assertEqual(len(gaps), 85)
        self.assertTrue(all(item["fallback"] == "visible_engine_plane" for item in gaps))

    def test_all_identities_have_unique_explicit_fallback(self):
        manifest = self.docs["manifest.json"]
        identities = manifest["identities"]
        self.assertEqual(manifest["identity_count"], 18403)
        self.assertEqual(len({item["identity"] for item in identities}), len(identities))
        self.assertTrue(all(item["fallback"] in {"r5_metatile", "visible_engine_plane"} for item in identities))
        self.assertEqual(manifest["verified_3d_models"], 0)
        self.assertEqual(manifest["fallback_identity_count"], len(identities))

    def test_used_identity_registry_and_no_source_substitutions(self):
        known = {item["identity"] for item in self.docs["manifest.json"]["identities"]}
        for layout in self.docs["map-audit.json"]["layouts"]:
            self.assertTrue(set(layout["identity_counts"]) <= known)
            self.assertEqual(sum(layout["identity_counts"].values()), layout["tile_count"])

    def test_representative_contexts_resolve(self):
        maps = self.docs["map-audit.json"]["representative_maps"]
        self.assertEqual(len(maps), 6)
        self.assertTrue({"MAP_TYPE_INDOOR", "MAP_TYPE_TOWN", "MAP_TYPE_ROUTE", "MAP_TYPE_UNDERGROUND", "MAP_TYPE_UNDERWATER"} <= {item["map_type"] for item in maps})
        by_identity = {item["identity"]: item for item in self.docs["manifest.json"]["identities"]}
        water_map = next(item for item in maps if item["name"] == "Route105")
        layout = next(item for item in self.docs["map-audit.json"]["layouts"] if item["id"] == water_map["layout"])
        self.assertTrue(any(by_identity[key]["family_hint"] == "water" for key in layout["identity_counts"]))

    def test_signs_are_source_landmarks(self):
        maps = {item["name"]: item for item in self.docs["map-audit.json"]["maps"]}
        self.assertEqual(maps["LittlerootTown"]["sign_landmarks"], [
            {"x": 15, "y": 13, "source_type": "sign"},
            {"x": 6, "y": 17, "source_type": "sign"},
            {"x": 7, "y": 8, "source_type": "sign"},
            {"x": 12, "y": 8, "source_type": "sign"}])

    def test_behavior_does_not_guess_building_geometry(self):
        self.assertEqual(family_hint([], "MB_NORMAL"), "unclassified")
        self.assertEqual(family_hint([], "MB_IMPASSABLE_NORTH"), "unclassified")
        self.assertEqual(family_hint(["METATILE_General_RockWall_GrassBase"], "MB_NORMAL"), "rocks")

    def test_primary_secondary_identity_boundary_and_strict_holes(self):
        tile = {"local_metatile_id": 0, "entries": [{}] * 8, "render_planes": ["bottom", "top"]}
        tilesets = {"primary": {"metatiles": [tile]}, "secondary": {"metatiles": [tile]}}
        self.assertEqual(resolve(tilesets, "primary", "secondary", 0), ("primary", 0))
        self.assertEqual(resolve(tilesets, "primary", "secondary", 512), ("secondary", 0))
        for value in (True, -1, 1024, 0.5):
            with self.assertRaises(ValueError):
                resolve(tilesets, "primary", "secondary", value)
        with self.assertRaises(ValueError):
            resolve(tilesets, "primary", None, 512)

    def test_invalid_r5_quadrants_fail_closed(self):
        tilesets = {"primary": {"metatiles": [{"local_metatile_id": 0, "entries": [], "render_planes": []}]}}
        with self.assertRaises(ValueError):
            resolve(tilesets, "primary", None, 0)

    def test_deterministic_package_and_integrity(self):
        with tempfile.TemporaryDirectory() as temp:
            a, b = Path(temp) / "a", Path(temp) / "b"
            build_package(SOURCE, a)
            build_package(SOURCE, b)
            self.assertEqual({p.name: p.read_bytes() for p in a.iterdir()}, {p.name: p.read_bytes() for p in b.iterdir()})
            build_package(SOURCE, a, verify=True)
            (a / "contract.json").write_text("{}")
            with self.assertRaises(ValueError):
                build_package(SOURCE, a, verify=True)

    def test_strict_json_rejects_duplicate_and_nonfinite_values(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "input.json"
            for text in ('{"x":1,"x":2}', '{"x":NaN}', '{"x":Infinity}'):
                path.write_text(text)
                with self.assertRaises(ValueError):
                    strict_json(path)

    def test_contract_runtime_budgets_match_shared_policy(self):
        contract = self.docs["contract.json"]
        math = (ROOT / "unreal/Source/PokemonEmeraldRemastered/RemasterEnvironmentMath.h").read_text()
        for name, value in zip(("MaxLod0Triangles", "MaxLod1Triangles", "MaxLod2Triangles"), contract["mobile"]["lod_triangle_limits"]):
            self.assertIn(f"{name} = {value};", math)
        for name, key in (("MaxMaterials", "max_material_slots"), ("MaxTextureDimension", "max_texture_dimension"),
            ("MaxComponentsPerChunk", "max_3d_components_per_chunk"), ("MaxInstancesPerChunk", "max_3d_instances_per_chunk"),
            ("MaxTrianglesPerChunk", "max_3d_lod0_triangles_per_chunk")):
            self.assertIn(f"{name} = {contract['mobile'][key]};", math)

    def test_environment_authority_and_occlusion_boundary(self):
        module = ROOT / "unreal/Source/PokemonEmeraldRemastered"
        camera = (module / "RemasterCameraRig.cpp").read_text()
        controller = (module / "RemasterEnvironmentController.cpp").read_text()
        world = (module / "RemasterWorldActor.cpp").read_text()
        self.assertIn("Snapshot.PlayerX, Snapshot.PlayerY", camera)
        self.assertIn("LastMapRevision != Revision", camera)
        self.assertIn("Save->GetLocalRtcNow(Rtc)", controller)
        self.assertIn("RuntimeWeatherId = Snapshot.Weather", controller)
        self.assertNotIn("GetVelocity", camera)
        for text in (camera, controller, world):
            for forbidden in ("LineTrace", "RealignRtcNow", "remaster_emerald_overworld_set(", "SetActorEnableCollision(true)"):
                self.assertNotIn(forbidden, text)
        self.assertIn("Item.OriginalTransform", world)
        self.assertIn("SetCanEverAffectNavigation(false)", world)
        self.assertIn("AddMissingDescriptorFallback(Visual, X, Y, Chunk)", world)


class LocalAssetValidationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.identity = "gTileset_General:1"
        self.manifest = self.root / "manifest.json"
        self.manifest.write_bytes(encoded({"identities": [{"identity": self.identity, "model_status": "source_missing"}]}))
        self.source = self.write_file("source.bin", b"synthetic source fixture, no commercial content")
        self.provenance = self.write_file("provenance.json", encoded({"identity": self.identity,
            "source_sha256": self.source["sha256"], "source_url": "https://example.com/synthetic-fixture"}))
        self.geometry = self.write_file("mesh.obj", b"v 0 0 0\nv 10 0 0\nv 0 10 0\nf 1 2 3\n")
        self.record = {"identity": self.identity, "source": self.source, "provenance": self.provenance,
            "lods": [self.geometry] * 3, "textures": [], "units": "centimetres", "pivot": "tile-ground", "cull_distance": 4000}

    def write_file(self, name, data):
        (self.root / name).write_bytes(data)
        return {"path": name, "sha256": digest(data)}

    def validate(self, records):
        bindings = self.root / "bindings.json"
        bindings.write_bytes(encoded({"schema_version": 1, "models": records}))
        return validate_assets(bindings, self.manifest)

    def test_real_lod_geometry_hashes_preimport_only(self):
        result = self.validate([self.record])
        self.assertEqual(result["status"], "PREIMPORT_VERIFIED")
        model = result["models"][0]
        self.assertEqual(model["lod_triangles"], [1, 1, 1])
        self.assertFalse(model["import_validated"])
        self.assertEqual(model["engine_import_status"], "R18_PENDING")
        self.assertNotIn("path", json.dumps(result))

    def test_unknown_or_duplicate_identity(self):
        bad = {**self.record, "identity": "gTileset_Unknown:1"}
        for records in ([bad], [self.record, self.record]):
            with self.assertRaises(ValueError):
                self.validate(records)

    def test_missing_or_tampered_normalized_output(self):
        (self.root / "mesh.obj").write_text("tampered")
        with self.assertRaises(ValueError):
            self.validate([self.record])

    def test_provenance_must_match_source_and_identity(self):
        bad = copy.deepcopy(self.record)
        bad["provenance"] = self.write_file("bad-proof.json", encoded({"identity": "wrong",
            "source_sha256": self.source["sha256"], "source_url": "https://example.com"}))
        with self.assertRaises(ValueError):
            self.validate([bad])

    def test_three_lods_and_centimetre_pivot_required(self):
        for changes in ({"lods": [self.geometry]}, {"units": "metres"}, {"pivot": "centre"}):
            with self.assertRaises(ValueError):
                self.validate([{**self.record, **changes}])

    def test_budget_failures(self):
        huge = self.write_file("huge.obj", b"v 0 0 0\nv 1 0 0\nv 0 1 0\n" + b"f 1 2 3\n" * 6001)
        for changes in ({"lods": [huge, self.geometry, self.geometry]}, {"cull_distance": True}, {"cull_distance": 5000}):
            with self.assertRaises(ValueError):
                self.validate([{**self.record, **changes}])

    def test_invalid_geometry(self):
        for payload in ("v nan 0 0\n", "v 2000 0 0\n", "v 0 0 0\nf 1 2 3\n",
                        "v 0 0 0\nf 1 1 1\n", "v 0 0 0\nf 1 1 1 1\n"):
            (self.root / "bad.obj").write_text(payload)
            with self.assertRaises(ValueError):
                obj_geometry(self.root / "bad.obj")

    def test_texture_budget_and_crc(self):
        def chunk(kind, data):
            return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
        def png(width):
            return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, 1, 8, 6, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(b"\0" + b"\0" * width * 4)) + chunk(b"IEND", b"")
        path = self.root / "image.png"
        path.write_bytes(png(1024))
        self.assertEqual(png_dimension(path), 1024)
        path.write_bytes(png(1025))
        with self.assertRaises(ValueError):
            png_dimension(path)
        data = bytearray(png(1)); data[-1] ^= 1; path.write_bytes(data)
        with self.assertRaises(ValueError):
            png_dimension(path)


if __name__ == "__main__":
    unittest.main()
