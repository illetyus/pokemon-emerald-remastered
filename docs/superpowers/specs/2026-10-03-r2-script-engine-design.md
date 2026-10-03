# R2 — Vanilla+ Script Engine Design

Date: 2026-10-03  
Branch: `remaster/r2-script-engine`  
Base: `R1 COMPLETE` (`4128d6c12f2481266e3a62fd7ef5e5866374dc28`)  
Authoritative gameplay source: `illetyus/pokezumrut-vanillaplus` at the R1-pinned baseline and later explicitly audited Vanilla+ updates.

## 1. Goal

R2 builds the portable event-script runtime that lets the remaster execute real Vanilla+ overworld scripts without depending on GBA ROM pointers, GBA callbacks, or Unreal-specific gameplay logic.

The first acceptance target is not all Hoenn. It is the real Littleroot/Route 101 opening flow, converted from Vanilla+ source scripts and executed by the native core with the same persistent flag/var/map/object mutations as Vanilla+.

R2 must preserve the R1 compatibility boundary:

- Vanilla+ remains the authoritative gameplay/content source.
- The portable C core owns authoritative script state and gameplay mutations.
- Unreal is a presentation/host layer, not the source of script truth.
- Existing 128 KiB Vanilla+ saves remain compatible.
- GBA `0x08xxxxxx` pointers are never executed by the remaster runtime.

## 2. Why source-to-IR instead of raw GBA bytecode

Three approaches were considered.

### A. Raw Emerald bytecode interpreter

Advantages:
- closest to original ROM execution;
- opcode numbers map directly to `gScriptCmdTable`.

Disadvantages:
- preserves ROM-address coupling;
- requires pointer relocation and ROM data-address ownership;
- encourages an emulator-shaped architecture rather than a portable gameplay core;
- makes generated content harder to inspect and validate.

### B. Manual script rewrites

Advantages:
- simple for a single scene;
- minimal parser/converter work.

Disadvantages:
- drifts from Vanilla+;
- does not scale to Hoenn;
- makes later Vanilla+ changes costly to port;
- weakens parity auditing.

### C. Vanilla+ source macros -> semantic IR -> native VM

Selected approach.

Advantages:
- keeps script identity symbolic and auditable;
- removes GBA pointer dependence;
- reuses the existing R1 native VM;
- scales into R3's all-Hoenn pipeline;
- allows unsupported commands to fail explicitly;
- lets async presentation actions yield without corrupting gameplay semantics.

The converter does not attempt to become a general GNU assembler. It parses the event-script/movement subset used by Vanilla+ data and resolves known macros/constants through an explicit symbol pipeline.

## 3. R2 scope

R2 includes:

- script command inventory and compatibility matrix;
- stable script, movement, text, special and map-script identities;
- conversion of real Vanilla+ script sources into portable IR;
- call/goto/conditional control flow across converted script labels;
- persistent vars/flags plus special vars/flags;
- map script dispatch for:
  - `MAP_SCRIPT_ON_LOAD`,
  - `MAP_SCRIPT_ON_TRANSITION`,
  - `MAP_SCRIPT_ON_FRAME_TABLE`,
  - `MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE`;
- object/background/coord interaction dispatch into script IDs;
- scripted object state changes;
- scripted movement queues and wait semantics;
- message/UI/audio/door/fade/warp requests through a host adapter;
- `special` and `specialvar` registry/adapters;
- item/money/Pokémon commands at the script interface boundary;
- deterministic yield/resume;
- serializable runtime/session checkpoint state;
- real Littleroot + Route 101 acceptance coverage.

R2 does not include:

- all-Hoenn data conversion completeness — R3;
- normal overworld collision/movement engine — R4;
- final Unreal world rendering — R5;
- final Android controls — R8;
- final UI implementation — R9;
- quest system — R10;
- complete party/item/Pokémon domain — R11;
- encounters — R12;
- battle engine — R13;
- final audio implementation — R15.

Commands that depend on later domains still receive stable R2 adapter contracts so scripts do not need to be redesigned later.

## 4. Authoritative Vanilla+ sources

The R2 inventory is derived from:

- `data/script_cmd_table.inc` — authoritative opcode table, currently `0x00..0xE7`;
- `asm/macros/event.inc` — semantic event macros and convenience condition macros;
- `data/specials.inc` — `special` registry order/identity;
- `src/script.c` — ScriptContext execution and map-script dispatch semantics;
- `src/scrcmd.c` — command behavior;
- `data/scripts/*.inc` — shared event scripts;
- `data/maps/*/scripts.inc` — map scripts and movement data.

For the first acceptance slice, dependency closure starts from:

