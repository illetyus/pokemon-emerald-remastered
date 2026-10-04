# R4 — Authoritative Overworld Runtime Implementation Plan

Date: 2026-10-04  
Branch: `remaster/r4-overworld-runtime`  
Base: R3 merge `db99d233f58085e35b5f25fd05fe8588300df2b8`

## Goal

Turn the deterministic R3 Hoenn world package into an authoritative portable
free-roaming runtime without moving game rules into Unreal presentation code.

The first vertical acceptance slice is:

**Brendan's House 1F -> real door warp -> Littleroot -> collision-valid player
movement -> real north map connection -> Route 101.**

Battle, final controls, final rendering and encounter resolution remain outside
R4.

## Architecture rule

The portable core owns gameplay decisions and persistent position changes.

Unreal may:

- load R3 map data;
- provide visible object/runtime collider state;
- request a player step;
- load destination map metadata when the core reports a warp/connection;
- present the resulting state.

Unreal must not reimplement collision, elevation, warp-coordinate or connection
translation rules.

## Task 1 — Authoritative player-step primitive

Status: **DONE**

Files:

- `core/include/remaster/emerald_overworld.h`
- `core/src/emerald_movement.c`
- `tests/emerald_overworld_step_test.c`

Behavior:

- validate one cardinal free-roaming step;
- use existing Emerald collision/elevation/directional behavior rules;
- leave save coordinates unchanged when blocked;
- commit successful local tile movement to the R1-compatible save;
- report a warp after entering a real warp tile;
- report a map connection when attempting to cross a valid map boundary;
- leave destination-map loading to the host/transition layer.

TDD evidence:

- the new test first failed because `emerald_overworld.h` did not exist;
- after implementation, `emerald_overworld_player_step` passes.

## Task 2 — Real Littleroot world acceptance

Status: **DONE**

Files:

- `tools/build_r4_littleroot_world_fixture.py`
- `tests/r4_littleroot_fixture.h`
- `tests/r4_littleroot_overworld_test.c`
- `.github/workflows/r4-overworld.yml`

The fixture is generated from the pinned full Vanilla+ source through the R3
converter. No hand-authored replacement map/collision grid is used.

Acceptance verifies:

1. start at Brendan's House 1F coordinate `(8,7)`;
2. step south onto the real `(8,8)` door warp;
3. apply the real destination warp into Littleroot;
4. land at Littleroot `(5,8)` through destination warp id 1;
5. find and execute a collision-valid path across the real 20x20 Littleroot
   layout;
6. reach the north map edge without crossing another warp;
7. request the real `up` map connection;
8. apply that connection into Route 101;
9. land on Route 101's south row on a passable tile.

Dedicated CI run:

`37180449334`

Result:

- `emerald_overworld_player_step` — PASS
- `r4_littleroot_overworld` — PASS
- 2/2 R4 tests passed

## Task 3 — Unreal gameplay subsystem player-step adapter

Status: **DONE**

Add a single gameplay call that:

- builds `RemasterEmeraldMapView` from `FRemasterMapIR`;
- supplies current visible object colliders;
- calls `remaster_emerald_player_step`;
- applies reported warp/connection through the existing transition functions;
- refreshes `CurrentMap` and save-backed object template state;
- returns the authoritative resulting map/position to presentation code.

Do not duplicate collision logic in Blueprint/C++.

## Task 4 — Runtime object collision state

Status: **DONE**

Move from template-only object coordinates to runtime collider coordinates.

Requirements:

- visibility flags respected;
- current and previous object tile both participate exactly as the portable
  collision primitive expects;
- player/follower exemptions remain explicit;
- moving NPC state feeds the same collision path;
- regression coverage for hidden, visible, moving and elevation-separated
  objects.

## Task 5 — Step-completion event ordering

Status: **DONE**

After a successful local step, preserve Emerald field ordering for the subset
owned by R4/R2:

- warp resolution;
- coordinate trigger resolution;
- weather coordinate events;
- R2 script dispatch handoff;
- map-load/transition hooks.

A blocked step must not trigger a tile-entry event.

## Task 6 — Special walking movement semantics

Status: **DONE FOR R4 WALKING SCOPE**

Promote currently reported special collisions into authoritative behavior where
they belong in R4:

- ledge jump;
- directional blockers;
- elevation transitions;
- camera/map-boundary movement.

Bike/surf-specific mechanics remain deferred unless they are required by an R4
acceptance map.

## Task 7 — Save/reload continuity

Status: **DONE**

Acceptance must prove:

- player position after free roaming survives R1 save encode/decode;
- current map identity survives;
- warp/connection destination state survives;
- temporary field state resets only on the correct transition path;
- no R2 script checkpoint regression is introduced.

## Task 8 — R4 completion gate

Status: **PASSED**

R4 closes only when:

- portable core regressions are green;
- R3 full-world acceptance remains green;
- dedicated R4 acceptance is green;
- Unreal source architecture validation is green;
- real Littleroot -> Route 101 slice works through the production gameplay
  adapter, not only the C fixture;
- save/reload continuity is demonstrated;
- `docs/R4_OVERWORLD_RUNTIME.md` records final evidence and CI run IDs.

## Current checkpoint

R4 is **COMPLETE**.

Acceptance implementation head: `d9c66ac5e459307d8b335edfe350a00727980e38`.

Final R4 run `37189007165` and R0/R1/R2/R3 regression run
`37188920857` are green. Final evidence is recorded in
`docs/R4_OVERWORLD_RUNTIME.md`.

The next phase is R5 — Unreal world renderer.
