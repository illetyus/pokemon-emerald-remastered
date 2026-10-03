# R2 Script Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Execute real Vanilla+ Littleroot/Route101 overworld scripts in the portable C core through generated semantic IR, deterministic yield/resume, map/event scheduling, and host/domain adapters while preserving R1 save compatibility.

**Architecture:** Vanilla+ event-script source is converted into stable symbolic IR by Python tooling. The portable C VM executes deterministic gameplay operations and yields typed requests for asynchronous presentation/domain work; a separate runtime layer owns map-script lifecycle, interaction dispatch, and resume semantics. Unreal never owns story state and no GBA ROM pointer is executed.

**Tech Stack:** C99 portable core, C++17 Unreal embed preflight, Python 3 standard library/unittest converter tooling, CMake/CTest, JSON generated IR.

**Spec:** `docs/superpowers/specs/2026-10-03-r2-script-engine-design.md`

## Global Constraints

- Vanilla+ remains the authoritative gameplay/content source.
- Base branch is `main` at/after `R1 COMPLETE`; development branch is `remaster/r2-script-engine`.
- Preserve 128 KiB Vanilla+ save compatibility; do not append R2 runtime state to the Emerald save image.
- Never dereference or execute GBA `0x08xxxxxx` pointers.
- Portable core must not call Unreal APIs.
- Unsupported reachable commands/specials are explicit conversion/runtime errors, never silent NOPs.
- Full all-Hoenn script completeness is R3; full free-roaming movement is R4; complete item/Pokémon domain is R11; encounters/battle remain R12/R13.
- Generated identities retain canonical symbolic labels for scripts, movements, text, specials and maps.
- New executable behavior follows TDD and each task ends with an independent commit.
- Formal R2 closure requires an actually executed green test suite; GitHub Actions `steps=[]` runner failures do not count as code-test failures.

## Review Focus

1. **Reachable-but-unclassified command** — converter must fail with source file, line, label and macro instead of generating partial IR. Covered in Task 1/2 tests.
2. **Cross-script recursion/stack exhaustion** — VM must stop with structured error without corrupting save state. Covered in Task 3 tests.
3. **Stale or duplicate host completion** — runtime must reject wrong request sequence/type and remain at the pending yield. Covered in Task 4 tests.
4. **Map lifecycle re-entry** — OnFrame/OnWarp scheduling must not start conflicting scripts while another authoritative script is active. Covered in Task 5 tests.
5. **Checkpoint with pending async request** — serialize/restore must preserve the exact script/PC/stack/request sequence and reject mismatched completion after restore. Covered in Task 9 tests.

---

## File Structure

### Converter/tooling
- Create: `tools/convert_scripts.py` — Vanilla+ event-script inventory, dependency closure and semantic IR generation.
- Create: `tools/script_ir.py` — focused parser/IR helpers and stable identity normalization.
- Create: `tests/test_convert_scripts.py` — converter/inventory/macro/dependency tests.
- Modify: `tools/convert_world.py` — attach resolved `script_id` to world events without removing symbolic `script`.
- Modify: `tests/test_convert_world.py` — verify world-to-script identity linkage.

### Portable core
- Modify: `core/include/remaster/emerald_script.h` — program registry, cross-script call frames, structured errors.
- Modify: `core/src/emerald_script.c` — deterministic VM execution only.
- Create: `core/include/remaster/emerald_script_host.h` — typed host/domain request/completion contracts.
- Create: `core/include/remaster/emerald_script_runtime.h` — scheduler, current script, yield/resume, map/event dispatch.
- Create: `core/src/emerald_script_runtime.c` — lifecycle and host orchestration.
- Create only if needed: `core/src/emerald_script_host.c` — shared request helpers, no platform implementation.
- Modify: `core/include/remaster/emerald_object_state.h`, `core/src/emerald_object_state.c` — only the permanent object/template operations required by scripts.
- Modify: `CMakeLists.txt` — new runtime sources/tests and Unreal embed smoke coverage.
- Create: `unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldScriptRuntimeEmbed.cpp` — C runtime embed preflight.