- `data/maps/LittlerootTown/scripts.inc`;
- `LittlerootTown_BrendansHouse_1F`;
- `LittlerootTown_BrendansHouse_2F`;
- `LittlerootTown_MaysHouse_1F`;
- `LittlerootTown_MaysHouse_2F`;
- `LittlerootTown_ProfessorBirchsLab`;
- `Route101`;
- every shared script, movement, special and text reference transitively required by those scripts.

## 5. Component architecture

### 5.1 Script source inventory

A converter-side inventory step produces a compatibility report for every command encountered in the selected source closure.

Each command is classified as one of:

- `CORE` — pure deterministic VM/state operation;
- `WORLD` — authoritative world/object mutation;
- `PRESENTATION_YIELD` — asynchronous host/presentation action;
- `DOMAIN_ADAPTER` — item, Pokémon, money or other later-domain boundary;
- `SPECIAL_ADAPTER` — entry through Vanilla+'s special registry;
- `DEFERRED` — legal Vanilla+ command outside the current R2 acceptance slice.

An encountered command may never be silently dropped. Conversion fails if it has no explicit classification.

The inventory additionally records:
- original opcode number when applicable;
- macro spelling;
- operand schema;
- source locations;
- first acceptance phase that must implement it.

### 5.2 Stable generated identities

Generated content uses stable symbolic IDs, not host pointers.

Required generated identities:

- `script_id`;
- `movement_id`;
- `text_id`;
- `special_id`;
- `map_id`;
- map-script trigger IDs.

The stable key is the canonical Vanilla+ label/name. Generated numeric indices may exist for compact runtime arrays, but JSON/debug output preserves the symbolic identity.

### 5.3 Portable script IR

The current `RemasterEmeraldScriptInstruction` model is retained conceptually but expanded into a generated-program representation that can reference other script labels and host/domain resources.

The IR must represent at minimum:

- opcode/operation kind;
- 16-bit operands preserving Vanilla semantics;
- resolved literal/var operands;
- branch/call target script + instruction index;
- object local ID;
- map ID;
- movement ID;
- text ID;
- special ID;
- condition code;
- presentation/domain payload as typed fields rather than opaque pointers.

IR is deterministic: the same pinned Vanilla+ tree must generate byte-identical or semantically identical normalized output.

### 5.4 VM core

The VM remains portable C.

Responsibilities:
- program counter;
- call stack;
- comparison result;
- persistent var/flag access through R1;
- special vars/flags;
- instruction dispatch;
- step budget;
- yield state;
- structured error state.

The VM must support cross-script calls. A call-frame therefore stores script identity plus return PC, not only an integer PC into one flat program.

The VM must never:
- dereference GBA pointers;
- call Unreal APIs;
- mutate presentation state directly;
- invent defaults for unsupported commands.

### 5.5 Script runtime / scheduler

A separate runtime layer owns when scripts start and resume.

Responsibilities:
- start script by stable `script_id`;
- run immediate map scripts;
- start frame-table scripts when their var predicates match;
- dispatch object/coord/background interactions;
- resume after host completion;
- prevent conflicting authoritative scripts from running simultaneously unless Vanilla semantics explicitly allow it;
- expose current script/yield/error for diagnostics.

Map-script semantics follow Vanilla `src/script.c`:
- OnLoad / OnTransition execute immediately;
- OnFrame table compares `VarGet(var1) == VarGet(var2)` and starts the selected script;
- OnWarp table uses the same comparison-table semantics before immediate execution.

### 5.6 Host adapter

Asynchronous non-core work crosses a single host interface.

A host request contains:
- request type;
- stable script identity;
- instruction location;
- typed operands/payload;
- a deterministic request sequence number.

Initial request types include:

- message show/close/wait;
- yes/no or choice request;
- delay/timer;
- sound effect;
- fanfare;
- BGM action;
- screen fade;
- door open/close/wait;
- movement start/wait;
- warp transition;
- starter selection;
- nickname/other menu request where required by acceptance flow.

The VM yields after emitting a request that cannot complete synchronously. The host later supplies a completion/result and the runtime resumes the same instruction boundary according to command semantics.

Presentation completion must never directly set flags/vars. Only resumed core execution may perform subsequent authoritative mutations.

### 5.7 World/object adapter

Scripted object operations are authoritative gameplay operations, not visual-only commands.

R2 supports the Littleroot dependency closure for:

- add/remove/show/hide object;
- set object temporary/permanent coordinates;
- set movement type;
- turn object;
- face player;
- scripted movement assignment;
- movement completion tracking.

Permanent template mutations use the R1 object-template compatibility layer where Vanilla persists them.

