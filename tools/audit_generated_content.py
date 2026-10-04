#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

from world_fingerprint import (
    fingerprint_document_content,
    fingerprint_world_package,
)


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    manifest_path = root / "manifest.json"

    if not manifest_path.is_file():
        return [f"missing manifest: {manifest_path}"]

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    maps = manifest.get("maps", [])

    if manifest.get("map_count") != len(maps):
        errors.append("manifest map_count does not match maps array")

    known_ids = {entry.get("id") for entry in maps}
    manifest_by_id = {
        entry.get("id"): entry
        for entry in maps
        if entry.get("id")
    }
    manifest_by_name = {
        entry.get("name"): entry
        for entry in maps
        if entry.get("name")
    }
    known_files = set()
    known_numeric_maps: set[tuple[int, int]] = set()

    provenance_file = manifest.get("provenance_file")
    provenance = None
    if provenance_file is not None:
        provenance_path = root / provenance_file
        if not provenance_path.is_file():
            errors.append(f"missing provenance file: {provenance_file}")
        else:
            provenance = json.loads(
                provenance_path.read_text(encoding="utf-8")
            )
            source_counts = provenance.get("source_counts", {})
            if source_counts.get("map_count") != manifest.get("map_count"):
                errors.append(
                    "provenance source map_count does not match manifest"
                )
            if (
                "layout_count" in manifest
                and source_counts.get("layout_count")
                != manifest.get("layout_count")
            ):
                errors.append(
                    "provenance source layout_count does not match manifest"
                )

            recomputed = fingerprint_world_package(root)
            if provenance.get("package_sha256") != recomputed["package_sha256"]:
                errors.append("provenance package fingerprint mismatch")
            if provenance.get("file_sha256") != recomputed["file_sha256"]:
                errors.append("provenance file fingerprints mismatch")

    script_catalog_by_map: dict[str, dict] = {}
    known_script_ids: set[str] = set()
    scripts_file = manifest.get("scripts_file")
    if scripts_file is not None:
        script_catalog_path = root / scripts_file
        if not script_catalog_path.is_file():
            errors.append(f"missing script catalog: {scripts_file}")
        else:
            script_catalog = json.loads(
                script_catalog_path.read_text(encoding="utf-8")
            )
            script_maps = script_catalog.get("maps", [])
            script_labels = script_catalog.get("labels", [])
            script_sources = script_catalog.get("source_files", [])

            if script_catalog.get("map_count") != manifest.get("map_count"):
                errors.append(
                    "script catalog map_count does not match world manifest"
                )
            if script_catalog.get("script_label_count") != len(script_labels):
                errors.append(
                    "script catalog script_label_count does not match labels array"
                )
            if manifest.get("script_label_count") != len(script_labels):
                errors.append(
                    "manifest script_label_count does not match script catalog"
                )
            if (
                script_catalog.get("script_source_file_count")
                != len(script_sources)
            ):
                errors.append(
                    "script catalog script_source_file_count does not match source_files"
                )
            if (
                manifest.get("script_source_file_count")
                != len(script_sources)
            ):
                errors.append(
                    "manifest script_source_file_count does not match script catalog"
                )

            for script_map in script_maps:
                map_name = script_map.get("map")
                if not isinstance(map_name, str) or not map_name:
                    errors.append("script catalog map entry has invalid map name")
                    continue
                if map_name in script_catalog_by_map:
                    errors.append(
                        f"duplicate script catalog map entry {map_name}"
                    )
                    continue
                script_catalog_by_map[map_name] = script_map

            for label in script_labels:
                script_id = label.get("script_id")
                if not isinstance(script_id, str) or not script_id:
                    errors.append("script catalog label has invalid script_id")
                    continue
                if script_id in known_script_ids:
                    errors.append(f"duplicate script catalog label {script_id}")
                    continue
                known_script_ids.add(script_id)

    encounters_file = manifest.get("encounters_file")
    if encounters_file is not None:
        encounter_path = root / encounters_file
        if not encounter_path.is_file():
            errors.append(f"missing encounter catalog: {encounters_file}")
        else:
            encounter_catalog = json.loads(
                encounter_path.read_text(encoding="utf-8")
            )
            encounter_groups = encounter_catalog.get("groups", [])
            if encounter_catalog.get("group_count") != len(encounter_groups):
                errors.append(
                    "encounter catalog group_count does not match groups array"
                )
            if manifest.get("encounter_group_count") != len(encounter_groups):
                errors.append(
                    "manifest encounter_group_count does not match encounter catalog"
                )

            counted_map_encounters = 0
            for group_index, group in enumerate(encounter_groups):
                fields = group.get("fields", [])
                field_slots = {}
                for field in fields:
                    field_type = field.get("type")
                    rates = field.get("encounter_rates", [])
                    if isinstance(field_type, str) and isinstance(rates, list):
                        field_slots[field_type] = len(rates)

                for encounter_index, encounter in enumerate(
                    group.get("encounters", [])
                ):
                    context = (
                        f"encounter group {group_index} entry {encounter_index}"
                    )
                    if group.get("for_maps"):
                        counted_map_encounters += 1
                        map_id = encounter.get("map")
                        if map_id not in manifest_by_id:
                            errors.append(
                                f"{context}: encounter map {map_id!r} "
                                "is missing from world manifest"
                            )
                        else:
                            target = manifest_by_id[map_id]
                            if encounter.get("map_name") != target.get("name"):
                                errors.append(
                                    f"{context}: encounter map_name mismatch"
                                )
                            if encounter.get("group_num") != target.get("group_num"):
                                errors.append(
                                    f"{context}: encounter group_num mismatch"
                                )
                            if encounter.get("map_num") != target.get("map_num"):
                                errors.append(
                                    f"{context}: encounter map_num mismatch"
                                )

                    for field_name, value in encounter.items():
                        if not field_name.endswith("_mons"):
                            continue
                        if not isinstance(value, dict):
                            errors.append(
                                f"{context}.{field_name}: expected object"
                            )
                            continue

                        rate = value.get("encounter_rate")
                        if not isinstance(rate, int) or rate < 0 or rate > 100:
                            errors.append(
                                f"{context}.{field_name}: invalid encounter_rate"
                            )

                        mons = value.get("mons", [])
                        expected_slots = field_slots.get(field_name)
                        if (
                            expected_slots is not None
                            and len(mons) != expected_slots
                        ):
                            errors.append(
                                f"{context}.{field_name}: slot count mismatch"
                            )

                        for mon_index, mon in enumerate(mons):
                            min_level = mon.get("min_level")
                            max_level = mon.get("max_level")
                            if (
                                not isinstance(min_level, int)
                                or not isinstance(max_level, int)
                                or min_level < 1
                                or max_level > 100
                                or min_level > max_level
                            ):
                                errors.append(
                                    f"{context}.{field_name}.mons[{mon_index}]: "
                                    "invalid level range"
                                )
                            if not isinstance(mon.get("species_id"), int):
                                errors.append(
                                    f"{context}.{field_name}.mons[{mon_index}]: "
                                    "missing numeric species_id"
                                )

            if encounter_catalog.get("map_encounter_count") != counted_map_encounters:
                errors.append(
                    "encounter catalog map_encounter_count does not match entries"
                )
            if manifest.get("map_encounter_count") != counted_map_encounters:
                errors.append(
                    "manifest map_encounter_count does not match encounter catalog"
                )

    layout_catalog_by_id: dict[str, dict] = {}
    layouts_file = manifest.get("layouts_file")
    if layouts_file is not None:
        layout_catalog_path = root / layouts_file
        if not layout_catalog_path.is_file():
            errors.append(f"missing layout catalog: {layouts_file}")
        else:
            layout_catalog = json.loads(
                layout_catalog_path.read_text(encoding="utf-8")
            )
            layout_entries = layout_catalog.get("layouts", [])
            manifest_layout_count = manifest.get("layout_count")
            catalog_layout_count = layout_catalog.get("layout_count")

            if manifest_layout_count != catalog_layout_count:
                errors.append(
                    "manifest layout_count does not match layout catalog"
                )
            if catalog_layout_count != len(layout_entries):
                errors.append(
                    "layout catalog layout_count does not match layouts array"
                )

            seen_layout_nums: set[int] = set()
            map_names = {
                item.get("name")
                for item in maps
                if isinstance(item.get("name"), str)
            }
            expected_usage: dict[str, list[str]] = {}
            for map_entry in maps:
                layout_id = map_entry.get("layout")
                map_name = map_entry.get("name")
                if isinstance(layout_id, str) and isinstance(map_name, str):
                    expected_usage.setdefault(layout_id, []).append(map_name)

            for layout_entry in layout_entries:
                layout_id = layout_entry.get("id")
                layout_num = layout_entry.get("layout_num")
                rel_layout = layout_entry.get("file")
                used_by = layout_entry.get("used_by_maps", [])

                if not isinstance(layout_id, str) or not layout_id:
                    errors.append("layout catalog entry has invalid id")
                    continue
                if layout_id in layout_catalog_by_id:
                    errors.append(f"duplicate layout catalog id {layout_id}")
                    continue
                layout_catalog_by_id[layout_id] = layout_entry

                if not isinstance(layout_num, int) or layout_num <= 0:
                    errors.append(
                        f"{layout_id}: invalid layout_num {layout_num!r}"
                    )
                elif layout_num in seen_layout_nums:
                    errors.append(
                        f"{layout_id}: duplicate numeric layout id {layout_num}"
                    )
                else:
                    seen_layout_nums.add(layout_num)

                if not isinstance(rel_layout, str) or not rel_layout:
                    errors.append(f"{layout_id}: missing layout file")
                    continue

                layout_path = root / rel_layout
                if not layout_path.is_file():
                    errors.append(f"missing layout file: {rel_layout}")
                    continue

                layout_doc = json.loads(
                    layout_path.read_text(encoding="utf-8")
                )
                canonical = layout_doc.get("layout", {})
                if canonical.get("id") != layout_id:
                    errors.append(
                        f"{rel_layout}: layout catalog/file id mismatch"
                    )
                if canonical.get("layout_num") != layout_num:
                    errors.append(
                        f"{rel_layout}: layout catalog/file number mismatch"
                    )
                if layout_doc.get("used_by_maps", []) != used_by:
                    errors.append(
                        f"{rel_layout}: layout usage differs from catalog"
                    )

                if not isinstance(used_by, list):
                    errors.append(f"{layout_id}: used_by_maps must be an array")
                    continue
                unknown_users = [
                    name for name in used_by if name not in map_names
                ]
                if unknown_users:
                    errors.append(
                        f"{layout_id}: unknown map in used_by_maps "
                        f"{unknown_users[0]!r}"
                    )

                expected = sorted(expected_usage.get(layout_id, []))
                if sorted(used_by) != expected:
                    errors.append(
                        f"{layout_id}: layout usage does not match map manifest"
                    )

    for entry in maps:
        rel = entry.get("file")
        if not rel:
            errors.append(f"map {entry.get('id')} has no file")
            continue

        if rel in known_files:
            errors.append(f"duplicate map file: {rel}")
        known_files.add(rel)

        group_num = entry.get("group_num")
        map_num = entry.get("map_num")
        layout_num = entry.get("layout_num")

        if not isinstance(group_num, int) or group_num < 0:
            errors.append(f"{rel}: invalid manifest group_num {group_num!r}")
        if not isinstance(map_num, int) or map_num < 0:
            errors.append(f"{rel}: invalid manifest map_num {map_num!r}")
        if not isinstance(layout_num, int) or layout_num <= 0:
            errors.append(f"{rel}: invalid manifest layout_num {layout_num!r}")

        if isinstance(group_num, int) and isinstance(map_num, int):
            numeric_key = (group_num, map_num)
            if numeric_key in known_numeric_maps:
                errors.append(
                    f"{rel}: duplicate numeric map address "
                    f"{group_num},{map_num}"
                )
            known_numeric_maps.add(numeric_key)

        path = root / rel
        if not path.is_file():
            errors.append(f"missing map file: {rel}")
            continue

        doc = json.loads(path.read_text(encoding="utf-8"))
        map_doc = doc.get("map", {})
        layout = doc.get("layout", {})

        if doc.get("schema_version") != 1:
            errors.append(f"{rel}: unsupported schema version")

        if "fingerprint_sha256" in doc:
            expected_map_fingerprint = fingerprint_document_content(doc)
            if doc.get("fingerprint_sha256") != expected_map_fingerprint:
                errors.append(f"{rel}: map fingerprint mismatch")
        elif provenance_file is not None:
            errors.append(f"{rel}: missing map fingerprint")

        width = int(layout.get("width", 0))
        height = int(layout.get("height", 0))
        blocks = layout.get("raw_blocks_u16", [])
        trailing = layout.get("trailing_words_u16", [])
        source_word_count = int(layout.get("source_word_count", len(blocks) + len(trailing)))
        active_word_count = int(layout.get("active_word_count", len(blocks)))
        border_active = layout.get("border_active_words_u16", [])
        border_trailing = layout.get("border_trailing_words_u16", [])
        border_source_word_count = int(
            layout.get(
                "border_source_word_count",
                len(border_active) + len(border_trailing),
            )
        )

        metatile_ids = layout.get("metatile_ids_u16", [])
        collision = layout.get("collision_u8", [])
        elevation = layout.get("elevation_u8", [])

        primary_attrs = layout.get("primary_metatile_attributes_u16", [])
        secondary_attrs = layout.get("secondary_metatile_attributes_u16", [])
        primary_behavior = layout.get("primary_metatile_behavior_u8", [])
        secondary_behavior = layout.get("secondary_metatile_behavior_u8", [])
        primary_layer = layout.get("primary_metatile_layer_u8", [])
        secondary_layer = layout.get("secondary_metatile_layer_u8", [])
        secondary_tileset = layout.get("secondary_tileset")
        secondary_tileset_missing = secondary_tileset in (None, 0, "0", "")

        if width <= 0 or height <= 0:
            errors.append(f"{rel}: invalid dimensions")
        elif active_word_count != width * height:
            errors.append(
                f"{rel}: active_word_count {active_word_count} != {width}x{height}"
            )
        elif len(blocks) != active_word_count:
            errors.append(
                f"{rel}: active block count {len(blocks)} != {active_word_count}"
            )

        if source_word_count != len(blocks) + len(trailing):
            errors.append(
                f"{rel}: source_word_count does not match active + trailing words"
            )

        if len(border_active) != 4:
            errors.append(
                f"{rel}: Emerald border must expose exactly 4 active words"
            )

        if border_source_word_count != len(border_active) + len(border_trailing):
            errors.append(
                f"{rel}: border_source_word_count does not match active + trailing words"
            )

        for name, values in (
            ("metatile_ids_u16", metatile_ids),
            ("collision_u8", collision),
            ("elevation_u8", elevation),
        ):
            if len(values) != active_word_count:
                errors.append(
                    f"{rel}: {name} count {len(values)} != active word count "
                    f"{active_word_count}"
                )

        for side, attrs, behavior, layer in (
            ("primary", primary_attrs, primary_behavior, primary_layer),
            ("secondary", secondary_attrs, secondary_behavior, secondary_layer),
        ):
            attributes_optional = side == "secondary" and secondary_tileset_missing
            if not attrs and not attributes_optional:
                errors.append(f"{rel}: {side} metatile attributes are empty")
            if len(attrs) > 512:
                errors.append(
                    f"{rel}: {side} metatile attributes exceed 512 entries"
                )
            if len(behavior) != len(attrs):
                errors.append(
                    f"{rel}: {side} behavior count does not match attributes"
                )
            if len(layer) != len(attrs):
                errors.append(
                    f"{rel}: {side} layer count does not match attributes"
                )

        for metatile_id in metatile_ids:
            if not isinstance(metatile_id, int) or metatile_id < 0 or metatile_id >= 1024:
                errors.append(f"{rel}: invalid metatile id {metatile_id!r}")
                continue

            if metatile_id < 512:
                if metatile_id >= len(primary_attrs):
                    errors.append(
                        f"{rel}: primary metatile {metatile_id} has no attribute entry"
                    )
            else:
                secondary_index = metatile_id - 512
                if secondary_index >= len(secondary_attrs):
                    errors.append(
                        f"{rel}: secondary metatile {metatile_id} has no attribute entry"
                    )

        if map_doc.get("id") != entry.get("id"):
            errors.append(f"{rel}: manifest/map id mismatch")

        if map_doc.get("group_num") != group_num:
            errors.append(f"{rel}: manifest/map group_num mismatch")
        if map_doc.get("map_num") != map_num:
            errors.append(f"{rel}: manifest/map map_num mismatch")
        if map_doc.get("layout_num") != layout_num:
            errors.append(f"{rel}: manifest/map layout_num mismatch")

        if scripts_file is not None:
            map_name = entry.get("name")
            script_map = script_catalog_by_map.get(map_name)
            if script_map is None:
                errors.append(
                    f"{rel}: map missing from script catalog"
                )
            else:
                expected_ownership = script_map.get("script_ownership")
                if entry.get("script_ownership") != expected_ownership:
                    errors.append(
                        f"{rel}: manifest script ownership differs from script catalog"
                    )
                if map_doc.get("script_ownership") != expected_ownership:
                    errors.append(
                        f"{rel}: map script ownership differs from script catalog"
                    )

        if layouts_file is not None:
            layout_id = entry.get("layout")
            if layout_id not in layout_catalog_by_id:
                errors.append(
                    f"{rel}: map layout {layout_id!r} missing from layout catalog"
                )
            elif map_doc.get("layout") != layout_id:
                errors.append(f"{rel}: manifest/map layout id mismatch")

        weather_id = map_doc.get("weather_id")
        map_type_id = map_doc.get("map_type_id")

        for source_key, numeric_key in (
            ("music", "music_id"),
            ("region_map_section", "region_map_section_id"),
            ("battle_scene", "battle_scene_id"),
        ):
            if source_key in map_doc and not isinstance(map_doc.get(numeric_key), int):
                errors.append(
                    f"{rel}: {source_key} is missing numeric {numeric_key}"
                )

        valid_weather_ids = set(range(16)) | {20, 21}
        if not isinstance(weather_id, int) or weather_id not in valid_weather_ids:
            errors.append(
                f"{rel}: invalid numeric weather_id {weather_id!r}"
            )

        if not isinstance(map_type_id, int) or map_type_id not in range(10):
            errors.append(
                f"{rel}: invalid numeric map_type_id {map_type_id!r}"
            )

        shared_events_map = map_doc.get("shared_events_map")
        if shared_events_map is not None:
            if shared_events_map not in manifest_by_name:
                errors.append(
                    f"{rel}: shared_events_map {shared_events_map!r} "
                    "is missing from manifest"
                )
            else:
                shared_entry = manifest_by_name[shared_events_map]
                shared_rel = shared_entry.get("file")
                shared_path = root / shared_rel if shared_rel else None

                if shared_path is None or not shared_path.is_file():
                    errors.append(
                        f"{rel}: shared events source file is missing"
                    )
                else:
                    shared_doc = json.loads(
                        shared_path.read_text(encoding="utf-8")
                    )
                    shared_map_doc = shared_doc.get("map", {})

                    for event_key in (
                        "object_events",
                        "warp_events",
                        "coord_events",
                        "bg_events",
                    ):
                        if map_doc.get(event_key, []) != shared_map_doc.get(
                            event_key,
                            [],
                        ):
                            errors.append(
                                f"{rel}: shared {event_key} differ from "
                                f"{shared_events_map}"
                            )

        for object_index, obj in enumerate(map_doc.get("object_events", []), start=1):
            script_id = obj.get("script_id")
            if scripts_file is not None and script_id is not None and script_id not in known_script_ids:
                errors.append(
                    f"{rel}: object {object_index} has unknown script_id {script_id!r}"
                )
            if obj.get("script") in {"0", "0x0", "0X0", "NULL", "null"} and obj.get("script_id") is not None:
                errors.append(
                    f"{rel}: object {object_index} null script sentinel must have null script_id"
                )
            if obj.get("local_id") != object_index:
                errors.append(
                    f"{rel}: object local_id {obj.get('local_id')!r} "
                    f"!= Vanilla array index {object_index}"
                )
            if "flag" in obj and not isinstance(obj.get("flag_id"), int):
                errors.append(
                    f"{rel}: object {object_index} is missing numeric flag_id"
                )
            if (
                "trainer_sight_or_berry_tree_id" in obj
                and not isinstance(
                    obj.get("trainer_sight_or_berry_tree_id_u16"),
                    int,
                )
            ):
                errors.append(
                    f"{rel}: object {object_index} is missing numeric "
                    "trainer/berry id"
                )

        for connection in map_doc.get("connections", []):
            target = connection.get("map")
            if "direction" in connection:
                direction_id = connection.get("direction_id")
                if not isinstance(direction_id, int) or direction_id not in {1, 2, 3, 4, 5, 6}:
                    errors.append(
                        f"{rel}: connection {target!r} is missing valid direction_id"
                    )
            if target and target not in known_ids:
                errors.append(
                    f"{rel}: connection references unknown map {target}"
                )
            elif target:
                expected = manifest_by_id[target]
                if connection.get("dest_group_num") != expected.get("group_num"):
                    errors.append(
                        f"{rel}: connection {target} group target mismatch"
                    )
                if connection.get("dest_map_num") != expected.get("map_num"):
                    errors.append(
                        f"{rel}: connection {target} map target mismatch"
                    )

        for warp in map_doc.get("warp_events", []):
            target = warp.get("dest_map")
            is_dynamic = bool(warp.get("dynamic_target", False))

            if is_dynamic:
                if target != "MAP_DYNAMIC":
                    errors.append(
                        f"{rel}: dynamic warp must target MAP_DYNAMIC"
                    )
                if not isinstance(warp.get("dest_warp_id_u16"), int):
                    errors.append(
                        f"{rel}: dynamic warp is missing numeric dest_warp_id"
                    )
                continue

            if target and target not in known_ids:
                errors.append(
                    f"{rel}: warp references unknown map {target}"
                )
            elif target:
                expected = manifest_by_id[target]
                if warp.get("dest_group_num") != expected.get("group_num"):
                    errors.append(
                        f"{rel}: warp {target} group target mismatch"
                    )
                if warp.get("dest_map_num") != expected.get("map_num"):
                    errors.append(
                        f"{rel}: warp {target} map target mismatch"
                    )
                if not isinstance(warp.get("dest_warp_id_u16"), int):
                    errors.append(
                        f"{rel}: warp {target} is missing numeric dest_warp_id"
                    )

        for coord_index, coord in enumerate(map_doc.get("coord_events", [])):
            script_id = coord.get("script_id")
            if scripts_file is not None and script_id is not None and script_id not in known_script_ids:
                errors.append(
                    f"{rel}: coord event {coord_index} has unknown script_id {script_id!r}"
                )
            if coord.get("script") in {"0", "0x0", "0X0", "NULL", "null"} and coord.get("script_id") is not None:
                errors.append(
                    f"{rel}: coord event {coord_index} null script sentinel must have null script_id"
                )
            coord_type = coord.get("type")
            if coord_type == "trigger":
                if not isinstance(coord.get("var_id"), int):
                    errors.append(
                        f"{rel}: coord event {coord_index} is missing numeric var_id"
                    )
                if not isinstance(coord.get("var_value_u16"), int):
                    errors.append(
                        f"{rel}: coord event {coord_index} is missing numeric var_value"
                    )
            elif coord_type == "weather":
                if not isinstance(coord.get("weather_id"), int):
                    errors.append(
                        f"{rel}: weather coord event {coord_index} "
                        f"is missing numeric weather_id"
                    )

        for bg_index, bg in enumerate(map_doc.get("bg_events", [])):
            script_id = bg.get("script_id")
            if scripts_file is not None and script_id is not None and script_id not in known_script_ids:
                errors.append(
                    f"{rel}: bg event {bg_index} has unknown script_id {script_id!r}"
                )
            if bg.get("script") in {"0", "0x0", "0X0", "NULL", "null"} and bg.get("script_id") is not None:
                errors.append(
                    f"{rel}: bg event {bg_index} null script sentinel must have null script_id"
                )
            bg_type = bg.get("type")

            if bg_type == "sign":
                facing = bg.get("player_facing_dir_id")
                kind = bg.get("kind_id")
                if not isinstance(facing, int) or facing < 0 or facing > 4:
                    errors.append(
                        f"{rel}: sign bg event {bg_index} is missing valid "
                        "numeric facing id"
                    )
                if kind != facing:
                    errors.append(
                        f"{rel}: sign bg event {bg_index} kind/facing mismatch"
                    )

            elif bg_type == "hidden_item":
                if bg.get("kind_id") != 7:
                    errors.append(
                        f"{rel}: hidden-item bg event {bg_index} "
                        "must use kind_id 7"
                    )
                if not isinstance(bg.get("item_id"), int):
                    errors.append(
                        f"{rel}: hidden-item bg event {bg_index} "
                        "is missing numeric item_id"
                    )
                if not isinstance(bg.get("flag_id"), int):
                    errors.append(
                        f"{rel}: hidden-item bg event {bg_index} "
                        "is missing numeric flag_id"
                    )

            elif bg_type == "secret_base":
                if bg.get("kind_id") != 8:
                    errors.append(
                        f"{rel}: secret-base bg event {bg_index} "
                        "must use kind_id 8"
                    )
                if not isinstance(bg.get("secret_base_id_u16"), int):
                    errors.append(
                        f"{rel}: secret-base bg event {bg_index} "
                        "is missing numeric secret-base id"
                    )

            else:
                errors.append(
                    f"{rel}: unknown background event type {bg_type!r}"
                )

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("generated_root", type=Path)
    args = parser.parse_args()

    errors = audit(args.generated_root)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print("Generated content audit passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