### Tests
- Expand: `tests/emerald_script_test.c` — VM semantics and cross-script execution.
- Create: `tests/emerald_script_runtime_test.c` — scheduler/yield/resume/host sequencing.
- Create: `tests/emerald_script_object_test.c` — object/scripted-movement behavior.
- Create: `tests/emerald_script_checkpoint_test.c` — checkpoint round-trip.
- Create: `tests/r2_littleroot_integration_test.c` — generated real-source acceptance runtime.
- Create: `tests/fixtures/r2/` generated or checked-in minimal normalized IR fixtures only where deterministic test isolation requires them; do not hand-author substitute Littleroot story logic.

---

### Task 1: Command/Special Inventory and Compatibility Matrix

**Files:**
- Create: `tools/script_ir.py`
- Create: `tools/convert_scripts.py`
- Create: `tests/test_convert_scripts.py`

**Interfaces:**
- Consumes: Vanilla+ `data/script_cmd_table.inc`, `asm/macros/event.inc`, `data/specials.inc`, selected map/shared script roots.
- Produces:
  - Python `build_command_inventory(source_root: Path) -> dict[str, CommandSpec]`
  - Python `build_special_inventory(source_root: Path) -> dict[str, SpecialSpec]`
  - Python `collect_script_dependency_closure(source_root: Path, roots: list[Path]) -> ScriptClosure`
  - normalized JSON compatibility report with classification `CORE|WORLD|PRESENTATION_YIELD|DOMAIN_ADAPTER|SPECIAL_ADAPTER|DEFERRED`.

- [ ] **Step 1: Write failing inventory tests**

Add tests that assert:
- opcode table contains slots `0x00..0xE7`;
- known mappings include `setvar -> 0x16`, `special -> 0x25`, `applymovement -> 0x4F`, `waitmovement -> 0x51`, `checkplayergender -> 0xA0`;
- `ChooseStarter` and `HealPlayerParty` resolve from `data/specials.inc`;
- Littleroot/Route101 root files produce a closure containing referenced shared labels;
- an encountered macro missing classification raises `ScriptConversionError` with file, line, label and command.

Run:
`python -m unittest tests/test_convert_scripts.py -v`

Expected: FAIL because inventory/parser APIs do not exist.

- [ ] **Step 2: Implement inventory/parser foundations**

Implement the exact APIs above in `tools/script_ir.py` / `tools/convert_scripts.py`. Keep macro parsing line-oriented and explicit; do not attempt a general assembler.

- [ ] **Step 3: Run inventory tests**

Run:
`python -m unittest tests/test_convert_scripts.py -v`

Expected: PASS.

- [ ] **Step 4: Generate a real Littleroot compatibility report locally when Vanilla+ source is available**

Run:
`python tools/convert_scripts.py <vanillaplus-root> build/generated-scripts --roots LittlerootTown Route101 --inventory-only`

Expected:
- deterministic JSON;
- no unclassified reachable command;
- deferred commands allowed only when unreachable from the selected acceptance closure.

- [ ] **Step 5: Commit**

Commit message:
`feat: inventory Vanilla+ script commands and specials`

---

### Task 2: Semantic Script IR, Stable IDs, and World Linkage

**Files:**
- Modify: `tools/script_ir.py`
- Modify: `tools/convert_scripts.py`
- Modify: `tests/test_convert_scripts.py`
- Modify: `tools/convert_world.py`
- Modify: `tests/test_convert_world.py`

**Interfaces:**
- Consumes: Task 1 inventories/closure.
- Produces:
  - `convert_script_closure(source_root: Path, roots: list[Path]) -> dict`
  - stable IDs equal to canonical labels for `script_id`, `movement_id`, `text_id`, `special_id`;
  - generated program records with operation, typed operands, source location and resolved branch/call target;
  - world event JSON containing both original `script` and resolved `script_id`.

