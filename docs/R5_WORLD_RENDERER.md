# R5 — Unreal World Renderer Completion Evidence

Date: 2026-10-04  
Branch: `remaster/r5-world-renderer`  
Acceptance implementation head: `df95ee5695e5dca42b87bdc81d741496fc2bafd6`  
Dedicated R5 run: `37207647696`  
R0/R1/R2/R3/R4 regression run: `37207647809`

## Status

**R5 COMPLETE — authoritative world-renderer implementation and production-scene
source/data acceptance passed.**

R5 converts the R3 world package and R4 authoritative overworld state into the
production Unreal presentation path without moving gameplay collision,
warps/connections, or movement rules into rendering code.

The accepted production path is:

**R4 authoritative map/state -> R5 map-change event -> chunk renderer ->
packaged Vanilla+ tileset descriptors/resources -> presentation-only pawn/camera.**

The real acceptance slice remains:

**Brendan's House 1F -> Littleroot -> Route 101.**

## Task 1 — Authoritative renderer coordinate/chunk core

**PASS**

- renderer consumes `GetCurrentMapForPresentation()`;
- map transitions are driven by `OnGameplayMapChanged`;
- tile/world math is centralized in `RemasterWorldGridMath.h`;
- maps are partitioned into 16x16 render chunks;
- renderer HISM collision is disabled because R4 owns gameplay collision;
- tile <-> local/world and chunk math are covered by `r5_world_grid_math`.

## Task 2 — Playable presentation bridge

**PASS**

- `ARemasterPlayerController` sends cardinal movement through R4
  `StepPlayer`;
- `ARemasterOverworldPawn` mirrors the resulting authoritative tile state;
- the pawn does not own gameplay collision;
- `R0GameMode` boots the production renderer, overworld pawn and follow
  camera;
- warp/connection map changes cause renderer/pawn resynchronization.

## Task 3 — Real Vanilla+ tileset semantics

**PASS**

The R5 source pipeline derives real visual semantics from the pinned Vanilla+
snapshot:

- exact primary/secondary tileset identities;
- split metatile-root vs visual-root support;
- real JASC-PAL palettes;
- real metatile binary decode;
- eight 8x8 tile entries per metatile;
- tile id, palette and H/V flip decoding;
- Vanilla layer type -> bottom/middle/top render-plane mapping.

No global metatile id is treated as a globally unique visual identity; the
renderer preserves `tileset + local metatile id`.

## Task 4 — Packaged render-source payload

**PASS**

`tools/build_r5_render_package.py` produces a deterministic,
runtime-package-safe payload corresponding to:

`Content/Generated/Render/`

The package contains:

- provenance-bearing `manifest.json`;
- one descriptor per real tileset;
- original source tile PNG and JASC palettes;
- normalized indexed tile payloads;
- 16x16 RGBA palette LUT payloads;
- per-file SHA-256 hashes;
- a deterministic aggregate content fingerprint.

Accepted package:

- tileset count: **75**
- content SHA-256:
  `85830099ea432fe3caaccae9afd25406a6bbc2105c71def1e73f9e61088dccf0`

The runtime package has no dependency on the vendored source tree.

## Task 5 — Unreal render catalog

**PASS**

`URemasterRenderCatalogSubsystem`:

- loads only `Content/Generated/Render/manifest.json`;
- rejects unsafe package-relative paths;
- validates source provenance and payload presence;
- resolves exact source tileset identity;
- lazy-loads and caches descriptors;
- validates dense metatile ids;
- validates decoded tile raw-u16 integrity;
- resolves `tileset + local metatile id` for the renderer.

Missing/invalid payloads fail visibly rather than silently inventing visuals.

## Task 6 — Descriptor-driven chunk geometry

**PASS**

`ARemasterWorldActor` now builds chunk geometry from packaged descriptors:

- primary metatile ids `0..511` resolve through the active primary tileset;
- secondary ids `512+` resolve through the active secondary tileset;
- every metatile creates its two source render planes;
- plane Z ordering uses tested `RemasterMetatileRenderMath`;
- each plane has exactly four source quadrants;
- tile id, palette, H/V flip are stored as per-instance custom data;
- chunk visual identity includes tileset/local-metatile/plane identity;
- descriptor absence is a hard render failure, not a fallback cube.

Gameplay collision remains exclusively R4.

## Task 7 — Runtime texture/material ingestion

**PASS**

The real Vanilla+ tile sheets are indexed PNG assets. R5 preserves that
semantics instead of flattening them to arbitrary RGBA source images.

