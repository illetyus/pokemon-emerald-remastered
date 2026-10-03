# R2 — Script Engine Completion Evidence

Date: 2026-10-03  
Branch: `remaster/r2-script-engine`  
R1 base: `4128d6c12f2481266e3a62fd7ef5e5866374dc28` (`R1 COMPLETE`)  
Validated R2 head: `628954d744f8161c291e39803969b7051a775952`

## Status

**R2 implementation and acceptance validation are complete on the R2 branch.**

The portable core can now execute the pinned real Vanilla+ Littleroot / Route 101 opening script slice through generated semantic IR, typed host/domain requests, deterministic yield/resume, cross-script control flow and R1-compatible persistent state.

The acceptance suite executes both male and female opening paths and verifies that the resulting gameplay state survives the R1 128 KiB save encode/decode round trip.

## Authoritative Vanilla+ provenance

The real-source acceptance snapshot is pinned to:

`illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`

The commit is recorded in:

`tests/fixtures/r2/vanillaplus_source/VANILLAPLUS_COMMIT.txt`

The snapshot contains the source files required for the acceptance closure rather than a hand-authored replacement story.

Acceptance map/script roots:

1. `data/maps/LittlerootTown/scripts.inc`
2. `data/maps/LittlerootTown_BrendansHouse_1F/scripts.inc`
3. `data/maps/LittlerootTown_BrendansHouse_2F/scripts.inc`
4. `data/maps/LittlerootTown_MaysHouse_1F/scripts.inc`
5. `data/maps/LittlerootTown_MaysHouse_2F/scripts.inc`
6. `data/maps/LittlerootTown_ProfessorBirchsLab/scripts.inc`
7. `data/maps/Route101/scripts.inc`

Shared source dependencies include the pinned event macro table, script command table, special table, player-house scripts, shared movement scripts and the required constants.

## Acceptance entry labels

The generated real-source fixture starts from these real Vanilla+ script labels:

- `LittlerootTown_EventScript_StepOffTruckMale`
- `LittlerootTown_EventScript_StepOffTruckFemale`
- `LittlerootTown_BrendansHouse_1F_EventScript_EnterHouseMovingIn`
- `LittlerootTown_MaysHouse_1F_EventScript_EnterHouseMovingIn`
- `LittlerootTown_BrendansHouse_2F_EventScript_WallClock`
- `LittlerootTown_MaysHouse_2F_EventScript_WallClock`
- `PlayersHouse_1F_EventScript_PetalburgGymReportMale`
- `PlayersHouse_1F_EventScript_PetalburgGymReportFemale`
- `LittlerootTown_BrendansHouse_2F_EventScript_MeetBrendan`
- `LittlerootTown_MaysHouse_2F_EventScript_MeetMay`
- `Route101_EventScript_StartBirchRescue`
- `Route101_EventScript_BirchsBag`
- `LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent`

The generated native fixture at the validated head contains 40 script programs and exercises 46 distinct native script opcode kinds.

The pinned special manifest contains 526 unique special symbols. The acceptance harness binds the specials actually required by this slice, including:

- `HealPlayerParty`
- `TurnOffTVScreen`
- `StartWallClock`
- `Special_ViewWallClock`
- `ChooseStarter`
- `ChangePokemonNickname`

## Implemented R2 architecture

R2 now includes:

- authoritative command/special inventory;
- stable script/movement/text/special identities;
- Vanilla+ source -> semantic IR conversion;
- semantic IR -> portable native C fixture generation;
- cross-script call/goto and bounded call stack;
- persistent and special var/flag access;
- structured VM errors;
- typed host requests;
- deterministic yield/resume;
- stale/mismatched completion rejection;
- map-script scheduler hooks;
- object/coord/background script dispatch;
- scripted object state operations;
- scripted movement requests and wait semantics;
- message/choice/audio/fade/door/warp presentation requests;
- money/domain request boundary;
- generated special registry/adapters;
- versioned runtime checkpoint serialization;
- C++17 Unreal embed preflight;
- real-source Littleroot / Route 101 acceptance regression.

## Important compatibility fixes found during Task 10

The real-source acceptance test exposed and fixed two code-generation issues.

### Portable empty map-script storage

When the selected acceptance closure did not emit map-script table entries, the generator produced an ISO-C-invalid zero-length initializer.

Fixed behavior:

- emit one dummy storage element;
- expose `MapScriptCount = 0`;
- never rely on a zero-sized C array.

Regression test:

`test_empty_map_script_set_emits_portable_zero_count_storage`