- [ ] **Step 1: Write failing IR tests**

Pin:
- labels/cross-script call targets resolve by canonical label;
- convenience macros `goto_if_eq`, `call_if_unset`, `call_if_lt` normalize to compare/check + conditional branch/call semantics;
- `map_script` and `map_script_2` tables are emitted separately from executable instruction streams;
- movement arrays end at `step_end`;
- text labels remain symbolic;
- identical input produces identical normalized JSON;
- unresolved label/symbol is an error, not a null target.

Run:
`python -m unittest tests/test_convert_scripts.py -v`

Expected: FAIL on missing conversion behavior.

- [ ] **Step 2: Implement semantic IR conversion**

Use typed JSON fields; do not store host/GBA addresses. Preserve numeric Vanilla operands where known and symbolic names alongside them.

- [ ] **Step 3: Add world linkage**

Update `convert_world.py` so object/coord/background events with `script` also carry `script_id` when present in the generated script manifest. Do not remove legacy symbolic `script`.

- [ ] **Step 4: Verify Python converter suites**

Run:
`python -m unittest tests/test_convert_scripts.py tests/test_convert_world.py -v`

Expected: PASS.

- [ ] **Step 5: Commit**

Commit message:
`feat: convert Vanilla+ event scripts to portable IR`

---

### Task 3: Cross-Script VM Program Registry and Structured Errors

**Files:**
- Modify: `core/include/remaster/emerald_script.h`
- Modify: `core/src/emerald_script.c`
- Modify: `tests/emerald_script_test.c`

**Interfaces:**
- Consumes: generated script programs from Task 2.
- Produces:
  - `RemasterEmeraldScriptProgram` with stable `script_id`, instruction pointer/count;
  - `RemasterEmeraldScriptRegistry`;
  - call frame `{ program_index, return_pc }`;
  - `remaster_emerald_script_init_program(..., const RemasterEmeraldScriptRegistry *, uint32_t program_index, uint32_t entry_pc)`;
  - `remaster_emerald_script_error_get(const RemasterEmeraldScriptVm *, RemasterEmeraldScriptError *)`;
  - branch/call targets expressed as program index + PC, never pointer.

- [ ] **Step 1: Write failing VM tests**

Add cases for:
- call from script A into script B and return;
- conditional call across scripts;
- invalid target program/PC;
- recursive stack overflow at exactly `REMASTER_EMERALD_SCRIPT_STACK_DEPTH`;
- step-limit leaves save bytes unchanged except operations completed before the limit;
- structured error contains current script ID, PC and opcode;
- existing single-program R1-era tests continue to pass through a compatibility initializer or are migrated explicitly.

Run:
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release && ctest --test-dir build -C Release -R emerald_script_vm --output-on-failure`

Expected: FAIL on new cross-script cases.

- [ ] **Step 2: Implement program registry/call frames/error state**

Keep deterministic state mutation in `emerald_script.c`; do not introduce host/runtime behavior yet.

- [ ] **Step 3: Run VM regression**

Run the command from Step 1.

Expected: PASS.

- [ ] **Step 4: Commit**

Commit message:
`feat: support cross-script native VM execution`

---

### Task 4: Typed Host Requests and Generic Yield/Resume

**Files:**
- Create: `core/include/remaster/emerald_script_host.h`
- Create: `core/include/remaster/emerald_script_runtime.h`
- Create: `core/src/emerald_script_runtime.c`
- Create if needed: `core/src/emerald_script_host.c`
- Create: `tests/emerald_script_runtime_test.c`
- Modify: `CMakeLists.txt`
- Create: `unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldScriptRuntimeEmbed.cpp`
- Modify: `tests/unreal_cpp_embed_smoke.cpp`

**Interfaces:**
- Produces:
  - `RemasterEmeraldScriptRequestType` enum;
  - `RemasterEmeraldScriptRequest { type, sequence, program_index, pc, ...typed payload... }`;
  - `RemasterEmeraldScriptCompletion { type, sequence, result_u16, accepted }`;
  - `remaster_emerald_script_runtime_start(...)`;
  - `remaster_emerald_script_runtime_run(..., size_t max_steps)`;
  - `remaster_emerald_script_runtime_pending_request(...)`;
  - `remaster_emerald_script_runtime_complete(..., const RemasterEmeraldScriptCompletion *)`.

- [ ] **Step 1: Write failing runtime tests**

Pin:
- `WAIT_STATE` becomes a typed yield request;
- sequence increments monotonically;
- matching completion resumes the exact next continuation point;
- duplicate completion rejected;
- stale sequence rejected;
- wrong request type rejected;
- rejection does not mutate PC/pending request;
- only resumed VM instructions mutate flags/vars.

Run:
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release && ctest --test-dir build -C Release -R emerald_script_runtime --output-on-failure`

