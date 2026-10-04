# R5 — Unreal World Renderer Implementation Plan

Date: 2026-10-04
Branch: `remaster/r5-world-renderer`
Base: R4 merge `a3560bc755589ff1c5b09b98fbe1434a202a393c`

## Goal

Render the authoritative R3/R4 world as the actual Unreal overworld without
moving gameplay rules into presentation code.

R5 acceptance target:

**Brendan's House / Littleroot / Route 101 are loaded from the authoritative
world state, rendered in the same tile coordinate system used by R4, and the
player can traverse the R4-accepted path while the renderer rebuilds correctly
across warp and map-connection transitions.**

R5 owns world geometry/chunking/render-source ingestion. Final art direction,
lighting, vegetation, water presentation and environment polish remain R6.

## Task 1 — Authoritative renderer coordinate/chunk core

Status: **DONE**

- production renderer consumes `GetCurrentMapForPresentation()`;
- map changes arrive through `OnGameplayMapChanged`;
- tile <-> Unreal world coordinate math is centralized in
  `RemasterWorldGridMath.h`;
- 16x16 render chunks are used;
- renderer collision remains disabled because R4 is authoritative;
- `r5_world_grid_math` regression is green.

## Task 2 — Playable presentation bridge

Status: **DONE**

- `ARemasterOverworldPawn` mirrors authoritative save coordinates;
- `ARemasterPlayerController` routes cardinal input through R4
  `StepPlayer`;
- pawn position/facing is updated from authoritative step results;
- GameMode spawns the renderer, pawn and follow camera;
- warp/connection map changes resynchronize renderer and pawn.

## Task 3 — Real Vanilla+ tileset semantics

Status: **DONE**

- source-derived tileset manifest;
- real primary/secondary tileset identity;
- split metatile/visual asset roots;
- JASC-PAL source validation;
- real metatile binary decode;
- eight 8x8 entries per metatile;
- tile id, palette, H/V flip decoding;
- Vanilla layer type -> bottom/middle/top render plane mapping;
- dedicated R5 CI and R0 regression CI green at
  `835a86c94018f17a50c2e7d22e9cb0ad65398b97`.

## Task 4 — Packaged render-source payload

Status: **DONE**

Create a deterministic build artifact matching Unreal's existing generated-data
contract:

`Content/Generated/Render/`

The package will contain:

- manifest with source provenance/fingerprints;
- one descriptor JSON per tileset;
- source tile sheet PNG for each visual asset root;
- source JASC palettes required by each tileset;
- no dependency on the vendored source tree at runtime.

Acceptance:

- package is deterministic;
- all descriptor paths resolve inside the package;
- all real Vanilla+ tilesets are represented;
- primary/secondary and split asset-root variants remain distinct;
- no absolute paths escape the package.

## Task 5 — Unreal render catalog

Status: **DONE**

Add a GameInstance render-catalog subsystem that:

- loads `Content/Generated/Render/manifest.json`;
- resolves primary/secondary tilesets by exact source identity;
- loads per-tileset descriptor data on demand;
- exposes decoded metatile render planes to `ARemasterWorldActor`;
- validates descriptor/source fingerprints where practical;
- fails visibly rather than silently inventing assets.

## Task 6 — Descriptor-driven chunk geometry

Status: **DONE**

Replace fallback one-block-per-metatile placement with descriptor-driven
render planes:

- primary ids 0..511 resolve in the primary tileset;
- secondary ids 512+ resolve in the active secondary tileset;
- each metatile places its two Vanilla render planes;
- layer ordering is deterministic;
- H/V flip and palette identity are retained in render instance data;
- chunk identity includes tileset + local metatile + plane/material identity.

Gameplay collision remains exclusively R4.

## Task 7 — Runtime texture/material ingestion

Status: **DONE**

Turn packaged tile-sheet/palette data into Unreal render resources:

- package-safe image loading;
- palette lookup;
- tile/palette/flip data reaches the material path;
- cache by tileset/fingerprint;
- no per-frame source decoding;
- deterministic fallback/error presentation for missing render assets.

This task provides the R5 material pipeline; visual art replacement remains R6.

## Task 8 — Real transition scene acceptance

Status: **DONE**

Demonstrate the production scene path:

1. Brendan's House renders from its real primary/secondary tilesets;
2. player exits via the R4 directional warp;
3. renderer reloads Littleroot from the gameplay map-change event;
4. player traverses Littleroot using R4 movement;
5. renderer reloads Route 101 after the real map connection;
6. pawn/world coordinates remain aligned with authoritative tile coordinates;
7. no Blueprint gameplay collision or transition logic is introduced.

## Task 9 — R5 exit gate

Status: **PASSED**

R5 closes only when:

- dedicated R5 renderer CI is green;
- R0/R1/R2/R3/R4 regressions remain green;
- full render package is deterministic;
- Unreal architecture validation is green;
- real R4 accepted slice uses production renderer/pawn/input path;
- final evidence is recorded in `docs/R5_WORLD_RENDERER.md`.


## Current checkpoint

R5 is **COMPLETE**.

Acceptance implementation head:
`df95ee5695e5dca42b87bdc81d741496fc2bafd6`.

Dedicated R5 run `37207647696` and R0/R1/R2/R3/R4 regression run
`37207647809` are green.

Final evidence is recorded in `docs/R5_WORLD_RENDERER.md`.

The next phase is R6 — visual system, camera and environment presentation.
