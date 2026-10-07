# R7 Environment and Camera — Source and Preparation Acceptance

Status: **implementation prepared; hosted acceptance pending first publication**.
This checkpoint covers pre-real-Unreal code, deterministic metadata, explicit
fallbacks and source budget validation. It does not assert a rendered 3D scene,
licensed Unreal/UHT compilation, commercial model acquisition or Android execution.

## Source and identity coverage

| Item | Verified source count |
| --- | ---: |
| Vanilla+ maps | 518 |
| Layouts, including unused/dynamic layouts | 441 |
| Tilesets | 75 |
| Source-backed R5 metatile identities | 18,318 |
| Additional explicit missing-descriptor identities | 85 |
| Complete R7 identity registry | 18,403 |
| Used identities across layouts and their border words | 11,118 |
| Unresolved identities after fallback | 0 |
| Verified imported 3D environment models | 0 |

Source pin remains `illetyus/pokezumrut-vanillaplus` at
`70db90c9077aed1272e746fc2537d9f12b95a91c`.
`build_r7_environment_package.py` uses the existing R3 layout decoder and R5
descriptor decoder. It hashes source map/layout words, tileset fingerprints,
labels, behavior/weather/map-type constants and the versioned R7 contract.
Output contains no timestamps, machine paths or commercial binary payloads.

Every identity has an explicit visual fallback. All 518 used maps resolve to
source-backed R5 descriptors. The 85 missing secondary identities occur only in
`LAYOUT_UNUSED_OUTDOOR_AREA`, whose secondary tileset is `0`; no used map references
that layout. These gaps are reported as gaps with visible basic planes, never
silently mapped to a different tileset. The renderer no longer discards an entire
otherwise valid map when an individual descriptor is absent.

Family hints come only from source labels and behavior names. Generic normal or
impassable metatiles do not identify a building/tree/fence reliably and remain
unclassified. All building/tree/rock/grass/sign/fence/cave/indoor-prop/water families
have the R5 surface fallback; signs additionally retain source background-event
landmarks. There is no verified ORAS environment acquisition, normalization,
local AssetSet, animated water material or HLOD proxy at this checkpoint.

## Runtime presentation

The camera reads core player coordinates through the save snapshot and R5
`TileToWorldLocation`, rather than engine velocity or a physics movement component.
Map-type profiles provide indoor/outdoor/cave/underwater framing. Interpolation
uses the tested exponential response, independent of frame rate for a fixed target.
First valid context and every world rebuild snap to the new target, including
same-map reloads. Missing/mismatched core context holds the camera and restores
any cutaway instances. Camera destruction also restores them. Lookahead remains
disabled until an authoritative motion snapshot exists.

The spawned environment controller reads core local RTC and runtime weather at
four updates per second. Day/night adjustments retain weather fog. Indoor/cave
profiles disable exterior sunlight; missing RTC retains the last valid visual
hour. Snow, abnormal weather and unresolved cycle selectors use a neutral tagged
fallback. Coordinate-event weather IDs are not interpreted as runtime weather
IDs. No core clock, weather cycle, flash, save, movement or encounter state is
changed. Water remains source metatiles; shader animation/particles are deferred.

Optional local 3D decorative fragments require exact `gTileset_Name:local-id`
binding, source/normalized/provenance hashes and explicitly validated import.
Unusable/missing/load-failed bindings retain R5 terrain underneath. Hash format
and import attestations are checked at runtime; actual file content is checked
offline. `validate_r7_local_assets.py` validates private triangulated centimetre
OBJ LODs, source/provenance hashes, PNG dimensions/CRC and budgets. Its report is
**PREIMPORT_VERIFIED**, always `import_validated=false`; it cannot certify an
Unreal mesh, material, pivot, cooked asset or device result.

Render components have no collision, overlaps or navigation influence. Optional
roof/wall fragments may opt into cutaway based on an open camera-to-player segment
against cached render bounds. Only the individual decorative instance is hidden;
its exact original transform is restored after obstruction ends. R5 surfaces
remain visible underneath. There are no gameplay physics traces or changes to
core collision/elevation/warp interpretation. Full identity strings are compared
inside chunk keys, preventing different identities from aliasing on a hash collision.