Expected: FAIL because runtime API does not exist.

- [ ] **Step 2: Implement generic runtime/yield contract**

Do not add platform implementations. Runtime owns sequence validation and continuation.

- [ ] **Step 3: Wire CMake and Unreal embed preflight**

Ensure runtime C files compile both as C99 library sources and inside the C++17 Unreal embed smoke target.

- [ ] **Step 4: Run runtime + embed tests**

Run:
`ctest --test-dir build -C Release -R "emerald_script_runtime|unreal_cpp_embed" --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit**

Commit message:
`feat: add deterministic script yield resume runtime`

---

### Task 5: Map-Script Scheduler and Event-to-Script Dispatch

**Files:**
- Modify: `core/include/remaster/emerald_script_runtime.h`
- Modify: `core/src/emerald_script_runtime.c`
- Modify: `core/include/remaster/emerald_events.h` only if stable script identity fields are missing from portable event defs.
- Modify: `core/src/emerald_events.c` only as needed for dispatch identity.
- Modify: `tests/emerald_script_runtime_test.c`

**Interfaces:**
- Consumes: generated map-script tables and R1 event matches.
- Produces:
  - `remaster_emerald_script_runtime_run_map_hook(runtime, REMASTER_MAP_SCRIPT_ON_LOAD|...)`;
  - `remaster_emerald_script_runtime_try_frame_table(runtime, ...)`;
  - `remaster_emerald_script_runtime_dispatch_script_id(runtime, script_id)`;
  - `remaster_emerald_script_runtime_dispatch_object/coord/background(...)`.

- [ ] **Step 1: Write failing lifecycle tests**

Pin Vanilla `src/script.c` semantics:
- OnLoad immediate execution;
- OnTransition immediate execution;
- OnFrame table selects first row where `VarGet(var1) == VarGet(var2)`;
- OnWarp table uses the same comparison-table lookup and immediate execution;
- no match means no script;
- attempting a second authoritative script while runtime is already running/yielded returns busy/conflict without replacing current state.

Run:
`ctest --test-dir build -C Release -R emerald_script_runtime --output-on-failure`

Expected: FAIL.

- [ ] **Step 2: Implement scheduler/dispatch**

Keep hook ordering explicit and testable. Do not add Unreal story logic.

- [ ] **Step 3: Add event-dispatch tests**

Use R1 object/coord/background definitions that resolve to stable script IDs and assert the runtime starts that exact program.

- [ ] **Step 4: Verify**

Run:
`ctest --test-dir build -C Release -R "emerald_script_runtime|emerald_map_events" --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit**

Commit message:
`feat: dispatch Vanilla map and interaction scripts`

---

### Task 6: Scripted Object State and Movement Requests