### Vanilla flag boolean condition codes

Vanilla `checkflag` places a boolean `0/1` result in the script comparison state.

Therefore:

- `goto_if FALSE` uses condition code 0 / LESS in the Vanilla comparison table;
- `goto_if TRUE` uses condition code 1 / EQUAL.

The generator previously mapped these as `FALSE -> EQUAL` and `TRUE -> NOT_EQUAL`, which caused the wall-clock script to take the already-set branch when the clock flag was actually clear.

Fixed behavior is regression-pinned by:

`test_flag_boolean_conditions_preserve_vanilla_condition_codes`

This fix allowed the real wall-clock opening scene to continue through the expected `VAR_LITTLEROOT_INTRO_STATE = 6` transition.

## Real Littleroot / Route 101 acceptance coverage

`tests/r2_littleroot_integration_test.c` drives the generated real-source fixture with a deterministic test host.

Both player genders are exercised.

Verified progression includes:

1. gender-specific moving-truck exit;
2. moving-in house scene;
3. wall-clock flow;
4. Petalburg Gym television scene;
5. gender-specific rival introduction;
6. Route 101 Birch rescue start;
7. Birch bag / starter-selection handoff;
8. deterministic R2 domain/battle completion injection;
9. Birch laboratory starter handoff;
10. final persistent story-state validation.

Representative persistent checks include:

- `VAR_LITTLEROOT_INTRO_STATE`;
- `VAR_LITTLEROOT_RIVAL_STATE`;
- `VAR_LITTLEROOT_TOWN_STATE`;
- `VAR_ROUTE101_STATE`;
- `VAR_BIRCH_LAB_STATE`;
- gender-specific truck/rival flags;
- `FLAG_SET_WALL_CLOCK`;
- `FLAG_RESCUED_BIRCH`;
- `FLAG_SYS_POKEMON_GET`.

The acceptance test then encodes the resulting state with the R1 save writer, decodes it again and verifies the critical R2 persistent state survived unchanged.

## Executed verification evidence

GitHub Actions run:

`37145423793`

Validated head:

`628954d744f8161c291e39803969b7051a775952`

Result: **SUCCESS**

### portable-core

Completed successfully.

The CMake build compiled:

- portable C core;
- C++17 Unreal embed smoke target;
- R0/R1 regression targets;
- R2 VM/runtime/checkpoint/object tests;
- generated real Littleroot fixture;
- `r2_littleroot_integration_test`.

CTest result: **20/20 tests passed**.

This includes:

- `emerald_script_vm`
- `emerald_script_runtime`
- `emerald_script_checkpoint`
- `emerald_script_object`
- `r2_opening_commands`
- `r2_littleroot_integration`
- `r1_vanillaplus_integration`
- existing R0/R1 compatibility tests.

### world-converter

Completed successfully.

Executed:

- `tests/test_convert_world.py`
- `tests/test_convert_scripts.py`
- `tests/test_generate_script_c.py`
- `tests/test_r2_real_opening_source.py`
- real R2 opening native-fixture generation
- generated-content audit
- platform-coupling inventory
- Unreal source architecture validation.

The generated real opening C fixture was also uploaded as a CI artifact.

### scenario-schema

Completed successfully.

## Scope intentionally deferred

R2 does not claim full-Hoenn script coverage.

The following remain intentionally assigned to later roadmap phases:

- all-Hoenn world/script data completion — R3;
- free-roaming authoritative movement/collision — R4;
- full Unreal world presentation — R5+;
- complete item/party/Pokémon domain — R11;
- encounter system — R12;
- battle engine — R13;
- final UI/audio presentation — R9/R15.

Commands outside the R2 acceptance closure remain explicitly classified/deferred rather than silently treated as supported.

## R2 exit-gate result

- [x] real Vanilla+ source drives the acceptance fixture;
- [x] no native runtime GBA script-pointer execution;
- [x] cross-script deterministic VM works;
- [x] typed yield/resume works;
- [x] stale/mismatched host completions are rejected;
- [x] required object/movement command path works;
- [x] required presentation/domain adapter path works;
- [x] required specials are adapted;
- [x] runtime checkpointing is regression-tested;
- [x] male and female Littleroot opening paths execute;
- [x] Route 101 / Birch starter handoff executes;
- [x] resulting gameplay state remains R1 save-compatible;
- [x] portable C tests actually executed successfully;
- [x] converter/content/architecture tests actually executed successfully;
- [x] Unreal C++ embed preflight actually executed successfully.

**R2 exit gate: PASSED.**