R2 scripted movement is intentionally narrower than R4:
- movement commands execute ordered script-defined steps;
- they update authoritative object/player scripted position/facing;
- they do not implement the full free-roaming collision/elevation movement engine;
- movement behavior that requires R4-only physics is represented by a controlled adapter/result rather than duplicated prematurely.

### 5.8 Domain adapter

Script commands for domains not fully implemented until later phases use a typed interface.

Initial categories:

- item add/remove/check/space;
- money add/remove/check;
- Pokémon give/heal/party-size/query;
- starter selection;
- Pokédex/state actions where required.

Money can be implemented directly on R1 state because the encrypted currency contract already exists.

Item and Pokémon storage must not be partially reimplemented in R2. The adapter can use deterministic test doubles until R11/R13 replace them.

The adapter's result semantics must match Vanilla command outputs, particularly `VAR_RESULT` and comparison behavior.

### 5.9 Special registry

`special` and `specialvar` resolve by generated special identity derived from `data/specials.inc`.

Each special is classified as:
- pure core;
- world;
- domain;
- presentation/menu;
- later-phase deferred.

The first slice implements/adapts every special reachable from the Littleroot opening dependency closure.

Known early examples include:
- player gender/string helpers;
- PC-related effects used in the player's house;
- wall-clock flow where reachable;
- `ChooseStarter`;
- `HealPlayerParty`.

Unknown or unregistered specials are hard errors with source identity.

## 6. Command coverage strategy

R2 does not require implementing all 232 opcode-table slots before the first vertical slice.

Instead:

1. inventory the entire authoritative table;
2. build the transitive Littleroot/Route101 script closure;
3. implement every command actually reachable in that closure;
4. retain explicit `DEFERRED` metadata for the rest;
5. make the converter fail if new reachable commands are not classified.

This provides strict acceptance without pulling R11-R15 into R2.

The existing native VM commands remain valid starting points:
- nop/end/return;
- goto/call and conditional forms;
- set/copy/add/sub var;
- compare var/value and var/var;
- set/clear/check flag;
- waitstate;
- weather/map-layout state.

They are not treated as proven complete until compared again with authoritative `scrcmd.c` semantics.

## 7. Text and message handling

R2 preserves text by identity and source content but does not require final UI rendering.

Generated text records contain:
- text label;
- decoded/remaster-safe content representation;
- source reference.

Message commands yield a host request referencing `text_id` plus Vanilla message-box mode.

Tests may acknowledge messages immediately. Unreal later renders them through R9 UI without changing script semantics.

## 8. Map/event integration

R1 event definitions already retain script names in generated world data. R2 extends the generated output with stable resolved `script_id` values.

Integration flow:

1. R1/R3 world event resolver finds object/coord/background event.
2. Event returns a stable script identity.
3. Script runtime starts that script.
4. VM mutates save/world state or emits host requests.
5. Runtime resumes until halt/yield/error.

No Unreal subsystem is allowed to contain bespoke story progression for Littleroot.

## 9. Yield/resume model

Yield is explicit and typed.

Runtime states:

- `HALTED`;
- `RUNNING`;
- `YIELDED`;
- `ERROR`;
- `STEP_LIMIT`.

A yield stores:
- script ID;
- PC;
- call stack;
- comparison state;
- special vars/flags;
- pending request type + sequence;
- any command-specific continuation metadata.

A host completion must match the pending sequence/type. Stale, duplicate or mismatched completions are rejected.

`waitstate`, `waitmovement`, message waits, door waits, fades and menu/special flows use the same mechanism rather than custom per-platform loops.

## 10. Session checkpointing

R2 requires serializable runtime checkpoint state so Android lifecycle interruption does not require duplicating story logic.

Checkpoint data is application/session data, not an extension of the 128 KiB Emerald save.

It contains only remaster runtime continuation state:
- current script ID/PC;
- call stack;
- comparison;
- special vars/flags;
- pending yield;
- deterministic adapter continuation metadata.

Loading a plain Vanilla+ save does not invent an active script. Script runtime starts from map/event rules just as a normal field load would.

R1 save bytes remain untouched except for legitimate Vanilla-compatible gameplay mutations.

## 11. Error handling and diagnostics

Every conversion/runtime failure must include enough identity to reproduce it.

Converter errors include:
- source file;
- source line;
- script/movement label;
- macro/command;
- unresolved symbol or unsupported classification.

Runtime errors include:
- script ID;
- instruction PC;
- operation;
- relevant operands;
- call-stack trace;
- pending host request if any.

Step-limit exhaustion is distinct from malformed/unsupported instruction failure.

No unsupported command is treated as NOP unless Vanilla itself defines that opcode as NOP.

## 12. Testing strategy

### 12.1 Converter tests