**Files:**
- Modify: `core/include/remaster/emerald_script.h`
- Modify: `core/src/emerald_script.c`
- Modify: `core/include/remaster/emerald_script_host.h`
- Modify: `core/src/emerald_script_runtime.c`
- Modify: `core/include/remaster/emerald_object_state.h`
- Modify: `core/src/emerald_object_state.c`
- Create: `tests/emerald_script_object_test.c`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Add operations for:
  - add/remove/show/hide object;
  - `setobjectxy`;
  - `setobjectxyperm`;
  - `setobjectmovementtype`;
  - `turnobject` / `faceplayer`;
  - `applymovement`;
  - `waitmovement`.
- Scripted movement host request identifies `local_id`, `map_id`, `movement_id`, request sequence.
- Permanent coordinate/movement changes use R1 object-template storage APIs.

- [ ] **Step 1: Write failing object/movement tests**

Pin:
- permanent template coordinate change survives save encode/decode;
- movement-type mutation changes only target object;
- applymovement yields with correct object/movement IDs;
- waitmovement does not resume until matching completion;
- player pseudo-object ID is distinguished from local NPC IDs;
- movement completion for the wrong local ID/sequence is rejected.

Run:
`ctest --test-dir build -C Release -R emerald_script_object --output-on-failure`

Expected: FAIL.

- [ ] **Step 2: Implement authoritative object mutations**

Use existing R1 save/object APIs; avoid duplicating persistence.

- [ ] **Step 3: Implement movement request/wait semantics**

Movement path execution stays host/world-adapter scoped for R2; do not implement R4 collision.

- [ ] **Step 4: Verify object + R1 save regressions**

Run:
`ctest --test-dir build -C Release -R "emerald_script_object|emerald_object_save_state|r1_vanillaplus_integration" --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit**

Commit message:
`feat: execute scripted object and movement commands`

---

### Task 7: Presentation and Domain Command Adapters

**Files:**
- Modify: `core/include/remaster/emerald_script.h`
- Modify: `core/src/emerald_script.c`
- Modify: `core/include/remaster/emerald_script_host.h`
- Modify: `core/src/emerald_script_runtime.c`
- Modify: `tests/emerald_script_runtime_test.c`

**Interfaces:**
- Presentation requests for:
  - message/show/close/wait;
  - yes/no or choice result;
  - delay;
  - play/wait SE;
  - play/wait fanfare;
  - BGM actions;
  - fade;
  - door open/close/wait;
  - warp.
- Domain requests/results for:
  - item add/remove/check/space;
  - money add/remove/check;
  - Pokémon give/heal/party query;
  - starter selection.
- Completion result writes `VAR_RESULT` or comparison state only where Vanilla command semantics require it.

- [ ] **Step 1: Write failing adapter tests**

Cover:
- msgbox yields text ID + box mode and resumes without mutating state;
- yes/no completion writes the expected `VAR_RESULT`;
- delay/fade/door waits use typed requests;
- money add/remove/check uses R1 encrypted money state synchronously;
- item/Pokémon operations yield to domain adapter and map result into `VAR_RESULT`;
- warp request does not directly mutate presentation; authoritative map transition happens only through runtime/core transition completion path.

Run:
`ctest --test-dir build -C Release -R emerald_script_runtime --output-on-failure`

Expected: FAIL.

- [ ] **Step 2: Implement presentation request opcodes**

Keep payload types explicit; no generic void pointer.

- [ ] **Step 3: Implement domain boundary and direct money semantics**

Do not implement full bag/party storage.

- [ ] **Step 4: Verify runtime regressions**

Run:
`ctest --test-dir build -C Release -R "emerald_script_runtime|emerald_overworld_state|emerald_warp_transition" --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit**

Commit message:
`feat: add script presentation and domain adapters`

---

### Task 8: Generated Special Registry and Littleroot Specials

**Files:**
- Modify: `tools/convert_scripts.py`
- Modify: `tests/test_convert_scripts.py`
- Modify: `core/include/remaster/emerald_script_host.h`
- Modify: `core/include/remaster/emerald_script_runtime.h`
- Modify: `core/src/emerald_script_runtime.c`
- Modify: `tests/emerald_script_runtime_test.c`

