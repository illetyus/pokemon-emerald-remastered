# R4 — Authoritative Overworld Runtime Completion Evidence

Date: 2026-10-04  
Branch: `remaster/r4-overworld-runtime`  
Acceptance implementation head: `d9c66ac5e459307d8b335edfe350a00727980e38`  
R4 acceptance run: `37188813328`  
R0/R1/R2/R3 regression run: `37188813321`

## Status

**R4 COMPLETE — authoritative free-roaming overworld runtime accepted.**

R4 turns the deterministic R3 Hoenn world package into a portable, save-backed
movement runtime. Gameplay decisions remain in the portable Emerald core;
Unreal loads R3 data, supplies transient world state, invokes the shared action
runtime, applies resolved transitions and presents the resulting state.

The accepted vertical slice is:

**Brendan's House 1F -> real directional door warp -> Littleroot -> collision
valid free roaming -> real north map connection -> Route 101 -> Emerald
save encode/decode -> identical map/position reload.**

Battles, final renderer, final UI and final Android controls remain outside R4.

## Authoritative movement path

The production movement path is now centered on:

- `remaster_emerald_player_step` — low-level collision/movement primitive;
- `remaster_emerald_overworld_step_action` — authoritative one-step field
  orchestration;
- `remaster_emerald_overworld_continue_action` — resumable continuation after
  synchronous immediate coordinate scripts;
- existing R1 transition functions for warp and connection application.

`URemasterWorldGameplaySubsystem::StepPlayer` consumes the same shared
`remaster_emerald_overworld_step_action` API used by the real R4 acceptance
fixture. It no longer reconstructs step-event ordering independently in Unreal.

## Task 1 — Authoritative player step

**PASS**

The portable step primitive:

- validates cardinal movement;
- applies raw Emerald collision bits;
- applies directional metatile blockers;
- applies elevation compatibility;
- applies runtime object collision;
- preserves blocked state without mutating player position;
- commits successful local movement to the R1-compatible save;
- resolves valid map-boundary connections.

Regression:

`emerald_overworld_player_step`

## Task 2 — Real Littleroot -> Route 101 acceptance

**PASS**

The R4 fixture is generated from the pinned real Vanilla+ source through the R3
converter. No substitute collision map is hand-authored.

The acceptance verifies:

1. Brendan's House 1F starts at `(8,7)`;
2. the real south-facing exit resolves the real door warp at `(8,8)`;
3. destination warp id 1 lands in Littleroot at `(5,8)`;
4. a path is executed across the real Littleroot 20x20 layout using production
   collision/elevation rules;
5. real coordinate-event ordering is evaluated while traversing the map;
6. the north boundary resolves the real `up` connection;
7. the connection enters Route 101 on its south edge;
8. the target tile is passable;
9. final Route 101 state survives Emerald save encode/decode.

Regression:

`r4_littleroot_overworld`

## Task 3 — Unreal production adapter

**PASS**

`URemasterWorldGameplaySubsystem` now:

- constructs `RemasterEmeraldMapView` directly from R3 `FRemasterMapIR`;
- supplies portable runtime object colliders;
- builds native connection, coordinate-event and warp definitions;
- calls `remaster_emerald_overworld_step_action`;
- resumes through `remaster_emerald_overworld_continue_action`;
- applies warp and connection results through the existing production
  transition functions;
- returns the final save-backed map/position to presentation code.

The source architecture validator explicitly rejects a return to duplicated
Unreal-side step orchestration.

## Task 4 — Runtime object collision state

**PASS**

Portable runtime object state models the distinction between map templates and
the active Vanilla object window.

Coverage includes:

- visibility flags;
- 64 source templates vs 16 active runtime slots;
- camera/view synchronization;
- current and previous occupied tile;
- elevation-sensitive collision;
- active/inactive state;
- explicit player/follower collision exemption;
- transient runtime position updates;
- full-world capacity validation across the Vanilla+ map catalog.

Regression:

`emerald_runtime_object_collision`

Full-world runtime-object capacity check:

`tests.test_r4_runtime_object_capacity`

## Task 5 — Step completion ordering

**PASS**

The shared action runtime preserves the R4/R2-owned field ordering:

- successful movement;
- coordinate-event scan;
- immediate coordinate script handoff;
- resumable continuation cursor;
- coordinate script handoff;
- weather coordinate application;
- warp resolution;
- directional warp resolution.

Blocked movement does not run tile-entry events.

Regression:

`emerald_step_event_ordering`

The real Littleroot acceptance also consumes converted coordinate events rather
than bypassing them.

## Task 6 — Special walking semantics

**PASS for R4 scope**

R4 implements the walking semantics required by the accepted slice and portable
field model:

- directional blockers;
- elevation rules;
- ledge detection;
- authoritative two-tile ledge movement;
- post-ledge step-event processing;
- ledge presentation hint;
- camera/map-boundary connection semantics;
- directional door/warp semantics.

Bike- and surf-specific movement remain outside this R4 gate unless a later
phase requires them.

## Task 7 — Save/reload continuity

**PASS**

The real acceptance encodes the final Route 101 state into an Emerald save
image, decodes it, and verifies:

- map group/number;
- player X/Y;
- layout identity;
- connection-style warp state;
- persistent story variable continuity.

Existing R2 checkpoint/save regressions remain green through the R0 Core
workflow.

## Task 8 — Exit gate

Acceptance implementation head:

`d9c66ac5e459307d8b335edfe350a00727980e38`

### Dedicated R4 run

GitHub Actions run:

`37188813328`

Result: **SUCCESS**

- `emerald_overworld_player_step` — PASS
- `emerald_step_event_ordering` — PASS
- `r4_littleroot_overworld` — PASS
- `emerald_runtime_object_collision` — PASS
- 4/4 dedicated C tests passed
- full-world runtime object capacity — PASS
- Unreal source architecture validation — PASS

### R0 / earlier-phase regression run

GitHub Actions run:

`37188813321`

Result: **SUCCESS**

- `portable-core` — PASS
- `scenario-schema` — PASS
- `world-converter` — PASS
- `r3-full-world` — PASS

Therefore R4 did not regress R1/R2 behavior or the complete 518-map R3 world
acceptance.

## R4 exit checklist

- [x] portable movement/collision core green;
- [x] runtime object collision green;
- [x] coord-before-warp ordering green;
- [x] directional warp semantics green;
- [x] ledge semantics green for walking scope;
- [x] real Brendan's House -> Littleroot -> Route 101 slice green;
- [x] real slice uses the same shared action runtime as production Unreal;
- [x] save/reload continuity demonstrated;
- [x] full-world runtime object capacity validated;
- [x] Unreal architecture validation green;
- [x] R1/R2 regressions green;
- [x] R3 full-world acceptance green.

**R4 exit gate: PASSED.**

## Next phase

R5 begins the Unreal world renderer.

R5 will convert the authoritative R3/R4 layout and state model into the actual
playable Unreal scene:

- map chunk/placement system;
- terrain/metatile geometry;
- material pipeline;
- chunk loading during map transitions;
- shared coordinate system between rendering and collision;
- no migration of gameplay rules into Blueprint.