The build pipeline produces:

- normalized index8 tile textures;
- 16x16 RGBA palette LUTs;
- package fingerprints for both.

The Unreal resource path:

- resolves only package files through the render catalog;
- loads and caches transient indexed tile/palette textures by tileset;
- binds them to dynamic metatile materials;
- feeds tile id / palette / flip custom data to the material contract;
- performs no per-frame source decoding.

Dedicated architecture validation requires this indexed material path.

## Task 8 — Real transition scene acceptance

**PASS**

The real acceptance is generated from the pinned R3 world conversion and the
actual deterministic R5 render package.

GitHub Actions artifact:

`r5-scene-acceptance`

Run:

`37207647696`

Validated maps:

### Brendan's House 1F

- map: `MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F`
- dimensions: 11x9
- tiles: 99
- primary: `gTileset_Building`
- secondary: `gTileset_BrendansMaysHouse`
- unique resolved render metatiles: 58
- render-plane instances: 198
- quadrant entries: 792
- real exit warp: `(8,8) -> MAP_LITTLEROOT_TOWN / warp 1`

### Littleroot

- map: `MAP_LITTLEROOT_TOWN`
- dimensions: 20x20
- tiles: 400
- primary: `gTileset_General`
- secondary: `gTileset_Petalburg`
- unique resolved render metatiles: 63
- render-plane instances: 800
- quadrant entries: 3,200
- real north connection: `up, offset 0 -> MAP_ROUTE101`

### Route 101

- map: `MAP_ROUTE101`
- dimensions: 20x20
- tiles: 400
- primary: `gTileset_General`
- secondary identity: `gTileset_Petalburg`
- actually used render tileset in the accepted layout: `gTileset_General`
- unique resolved render metatiles: 31
- render-plane instances: 800
- quadrant entries: 3,200

Acceptance totals:

- real map tiles: **899**
- render-plane instances: **1,798**
- render quadrant entries: **7,192**
- active descriptors loaded by the slice: **4**
- full R3 map count: **518**
- full R5 tileset count: **75**
- status: **PASS**

The existing R4 acceptance independently verifies the actual movement,
collision, warp, connection and save/reload semantics on the same slice.

The R5 source architecture validator independently verifies that production
input/pawn/renderer code is wired to that authoritative R4 path.

## Task 9 — Exit gate

Acceptance implementation head:

`df95ee5695e5dca42b87bdc81d741496fc2bafd6`

### Dedicated R5 run

GitHub Actions:

`37207647696`

Result: **SUCCESS**

Validated in that run:

- world-grid math — PASS
- metatile plane/custom-data math — PASS
- real Vanilla+ tileset source manifest — PASS
- real metatile descriptor decode — PASS
- deterministic package-safe render payload — PASS
- real Brendan house -> Littleroot -> Route 101 render acceptance — PASS
- scene report artifact generation/upload — PASS
- Unreal R5 source architecture validation — PASS

### Earlier-phase regression run

GitHub Actions:

`37207647809`

Result: **SUCCESS**

R0/R1/R2/R3/R4 regressions remain green.

## R5 exit checklist

- [x] authoritative tile/world coordinate contract;
- [x] chunked renderer;
- [x] presentation-only pawn/input bridge;
- [x] exact primary/secondary tileset identity;
- [x] real Vanilla+ metatile decode;
- [x] deterministic packaged render payload;
- [x] package-safe Unreal render catalog;
- [x] descriptor-driven two-plane chunk geometry;
- [x] tile/palette/flip custom-data path;
- [x] indexed texture + palette LUT runtime cache/material binding;
- [x] real three-map transition render acceptance;
- [x] Unreal source architecture validation;
- [x] R0-R4 regression gate.

**R5 exit gate: PASSED.**

## Verification scope

The repository CI validates the production Unreal source architecture and the
complete real-data/render-package path, but it does not contain a licensed
Unreal Engine runner. Therefore this evidence does not claim a rendered-frame
or packaged-APK smoke test executed inside the Unreal executable. The
production code path required for that runtime smoke test is implemented and
source-gated here.

## Next phase

R6 — Visual system, camera and environment presentation.

R6 will build on the R5 authoritative renderer with:

- modern 3D environmental language;
- buildings/props/vegetation;
- water presentation;
- lighting/shadows;
- day/night infrastructure;
- follow-camera refinement and occlusion;
- performance-oriented LOD/HLOD.

R4 remains the gameplay authority and R5 remains the world-data/render-source
bridge.