## Mobile source contract

| Constraint | Baseline |
| --- | --- |
| Chunk size | 16 × 16 source tiles |
| Per-model LOD0/1/2 triangles | ≤6,000 / 3,000 / 1,500, descending |
| Material slots / texture dimension | ≤2 / ≤1,024 |
| Optional 3D components / instances per chunk | ≤32 / ≤256 |
| Optional 3D LOD0 triangles per chunk | ≤60,000 |
| Optional 3D distance culling | 1,000–4,000 world units |
| Instancing | HISM, source row-order budget admission |
| Dynamic environment shadows / real-time skylight capture | Disabled |
| HLOD / Lumen | HLOD disabled baseline; Lumen not required |

Budget overflow retains the underlying R5 surface. These are enforced source
constraints for optional 3D fragments, not measurements of performance. The R5
base surface audit reports up to 304 identity/plane components in a source chunk;
the 32-component cap does not cover the legacy base renderer. Its actual draw
calls, memory, streaming and device frame time remain R18/R19 measurement work.
No all-world budget or high-FPS claim is made.

## Validation and phase reconciliation

Local checks passed: 21 R7 Python tests; all 202 Python regressions; 53 shared
native presentation checks under GCC C++17 with warnings as errors; deterministic
package regeneration/integrity verification; Unreal source authority validation;
clean diff formatting. Hosted full CMake/CTest acceptance is pending publication.
The shared C++ helper is genuinely compiled, while the Unreal actor/UHT layer is
source checked only.

Representative source audits cover Brendan's house, Littleroot, Route101,
GraniteCave_1F, Route105 and Underwater_Route124. Route105 is a source `MAP_TYPE_ROUTE`
with water metatiles; its map type is preserved rather than relabeled as an ocean
route. All 441 layouts and all 518 maps are also audited, beyond this sample set.

R7-P1/P2, I1–I7 and T1 are implemented in the pre-real-Unreal scope. V1/G1 await
hosted acceptance; D1/F1 are this evidence and the independent regression/source
reconciliation. R7-M1/M2 remain PR/merge/main gates.

The user explicitly authorized R7 before the R6 merge/main gates. R7 is therefore
prepared on dependent branch `r7-environment-presentation` based on verified R6
head `db8c50c76365bd656fc5db3bad2eba25a336b83c`. Main remains
`a7ed422da7817c05aa1eac21bdbca6bd6edced5e`. This direct instruction changes the
preparation sequence, while retaining the earlier PR/merge/main authorization
boundary. R6 and R7 must be integrated in dependency order when those gates are
authorized. Next preparation phase is R9; no subsequent phase starts automatically
under this R7-only instruction.

Actual 3D source acquisition/import/material/cutaway/LOD/cooked visibility,
UE 5.8.x/UHT and Android remain explicitly pending. Asset family fallback coverage
is not evidence that these modern 3D sources are ready.

## Reproduction

```sh
python tools/build_r7_environment_package.py vendor/vanillaplus /tmp/r7-package
python tools/build_r7_environment_package.py vendor/vanillaplus /tmp/r7-package --verify
python -m unittest discover -s tests -p 'test_r7*.py' -v
python -m unittest discover -s tests -p 'test_*.py' -v
python tools/validate_unreal_source.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Stage the metadata package in `unreal/Content/Generated/Environment` before cook.
Private normalized OBJ recipes use `schema_version: 1`, `models`, exact `identity`,
`source` and `provenance` `{path,sha256}` records, three `lods` records, optional
PNG `textures` records, `units: centimetres`, `pivot: tile-ground` and `cull_distance`.
Provenance must name the same identity/source hash and a source URL. Run:

```sh
python tools/validate_r7_local_assets.py /private/bindings.json /tmp/r7-package/manifest.json /private/preimport-report.json
```

After actual editor validation, set the optional environment AssetSet only in
local configuration and add its `/Game/Local/R7` folder to local cook settings.
Real imported mesh/material/LOD/bounds and cooked dependency checks are required
before setting its `bImportValidated` flag. Public defaults contain no local binding.