**Interfaces:**
- Converter emits ordered special manifest from `data/specials.inc`.
- Runtime resolves `special_id` by generated identity.
- Add `SPECIAL` and `SPECIAL_VAR` execution paths.
- Every special in Littleroot acceptance closure is classified and implemented/adapted.
- At minimum support/adapt reachable helpers including player-gender/string helpers, PC/clock helpers where closure requires them, `ChooseStarter`, `HealPlayerParty`.

- [ ] **Step 1: Write failing special-registry tests**

Pin:
- special numeric order is deterministic from source;
- unknown special is conversion/runtime error;
- `specialvar VAR_RESULT, X` writes adapter result;
- `ChooseStarter` yields `STARTER_SELECTION`;
- `HealPlayerParty` calls the domain adapter but does not introduce R11/R13 storage logic;
- player gender helper result matches seeded player data/adapter contract.

Run Python and C tests:
`python -m unittest tests/test_convert_scripts.py -v`
`ctest --test-dir build -C Release -R emerald_script_runtime --output-on-failure`

Expected: FAIL before implementation.

- [ ] **Step 2: Implement generated special manifest and runtime dispatch**

Prefer table-driven registration over switch statements spread across files.

- [ ] **Step 3: Verify both suites**

Expected: PASS.

- [ ] **Step 4: Commit**

Commit message:
`feat: dispatch Vanilla+ script specials through adapters`

---

### Task 9: Runtime Checkpoint Serialization

**Files:**
- Modify: `core/include/remaster/emerald_script_runtime.h`
- Modify: `core/src/emerald_script_runtime.c`
- Create: `tests/emerald_script_checkpoint_test.c`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produce:
  - fixed-version `RemasterEmeraldScriptCheckpoint` logical schema;
  - `remaster_emerald_script_runtime_checkpoint_size(void)`;
  - `remaster_emerald_script_runtime_checkpoint_write(runtime, void *dst, size_t dst_size)`;
  - `remaster_emerald_script_runtime_checkpoint_read(runtime, registry, save, const void *src, size_t src_size)`.
- Checkpoint contains runtime continuation only; never appended to Emerald save image.

- [ ] **Step 1: Write failing checkpoint tests**

Pin:
- running state round-trip preserves program/PC/stack/comparison/special vars/flags;
- yielded state preserves request type/sequence/payload;
- restored pending request accepts exactly the matching completion;
- truncated/unknown-version checkpoint rejected without mutating destination runtime;
- plain R1 save load starts with no active script checkpoint unless a separate checkpoint is explicitly supplied.

Run:
`ctest --test-dir build -C Release -R emerald_script_checkpoint --output-on-failure`

Expected: FAIL.

- [ ] **Step 2: Implement versioned checkpoint codec**

Use explicit endian-stable fields; do not memcpy host structs with padding/pointers.

- [ ] **Step 3: Verify checkpoint + save isolation**

Run:
`ctest --test-dir build -C Release -R "emerald_script_checkpoint|emerald_save_compatibility|r1_vanillaplus_integration" --output-on-failure`

Expected: PASS.

- [ ] **Step 4: Commit**

Commit message:
`feat: serialize native script runtime checkpoints`

---

### Task 10: Real Littleroot/Route101 Acceptance Slice and R2 Gate

**Files:**
- Modify: `tools/convert_scripts.py`
- Modify: `tools/convert_world.py` only if final generated linkage needs adjustment.
- Create: `tests/r2_littleroot_integration_test.c`
- Create/update deterministic generated acceptance fixtures produced from the authoritative Vanilla+ source; retain source provenance metadata.
- Modify: `CMakeLists.txt`
- Modify: `.github/workflows/r0-core.yml` to add script-converter Python tests if not already covered.
- Create: `docs/R2_SCRIPT_ENGINE.md`

**Interfaces:**
- Consumes all Tasks 1–9.
- Produces one deterministic acceptance harness able to step real generated scripts with a fake host/domain adapter.
- Test harness exposes host completions for messages, movement, clock/menu, starter selection and battle/domain outcomes without embedding alternate story logic.

