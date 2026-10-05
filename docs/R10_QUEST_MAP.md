# R10 — Map + Quest Guidance Completion Evidence

Date: 2026-10-05  
Branch: `remaster/r10-quest-map`  
Acceptance implementation head: `19d33c32dcce2f7841ba8e4c9ae9656dae917820`  
Dedicated R10 run: `37263210755`  
R0 regression run: `37263210890`  
R4 regression run: `37263210850`  
R5 regression run: `37263210820`

## Status

**R10 COMPLETE — main-story objective and map guidance are derived from
authoritative Emerald state without creating a second progression system.**

The accepted flow is:

```text
Emerald/Vanilla+ flags + vars
        ↓
portable R10 resolver
        ↓
one active main-story objective
        ↓
region / map / object target
        ↓
Unreal presentation-only navigation adapter
```

## Core rule

R10 does not persist quest progress.

There is no remaster-owned current-quest field in SaveBlock1, SaveBlock2 or
Pokémon storage.

Objective state is recalculated from the existing Emerald flag/var state.

## Canonical objective catalog

R10 contains exactly **32** main-story objectives from the existing Vanilla+
Phase 10A chain, re-audited against the pinned source.

The catalog is declared in:

`data/r10/quest_catalog.json`

and deterministically compiled into:

`core/src/emerald_quest_catalog.inc`

The generator:

`tools/build_r10_quest_catalog.py`

resolves numeric identities from the pinned Vanilla+ source rather than keeping
a second hand-maintained numeric table.

Validated source identities include:

- flags;
- vars;
- map group / map number;
- Region Map section IDs;
- canonical Region Map marker coordinates;
- local object-event targets.

## Local target audit

There are **17 object-event objective entries**.

Each target is validated against the real map JSON and the exact object-event
script for that local ID.

Representative accepted targets include:

- Norman — Petalburg Gym local ID 1;
- Roxanne — Rustboro Gym local ID 1;
- Team Aqua grunt — Rusturf Tunnel local ID 6;
- Steven — Granite Cave Steven's Room local ID 1;
- Captain Stern — Oceanic Museum 2F local ID 1;
- Brawly — Dewford Gym local ID 1;
- Wattson — Mauville Gym local ID 1;
- Flannery — Lavaridge Gym local ID 1;
- Steven — Route 120 local ID 31;
- Winona — Fortree Gym local ID 1;
- Tate/Liza — Mossdeep Gym local ID 1;
- Juan — Sootopolis Gym local ID 1.

A wrong map/local-ID/script combination fails the R10 catalog audit.

## Region Map guidance

Every objective resolves a canonical Region Map marker from:

`vendor/vanillaplus/src/data/region_map/region_map_sections.json`

The portable objective exposes:

- map-section ID;
- marker x/y;
- marker width/height.

Example regression:

Route 103 resolves to:

- x = 4;
- y = 8;
- width = 4;
- height = 1.

No GPS route/pathfinding system was added.

## Portable API

R10 adds:

- `remaster_emerald_quest_objective_count()`;
- `remaster_emerald_quest_objective_at()`;
- `remaster_emerald_quest_objective_by_id()`;
- `remaster_emerald_quest_state()`;
- `remaster_emerald_quest_active()`;
- `remaster_emerald_quest_target_matches_map()`;
- `remaster_emerald_quest_active_region_marker()`.

Progress states are:

- locked;
- active;
- completed.

Conditions support the Emerald state primitives already used by the old
Phase 10A design:

- flag set;
- flag clear;
- var equal;
- var not equal;
- var greater/equal;
- var less-than.

The resolver reads state only. It does not set story flags or vars.

## Save compatibility

`r10_quest_save_roundtrip` verifies that:

1. an Emerald-compatible save is decoded;
2. its active objective is derived;
3. a real story flag is changed;
4. the derived objective advances;
5. the save is encoded through the existing Emerald writer;
6. it is decoded again;
7. the same new objective is derived.

R10 also regression-pins the existing save geometry:

- SaveBlock2: `0x0F44`;
- SaveBlock1: `0x3DC8`;
- Pokémon storage: `0x83D0`.

R10 adds no save-layout field.

## Full progression regression

`r10_quest_progression` synthetically exercises every one of the 32 declared
objective states using only each objective's real activation/completion
conditions.

It verifies:

- each catalog entry can become the canonical active objective;
- the resolver returns the first valid objective in story order;
- each objective returns its canonical Region Map marker;
- local targets match only their canonical map;
- region-only targets do not pretend to be local map targets;
- after the full declared completion chain / game-clear state, no normal
  main-story objective remains active.

## Unreal presentation boundary

The old Unreal navigation API allowed presentation code to call
`SetActiveObjective()`.

R10 removes that authority.

`URemasterNavigationSubsystem` now:

- reads the authoritative native Emerald save;
- calls `remaster_emerald_quest_active()`;
- converts the returned objective to presentation data;
- refreshes local map/object context after authoritative map changes;
- exposes `RefreshFromCore()` for presentation refreshes;
- never writes Emerald flags/vars.

The portable resolver is also compiled through the existing Unreal C++ embed
smoke path.

No licensed Unreal Engine compile/runtime claim is made by R10. That remains
deferred to R18.

## CI evidence

Dedicated R10 run:

`37263210755`

Result: **SUCCESS**

Passed steps:

- configure — PASS;
- build `emerald_quest_test` — PASS;
- build `emerald_quest_save_test` — PASS;
- build Unreal C++ embed smoke — PASS;
- R10 progression regression — PASS;
- R10 save round-trip regression — PASS;
- Unreal C++ embed smoke — PASS;
- deterministic catalog audit — PASS;
- pinned Vanilla+ regeneration byte comparison — PASS;
- Unreal presentation-boundary validation — PASS.

The CTest subset reported:

**3/3 tests passed.**

The catalog audit reported:

- objectives: **32**;
- object-event targets: **17**;
- generated catalog matches pinned Vanilla+.

Earlier-stage regressions on the same implementation head:

- R0 Core `37263210890` — SUCCESS;
- R4 Overworld `37263210850` — SUCCESS;
- R5 World Renderer `37263210820` — SUCCESS.

## R10 exit checklist

- [x] no second persisted story/quest state;
- [x] 32 main-story objectives;
- [x] deterministic locked/active/completed state;
- [x] one canonical active objective;
- [x] Region Map marker data;
- [x] map target support;
- [x] object-event target support;
- [x] coordinate target contract;
- [x] 17 local-object target script audits;
- [x] save round-trip stability;
- [x] game-clear produces no normal objective;
- [x] Unreal navigation is presentation-only;
- [x] portable C++ embed path passes;
- [x] R0/R4/R5 regressions green.

**R10 exit gate: PASSED.**

## Next phase

R11 — Pokémon / party / item core.

R11 will establish the authoritative Pokémon-domain representation required by
encounters, battle, UI and full save compatibility.