Golden/unit tests cover:
- labels and cross-script call/goto resolution;
- convenience macros such as `goto_if_eq` / `call_if_unset`;
- map-script tables;
- movement arrays;
- text/special identities;
- deterministic output;
- unknown reachable command rejection;
- dependency-closure collection.

### 12.2 VM unit tests

Tests pin:
- Vanilla compare-condition table;
- literal vs var semantics;
- persistent/special vars;
- persistent/special flags;
- cross-script call stack;
- stack overflow;
- invalid target;
- step limit;
- yield/resume sequencing;
- stale completion rejection.

### 12.3 Runtime/map-script tests

Tests pin:
- OnLoad;
- OnTransition;
- OnFrame table;
- OnWarp table;
- event -> script dispatch;
- immediate vs yielded script behavior;
- only authoritative core changes flags/vars.

### 12.4 Object/movement tests

Tests cover:
- object add/remove/show/hide;
- permanent coordinate/movement-type changes;
- ordered scripted movement;
- waitmovement yield/resume;
- player/NPC target identity.

### 12.5 Adapter tests

Fake adapters pin command semantics for:
- messages/choices;
- money;
- item results;
- Pokémon/starter operations;
- specials.

### 12.6 Littleroot acceptance regression

The acceptance test consumes generated IR from real Vanilla+ sources, not a hand-authored replacement script.

Both male and female opening branches are covered where they diverge.

Required progression coverage:

1. Littleroot map transition setup;
2. moving-truck exit;
3. player's house moving-in scene;
4. clock/progression handoff;
5. rival house/bedroom progression;
6. exit toward Route 101;
7. Birch rescue scene;
8. starter-selection handoff;
9. deterministic battle/domain result injection for R2 testing;
10. Birch laboratory return/progression.

At defined checkpoints, compare the relevant:
- vars;
- flags;
- map identity;
- player/object coordinates;
- object-template state;
- runtime/yield state.

The final gameplay save must still pass the R1 save encoder/decoder round trip.

## 13. Initial file boundaries

Expected portable-core files:

- `core/include/remaster/emerald_script.h` — core VM public types;
- `core/src/emerald_script.c` — deterministic instruction execution;
- `core/include/remaster/emerald_script_runtime.h`;
- `core/src/emerald_script_runtime.c` — lifecycle/map/event scheduler;
- `core/include/remaster/emerald_script_host.h` — host/domain request contracts;
- `core/src/emerald_script_host.c` only if common helper logic is required;
- focused world/domain adapter files only when a responsibility cannot stay cleanly in existing R1 modules.

Converter/tooling:

- `tools/convert_scripts.py`;
- shared constant/symbol resolver helpers, factored from `convert_world.py` only where this prevents duplicated semantics;
- generated script manifest/IR under the existing generated-content build area.

Tests:

- expand `tests/emerald_script_test.c`;
- add focused runtime/host/converter tests;
- add an R2 Littleroot integration regression.

Avoid a monolithic script file. VM execution, runtime scheduling and host contracts remain separate responsibilities.

## 14. Delivery sequence

R2 implementation should proceed in these reviewable increments:

1. R2.0 command/special inventory + compatibility report;
2. R2.1 source-to-IR converter and stable IDs;
3. R2.2 cross-script VM core;
4. R2.3 host request + generic yield/resume;
5. R2.4 map-script/event scheduler;
6. R2.5 object/scripted movement commands;
7. R2.6 presentation/domain command adapters;
8. R2.7 special registry and Littleroot-required specials;
9. R2.8 session checkpoint serialization;
10. R2.9 real Littleroot/Route101 acceptance regression.

Each increment uses TDD where executable behavior is added and ends in its own commit.

## 15. R2 exit gate

R2 is complete only when all of the following are true:

- real Vanilla+ Littleroot/Route101 scripts are converted automatically from source;
- no GBA script pointer is dereferenced by native runtime;
- required map-script lifecycle hooks behave like Vanilla;
- object/coord/background interactions dispatch into native scripts;
- required scripted object/movement operations execute deterministically;
- async operations yield and resume through the host contract;
- every reachable special/command in the acceptance closure is implemented or adapted;
- unsupported reachable commands fail explicitly;
- relevant persistent flag/var/map/object mutations match Vanilla+ expectations;
- both gender-dependent opening paths are covered where behavior differs;
- runtime checkpoint serialize/restore is deterministic;
- the resulting 128 KiB gameplay save remains R1-compatible;
- the full R2 regression suite executes successfully in a functioning test environment.

GitHub Actions runner availability remains infrastructure evidence, not a gameplay-semantic requirement. A runner failure before workflow steps does not count as an R2 code-test failure, but a successful executable test run is still required before R2 can be formally closed.