- [ ] **Step 1: Generate the real Littleroot/Route101 dependency closure from pinned Vanilla+ source**

Required roots:
- LittlerootTown;
- both player/rival house gender variants;
- Professor Birch's Lab;
- Route101;
- all transitively referenced shared scripts/movements/specials/text.

Assert generation contains no unclassified reachable command/special.

- [ ] **Step 2: Write failing male opening acceptance path**

Check defined milestones:
1. Littleroot transition setup;
2. truck exit request/movement;
3. player's house moving-in progression;
4. clock/progression handoff;
5. rival introduction progression;
6. Route101 Birch rescue;
7. starter-selection yield;
8. injected deterministic R2 battle/domain completion;
9. lab return/progression.

At each selected checkpoint assert exact relevant vars/flags/map/object coordinates from generated Vanilla+ semantics.

- [ ] **Step 3: Write failing female divergence acceptance path**

Cover gender-specific house/truck/rival branches and assert convergence on the same expected later story state where Vanilla does.

- [ ] **Step 4: Implement remaining reachable command/special adapters discovered by the real closure**

Do not broaden into unrelated Hoenn commands. Any new reachable command must first receive classification and a focused unit test.

- [ ] **Step 5: Verify R1 save round trip after R2 progression**

Encode the resulting gameplay state with R1 save writer, decode it again and assert relevant R2 persistent vars/flags/map/object state survives unchanged.

- [ ] **Step 6: Run full portable verification**

Run:
`python -m unittest tests/test_convert_scripts.py tests/test_convert_world.py tests/test_content_audit.py tests/test_platform_coupling.py -v`

Then:
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
`cmake --build build --config Release`
`ctest --test-dir build -C Release --output-on-failure`

Expected: all executed tests PASS, including:
- existing R0/R1 tests;
- converter tests;
- VM/runtime/object/checkpoint tests;
- `r2_littleroot_integration`;
- Unreal C++ embed preflight.

- [ ] **Step 7: Run source-architecture validation**

Run:
`python tools/validate_unreal_source.py`

Expected: PASS and no gameplay-story implementation introduced into Unreal presentation classes.

- [ ] **Step 8: Document R2 evidence**

In `docs/R2_SCRIPT_ENGINE.md`, record:
- authoritative Vanilla+ source commit used for acceptance fixture generation;
- generated command/special coverage counts;
- exact accepted Littleroot roots;
- test commands/results;
- deferred command categories;
- confirmation that R1 save compatibility remains intact;
- CI runner status separately from code-test evidence.

- [ ] **Step 9: Commit**

Commit message:
`test: validate real Vanilla+ Littleroot script flow`

---

## Final R2 Closure Checklist

Before marking R2 complete:

- [ ] Converter consumes real Vanilla+ scripts, not hand-authored replacement story code.
- [ ] Stable symbolic IDs exist for scripts, movements, text, specials and map hooks.
- [ ] Every reachable acceptance command/special has explicit classification.
- [ ] No runtime GBA pointer dereference exists.
- [ ] Cross-script call/goto stack is deterministic and bounded.
- [ ] Host yield/resume rejects stale, duplicate and mismatched completions.
- [ ] OnLoad/OnTransition/OnFrame/OnWarp semantics match audited Vanilla behavior.
- [ ] R1 object/coord/background event resolution dispatches stable native script IDs.
- [ ] Scripted object/movement commands work without prematurely implementing R4 free movement.
- [ ] Presentation/domain operations cross typed adapter boundaries.
- [ ] Session checkpoint survives pending-yield round trip and remains separate from Emerald save bytes.
- [ ] Male/female real Littleroot acceptance paths pass.
- [ ] Resulting gameplay save passes R1 encode/decode round trip.
- [ ] Full Python + CMake/CTest + Unreal embed verification has actually executed successfully.
- [ ] R2 evidence document is committed.
