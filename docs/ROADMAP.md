# Pokémon Emerald Remastered — Canonical Roadmap

Date: 2026-10-10
Status: **R0-R5, R10-R13, R16/R17 and R6/R7/R9/R14/R15/R8/R19 pre-real-UE source phases verified complete on main; R20 production polish active. Actual engine/assets/device obligations remain open.**

This document is the single authoritative development roadmap for the project.
Older R0-R5 plans and evidence documents remain historical records, but they do
not define the current phase order.

## 1. Product definition

Pokémon Emerald Remastered preserves Pokémon Emerald/Vanilla+ gameplay truth
while replacing presentation and platform layers with modern equivalents.

The project is **not** a reimplementation of Emerald rules inside Unreal.

The production model is:

```text
Emerald / pinned Vanilla+ source truth
        ↓
portable deterministic C/C++ gameplay core
        ↓
explicit state snapshots + presentation events
        ↓
Unreal Engine presentation / UI / audio / input
        ↓
Android
```

Gameplay behavior is preserved unless a change is explicitly accepted as a
Vanilla+ QoL change.

## 2. Non-negotiable architecture rules

1. The portable core owns gameplay authority.
2. Unreal is a presentation, input, audio and platform host.
3. Movement, collision, scripts, warps, connections, flags, vars, encounters,
   Pokémon state, items, battle rules and save-domain truth do not move into
   Unreal.
4. Given the same initial state, data, RNG seed and ordered inputs, the gameplay
   state sequence must be deterministic.
5. Presentation may consume state and events; it must not invent gameplay
   outcomes.
6. Emerald/Vanilla+ source behavior is the primary specification. Secondary
   wikis are reference material only.
7. Regression tests are written before or together with replacement behavior.
8. No phase is complete until its acceptance tests actually run and pass.
9. No direct commits to `main`. Work uses a branch + PR.
10. A phase merges only after required CI is green.
11. The repository is public. ROMs, extracted Nintendo/Game Freak/Creatures
    commercial assets, keys and secrets must never be committed.
12. Real Unreal Engine build, APK production and BrowserStack real-device tests
    are deliberately deferred to the final PC stage.

## 3. Current completed foundation

### R0 — Foundation / production architecture

Complete.

- portable authoritative core boundary;
- deterministic state/event contract;
- Unreal selected as production presentation runtime;
- SDL/Godot retained only as historical R0 evidence/reference.

### R1 — Save / state / RTC / event foundation

Complete.

- Emerald-compatible save-domain work;
- persistent flags/vars;
- event/state bridge;
- RTC-related behavior;
- real-save validation evidence.

### R2 — Script engine

Complete.

- portable script runtime;
- map script behavior;
- flag/var integration;
- deterministic script regression coverage.

### R3 — Full Hoenn world data

Complete.

Accepted source package includes:

- 34 map groups;
- 518 maps;
- 441 layouts;
- 468 map-script source files;
- deterministic conversion and audit.

### R4 — Authoritative overworld runtime

Complete.

- real movement;
- collision;
- warps;
- map connections;
- object events;
- authoritative player/world state;
- Brendan's House -> Littleroot -> Route 101 acceptance slice.

### R5 — Unreal world renderer bridge

Complete at source/data acceptance level.

- R4 remains authoritative;
- Unreal renders the world package;
- deterministic render package;
- chunked/HISM world path;
- real Vanilla+ tileset semantics;
- no claim yet of a licensed UE executable or packaged Android runtime smoke.

## 4. Fixed implementation order

The phase numbers are intentionally preserved. The implementation order from
this point is:

1. R17 — Save compatibility / migration
2. R6 — Character / NPC presentation and asset pipeline
3. R7 — Camera / environment presentation and asset pipeline
4. R9 — UI / HUD / menu infrastructure
5. R14 — Battle presentation and Pokémon asset pipeline
6. R15 — Audio
7. R8 — Android input infrastructure
8. R19 — Regression / test infrastructure expansion
9. R20 — Code / package polish
10. R18 — Real UE 5.8.3 / Android production build on PC
11. Real Unreal runtime validation for R6/R7/R8/R9/R14/R15
12. BrowserStack real Android smoke test
13. R19 final Android device matrix
14. R21 — Release Candidate
15. R22 — Final Release

Do not renumber phases to match implementation order.

---

# 5. Phase specifications

## R10 — Map + quest guidance

**Status: COMPLETE.** See `docs/R10_QUEST_MAP.md`.

### Goal

Expose the current Vanilla Emerald main-story objective without creating a
second story/progression system.

### Rules

- Emerald flags/vars/scripts remain authoritative.
- No separate persisted quest-progress field.
- The active objective is derived from current Emerald state.
- One highlighted main-story objective.
- Region/map/local target guidance is allowed.
- No GPS-style route solver.
- Unreal only displays the core result.

### Initial data basis

The vendored Vanilla+ Phase 10A objective set is the starting source, but every
objective must be re-audited against the pinned real Emerald/Vanilla+ flag,
var, map, region-section and object-event data.

### Acceptance

- 32 current main-story objectives are represented or explicitly corrected;
- objective state is deterministic: locked / active / completed;
- exactly one active main-story objective when story state requires one;
- region, map, object-event and coordinate targets resolve against R3 data;
- region-map marker data is canonical and deterministic;
- save encode/decode yields the same derived objective;
- after game-clear state, no normal main-story objective remains;
- no SaveBlock/Pokémon-storage layout change;
- portable tests and data audit pass.

## R11 — Pokémon / party / item core

**Status: COMPLETE.** See `docs/R11_POKEMON_PARTY_ITEM.md`.

### Goal

Port the Emerald Pokémon-domain truth needed by encounters, battles, UI and
save compatibility.

### Scope

- species identity and core species data;
- party and storage-compatible Pokémon representation;
- personality/nature;
- IV/EV;
- level/experience;
- stats;
- moves, PP and move state;
- status;
- friendship;
- ability identity;
- held items;
- item inventory/bag semantics required by gameplay;
- evolution-facing data needed by later gameplay.

### Acceptance

- canonical representative Pokémon fixtures match Emerald/Vanilla+ behavior;
- encode/decode preserves Pokémon-domain state;
- party/item operations are deterministic;
- no Unreal gameplay ownership;
- old R0-R10 regressions remain green.

## R12 — Encounter system

**Status: COMPLETE.** See `docs/R12_ENCOUNTER_SYSTEM.md`.

### Goal

Port the real Emerald encounter decision path, not merely encounter tables.

### Scope

- land/water/fishing encounter selection;
- encounter rates;
- map encounter tables;
- RNG call ordering;
- repel behavior;
- level checks;
- lead Pokémon effects/abilities supported by Emerald;
- Cleanse Tag / flute modifiers where applicable;
- roaming/special/outbreak hooks owned by Emerald;
- story-state encounter suppression where applicable.

### Acceptance

- fixed seeds reproduce expected encounter/no-encounter sequences;
- representative maps match pinned source tables;
- modifiers are regression-tested independently and in combination;
- RNG ordering is stable;
- R3/R4 world ownership remains unchanged.

## R13 — Battle core

**Status: COMPLETE.** See `docs/R13_BATTLE_CORE.md`.

### Goal

Port Emerald battle authority into the portable core.

### Scope

- battle state machine;
- turn/action resolution;
- move execution;
- damage;
- accuracy/evasion;
- critical hits;
- type effectiveness;
- status;
- weather;
- abilities;
- held-item battle effects;
- switching/fainting;
- experience and battle-end results;
- deterministic battle RNG;
- trainer/wild battle state required by Vanilla Emerald/Vanilla+.

### Presentation contract

The core emits descriptive battle events such as:

- battle started;
- move used;
- damage applied;
- critical hit;
- effectiveness result;
- status applied/removed;
- switch;
- faint;
- battle ended.

Unreal must not calculate these outcomes.

### Acceptance

- deterministic fixed-seed battle fixtures;
- representative one-on-one, multi-turn, status, weather, ability and item
  cases;
- battle result and resulting Pokémon/save-domain state are reproducible;
- no presentation dependency in the core.

## R16 — Vanilla+ QoL

**Status: COMPLETE.** See `docs/R16_VANILLAPLUS_QOL.md`.

### Goal

Apply only approved Vanilla+ quality-of-life behavior after the Vanilla Emerald
core is stable.

### Rules

- every QoL change must be explicit and regression-pinned;
- no accidental mechanical drift;
- QoL must not silently alter unrelated save, battle, encounter or story rules;
- pinned Vanilla+ source is the specification for already accepted changes.

### Acceptance

- each QoL feature has a dedicated behavior test;
- Vanilla behavior outside the changed surface remains unchanged;
- the QoL set is documented as a deliberate delta from stock Emerald.

## R17 — Save compatibility / migration

### Goal

Make save compatibility a release-grade contract.

### Scope

- real Emerald/Vanilla+ save import;
- validation/checksums/sector behavior;
- portable state reconstruction;
- save round-trip;
- forward migration rules if remaster-owned metadata is needed;
- remaster settings stored outside Emerald gameplay save data unless a proven
  compatibility requirement says otherwise.

### Acceptance

- representative real saves load;
- save -> remaster -> save round-trip remains valid;
- active story objective, party, bag, world state and progression survive;
- corrupt/unsupported saves fail safely;
- compatibility fixtures are versioned and regression-tested.

## R6 — Character / NPC presentation + character asset pipeline

**Status: pre-real-Unreal source/preparation acceptance VERIFIED_COMPLETE on
`r6-character-presentation`; merge/main gates pending.** See
[R6_PRESENTATION_COMPLETION.md](R6_PRESENTATION_COMPLETION.md) for source choices,
explicit fallbacks, local asset workflow, 181 Python / 62 CTest results and
remaining engine/device validation.

### Goal

Replace placeholder character presentation without moving NPC gameplay logic
out of the core.

### Asset strategy

Use locally extracted assets from a user-owned compatible source such as ORAS
where practical. Commercial extracted models/textures are never committed to
the public repository.

The repository stores only:

- extraction/conversion tools;
- mapping manifests;
- hashes/provenance;
- import settings;
- validation rules;
- placeholders or redistributable test assets.

### Scope

- Emerald graphics/object identity -> presentation model mapping;
- Brendan/May and main NPC presentation;
- trainer/NPC class mappings;
- shared human skeleton families where practical;
- idle/walk/run/turn/interact animation set;
- animation retargeting;
- materials/textures;
- scale/ground offset;
- LOD policy;
- fallback presentation for missing assets.

### Acceptance before real UE build

- deterministic asset manifest generation;
- all required character IDs either resolve or have an explicit fallback;
- no commercial binary asset leakage into Git;
- Unreal source consumes presentation identity only;
- gameplay object position/state remains R4-owned.

## R7 — Camera / environment presentation + environment asset pipeline

Current checkpoint: pre-real-Unreal implementation and hosted acceptance verified
on the dependent R7 branch; merge/main gates pending. See [R7_ENVIRONMENT_COMPLETION.md](R7_ENVIRONMENT_COMPLETION.md) for full
source identity/fallback coverage, camera/environment changes, budgets and limits.

### Goal

Render Emerald's authoritative world using modern 3D environmental assets while
preserving Emerald map geometry and gameplay.

### Core rule

Do **not** replace Emerald maps with ORAS maps.

The model is:

```text
Emerald map/layout/collision/warp truth
            +
presentation-only 3D visual assets
```

### Scope

- metatile/tileset/semantic visual identity -> 3D asset mapping;
- buildings;
- trees;
- rocks;
- grass;
- signs;
- fences;
- caves;
- indoor props;
- water;
- lighting/shadows;
- camera follow/framing;
- camera occlusion behavior;
- day/night presentation infrastructure where it does not change gameplay;
- LOD/HLOD/instancing;
- Android-first visual budgets.

### Rendering policy

- mobile performance is the baseline;
- Lumen is not a required dependency for visual correctness;
- expensive desktop-only features cannot become gameplay requirements;
- collision remains core-owned.

### Acceptance before real UE build

- complete mapping audit or explicit fallback for required R3 world visuals;
- no presentation mesh determines gameplay collision;
- deterministic environment package;
- representative maps resolve without missing identity errors;
- source-level mobile budget rules documented.

## R9 — UI / HUD / menu infrastructure

Source/preparation implementation on dependent `r9-ui-presentation` branch:
[R9 UI completion evidence](R9_UI_COMPLETION.md). This checkpoint provides the
shared native model and source UMG/input integration; real engine/device and
host-entry integration checks remain explicit. PR/merge/main gates are separate.

### Goal

Provide modern presentation for Emerald information without duplicating
gameplay state.

### Scope

- HUD;
- start/menu shell;
- map;
- R10 quest/objective display;
- party UI;
- bag/item UI;
- Pokémon summary UI;
- settings;
- save/load presentation;
- dialogue presentation hooks;
- controller/touch-friendly navigation.

### Acceptance

- UI is a consumer of core state;
- no duplicated progression, party, item or battle truth;
- all required screens can be driven from portable/test fixtures;
- input actions are abstract and platform-independent.

## R14 — Battle presentation + Pokémon asset pipeline

### Goal

Present R13 battle events with modern Pokémon models, animation, camera and VFX.

### Asset strategy

Prefer locally extracted ORAS-compatible Pokémon assets from a user-owned
source. Do not scrape individual Pokémon as the production dependency.

The asset pipeline must audit:

- species model;
- texture;
- skeleton;
- available skeletal animations;
- scale;
- ground offset;
- missing/special animation requirements.

Known special case: some source games use material/visibility animation in
addition to skeletal animation. Missing support must be explicit rather than
silently ignored.

### Scope

- species -> model resolver;
- idle/entry/attack/hit/faint presentation;
- battle camera;
- positioning/scaling;
- trainer presentation;
- VFX;
- event sequencing;
- animation fallback families where a source animation is missing.

### Acceptance before real UE build

- species #001-#386 asset audit report;
- missing assets/animations listed explicitly;
- deterministic species manifest;
- battle presentation consumes R13 events only;
- battle outcome never depends on animation timing.

## R15 — Audio

Preparation/pilot status: [R15_AUDIO_PREPARATION.md](R15_AUDIO_PREPARATION.md).
Modern style selected after the 12-species pilot; 386 normal cries and 4,632
special-mode candidates are prepared. Special-mode listening/hardware comparison,
music/FX and runtime gates remain open.

### Goal

Preserve Emerald audio identity while replacing playback infrastructure.

### Authority model

Core/gameplay emits stable semantic IDs/events.

Examples:

- music ID;
- SFX ID;
- jingle ID;
- Pokémon cry event;
- ambience event.

Unreal resolves the ID to the actual local audio asset.

### Asset pipeline

The public repository contains conversion/render tools, mappings, manifests and
validation, not copyrighted commercial audio binaries.

Support may include:

- original-style locally rendered pack;
- optional remastered presentation pack;
- identical semantic IDs for both.

### Scope

- BGM mapping;
- SFX mapping;
- jingles;
- Pokémon cries;
- battle transitions;
- world ambience;
- loop metadata;
- crossfade;
- concurrency/priority;
- music/SFX/master settings;
- Android lifecycle pause/resume behavior.

### Acceptance before real UE build

- all required semantic audio IDs resolve or fail explicitly;
- loop/transition metadata is deterministic;
- gameplay code has no asset-path dependency;
- no copyrighted audio binary is committed.

## R8 — Android input infrastructure

### Goal

Map platform input onto common gameplay/UI actions.

### Model

```text
touch / gamepad / keyboard
          ↓
common actions
          ↓
core command or UI action
```

Examples:

- up/down/left/right;
- confirm/cancel;
- menu;
- interact/action;
- battle/menu selection.

### Acceptance before real UE build

- platform-neutral action contract;
- touch layout/source wiring;
- gamepad/keyboard equivalents;
- no Android-specific gameplay rule;
- lifecycle-safe input state reset.

## R19 — Regression / test infrastructure expansion

### Goal

Expand the existing regression-first process into full-game automated evidence.

Testing does **not** begin here; every earlier phase already requires tests.
R19 expands coverage and orchestration.

### Scope

- deterministic input replays;
- canonical state hashes;
- story/progression replay slices;
- save fixture matrix;
- encounter seed matrix;
- battle seed/scenario matrix;
- asset manifest audits;
- package integrity tests;
- Unreal smoke harness preparation;
- final device-matrix definitions.

### Acceptance before real UE build

- portable full-suite entry point;
- replay failures identify the first divergent state;
- asset/data/package audits are CI-gated;
- no real UE/APK requirement yet.

## R20 — Code / package polish

### Goal

Prepare a clean, auditable production tree before the expensive real-engine
stage.

### Scope

- remove dead production code;
- keep R0 experiments clearly historical;
- dependency cleanup;
- generated package structure;
- deterministic manifests/hashes;
- provenance records;
- documentation consistency;
- public-repo asset leakage audit;
- secret scan;
- release configuration preparation.

### Acceptance

- canonical docs agree with this roadmap;
- source/data packages are reproducible;
- no required production step depends on an undocumented local file;
- portable CI remains green.

---

# 6. Deferred PC / real-engine stage

No Unreal Engine installation, self-hosted runner, APK production or
BrowserStack real-device execution is required before this stage.

## R18 — Real UE 5.8.3 / Android production build

Run on the project PC.

### Tasks

- install/verify Unreal Engine 5.8.3;
- install/verify Android toolchain required by UE 5.8.x;
- compile production Unreal target;
- cook/package Android ARM64;
- produce installable APK;
- capture compile/cook/package failures as source fixes;
- run local launch smoke.

Expected Android baseline for the planned UE 5.8.x toolchain is tracked in the
Unreal setup documentation and must be re-verified at R18 before installation.

## Real Unreal runtime validation

After a successful build, validate the presentation phases that were previously
source/data-gated:

- R6 character/NPC runtime;
- R7 environment/camera runtime;
- R8 Android input runtime;
- R9 UI runtime;
- R14 battle presentation runtime;
- R15 audio runtime.

Fix failures through normal branch/PR/regression workflow.

## Local Android acceptance before cloud devices

Before BrowserStack:

- install APK on a local physical Android device;
- verify boot;
- verify rendering package;
- verify save access;
- verify touch input;
- verify the Brendan's House -> Littleroot -> Route 101 slice;
- verify one battle;
- verify audio;
- verify pause/resume.

---

# 7. BrowserStack real-device stage

Existing BrowserStack integration infrastructure is useful, but real-device
execution waits until a valid APK exists.

The secret flow remains external to the repository:

```text
Doppler
  -> GitHub environment: browserstack-ci
  -> GitHub Actions
  -> BrowserStack API
```

Never commit BrowserStack credentials.

## Required smoke markers

The packaged application/runtime harness must emit:

```text
REM_SMOKE: BOOT_OK
REM_SMOKE: RENDER_PACKAGE_OK
REM_SMOKE: HOUSE_RENDER_OK
REM_SMOKE: HOUSE_WARP_OK
REM_SMOKE: LITTLEROOT_RENDER_OK
REM_SMOKE: ROUTE101_RENDER_OK
REM_SMOKE: PASS
```

BrowserStack acceptance requires the final `PASS` marker and no earlier marker
failure.

---

# 8. R19 final Android device matrix

After the first BrowserStack smoke succeeds, execute the full device matrix.

The matrix should cover:

- at least one lower supported performance tier;
- representative mid-range Android;
- representative high-end Android;
- different GPU vendors where practical;
- supported Android API range;
- portrait/landscape behavior only where the product actually supports it;
- lifecycle/background-resume;
- save persistence;
- input;
- overworld;
- battle;
- UI;
- audio;
- performance/memory stability.

Performance targets must be defined from measured R18 builds, not guessed before
a real engine build exists.

---

# 9. R21 — Release Candidate

### Goal

Freeze a candidate build for final verification.

### Entry requirements

- all planned gameplay phases merged;
- real Unreal compile/cook/package passes;
- local device smoke passes;
- BrowserStack smoke passes;
- final Android matrix passes or has documented accepted exceptions;
- no known save-corruption issue;
- no known deterministic gameplay divergence;
- no missing mandatory asset mapping;
- no repository secret/commercial-asset leakage.

### RC work

- version/build identifiers;
- clean release configuration;
- final migration tests;
- final replay suite;
- final package hash/provenance report;
- release notes;
- known limitations.

---

# 10. R22 — Final Release

### Exit requirements

- R21 RC accepted;
- no release-blocking regression;
- production package reproducible from documented local prerequisites;
- final source tree and documentation match the released build;
- final APK/package hashes recorded;
- release artifacts contain no prohibited source assets or secrets.

---

# 11. Asset factory — shared rule for R6/R7/R14/R15

The project uses one common local asset-preparation concept rather than
independent manual import workflows.

```text
user-owned source data
        ↓
extract
        ↓
normalize / convert
        ↓
validate
        ↓
manifest + hashes
        ↓
Unreal-ready local package
```

The public repository contains tooling and metadata, not extracted commercial
assets.

The pipeline must be:

- deterministic where possible;
- auditable;
- resumable;
- explicit about missing assets;
- capable of producing an inventory report;
- independent from gameplay authority.

For environment visuals, ORAS or other compatible models are visual source
material only. Emerald map/layout/collision/warp data remains authoritative.

---

# 12. Testing policy for every remaining phase

Every phase follows this order:

1. inspect current `main` implementation and data;
2. inspect pinned Emerald/Vanilla+ source behavior;
3. define acceptance criteria;
4. write failing regression/audit tests first where practical;
5. implement in small verifiable commits;
6. run targeted tests;
7. run earlier-phase regression tests;
8. open/update PR;
9. require green CI;
10. merge to `main`;
11. only then start the next phase.

A source-level Unreal validator is useful before R18, but it is **not** evidence
of successful UE compilation or Android runtime.

---

# 13. Repository and security policy

- public repository;
- no ROM images;
- no extracted commercial model/texture/audio payloads;
- no credentials;
- no Doppler/BrowserStack secrets;
- generated local asset directories remain ignored;
- manifests may contain IDs/hashes/provenance but not proprietary payloads;
- third-party redistributable assets must retain their licenses;
- if a self-hosted runner is ever introduced, it must not execute untrusted
  public-fork PR code with access to the host or secrets.

---

# 14. Immediate next action

The active phase is **R19 Regression infrastructure** on
`r19-regression-infrastructure`. Verified main is
`7e8d027d6c1f00677b0f270d56bfe7d4de83590d`: R8 PR #22, all 15
workflows terminal-success including CodeQL. Main R8 workflow 37853379312
passed 8 targeted / 75 full CTest and 328 Python tests plus source guards.
R6/R7/R9/R14/R15/R8 M1/M2 are VERIFIED_COMPLETE.
[R19-P1 inventory and remaining source/runtime gaps](R19_REGRESSION_INVENTORY.md).

R15-V1-Closure-1 is VERIFIED_COMPLETE at
`3b9b208f934ecba9ac9be857cd40e38e88cbfec0`: hosted
[workflow 37763598815](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37763598815)
passed 35/35 targeted Python, 271/271 full Python and 67/67 CTest with zero skips.

R15-I3-Closure-1 is VERIFIED_COMPLETE at
`9df9fae1d56420259f9d48ab950183800a5eea06` (workflow 37766890311).
The unavailable local pack contents were recovered from pinned receipts:
386 normal WAV hashes and the entire historical special manifest match exactly.
5,018 candidates and both complete ZIP contents passed local verification.
New ZIP containers have new hashes, recorded separately from unchanged historical
evidence. Its exact-head CI passed 43 targeted Python, 279 full Python and 67 CTest.
[Recovery evidence and next steps](R15_AUDIO_PACK_RECOVERY.md).

R15-I2-Closure-1 and Closure-2 are VERIFIED_COMPLETE at `4f8ef9a` and `2ae0e26`.
Their exact-head workflows 37769695700 and 37777810560 completed successfully;
Closure-2 passed 68 targeted Python, 304 full Python and 67 CTest with zero skips.

R15-I2-Closure-3 is VERIFIED_COMPLETE at `5a547510`:
workflow 37779942858 passed 74 targeted Python, 310 full Python and 67 CTest,
zero skips; 209/209 music/jingle candidates and all 176 loop bounds passed.
[Full coverage evidence](R15_BGM_RENDER_COVERAGE.md) records private source
rendering without promoting loop playback, listening or engine/device readiness.
R15-G1/D1 is VERIFIED_COMPLETE at `695d01b`; workflow 37780786306 passed
74 targeted Python, 310 full Python and 67 CTest, zero skips.
The four canonical source acceptance bullets pass; actual assets/import, listening,
owner/platform attachment and engine/device obligations remain explicitly open in
[R15_AUDIO_COMPLETION.md](R15_AUDIO_COMPLETION.md).
R15-F1 publication `515dde0` passed workflow 37781258584 (74/310/67, zero skips).
The verified dependent phases then integrated through PR #17/#18/#19/#20/#21
and terminal-success main CI in order. R8-P1 is versioned at `261cef11`. R8-P2 freezes the platform/lifecycle
schema in [R8_INPUT_CONTRACT.md](R8_INPUT_CONTRACT.md) and
`data/r8/input_contract.json`; R8-I1 is VERIFIED_COMPLETE at `1d45a91d` (workflow 37817943261: 1/68/310 PASS).
R8-I2 is VERIFIED_COMPLETE at `c18d820c` (workflow 37846220404: 2/69/310 PASS;
all 16 exact-head workflows including CodeQL succeeded).
R8-I3 is VERIFIED_COMPLETE at `a39c97a3` (workflow 37847802114: 3/70/313 PASS).
R8-I4 is VERIFIED_COMPLETE at `3f04104e` (workflow 37848753892: 4/71/317 PASS).
R8-I5 is VERIFIED_COMPLETE at `6046cba6` (workflow 37849310238: 5/72/320 PASS).
R8-I6 is VERIFIED_COMPLETE at `7194102a` (workflow 37849841175: 6/73/322 PASS).
R8-T1/V1 passed at `d1730592` (workflow 37850579912: 7/74/326 PASS).
R8-Closure-1/V1/G1 is VERIFIED_COMPLETE at `19f8716d`; workflow 37851201049
passed 8/75/328. R8-D1 is VERIFIED_COMPLETE at `ddd3ce19`; workflow
37851617574 passed 8/75/328 and all 16 exact-head workflows including CodeQL succeeded.
R8-F1 is VERIFIED_COMPLETE at `ea582534`; all 16 final-head push/PR workflows succeeded.
[Acceptance/evidence/runtime boundaries](R8_INPUT_COMPLETION.md). Evidence: [R8_INPUT_IMPLEMENTATION.md](R8_INPUT_IMPLEMENTATION.md).
R8-M1 PR #22 merge and R8-M2 exact-main CI are VERIFIED_COMPLETE.
R19-P1/P2 are VERIFIED_COMPLETE at `915fee92` / `605a8378`.
R19-I1 is VERIFIED_COMPLETE at `ab3196cc`; workflow 37855418711: 1/76 CTest, 9/337 Python/source PASS.
R19-I2 is VERIFIED_COMPLETE at `af9ffdea`; workflow 37857133241: 1/76 CTest, 19/347 Python/source PASS.
**R19-Closure-1** is VERIFIED_COMPLETE at c76443b0; workflow 37953441780:
2 targeted / 77 full CTest, 19 targeted / 347 full Python/source PASS.
R19-I3 VERIFIED_COMPLETE at de446656; workflow 37955312626:
3 targeted / 78 full CTest, 22 targeted / 350 full Python/source PASS.
R19-I4 VERIFIED_COMPLETE at 33945600; workflow 37956206672:
4 targeted / 79 full CTest, 28 targeted / 356 full Python/source PASS.
R19-Closure-2 VERIFIED_COMPLETE at 46611975; workflow 37956783169:
4 targeted / 79 full CTest, 29 targeted / 357 full Python/source PASS.
R19-I5 VERIFIED_COMPLETE at ff8858d0; workflow 37958197475:
5 targeted / 80 full CTest, 34 targeted / 362 full Python/source PASS.
R19-I6 VERIFIED_COMPLETE at 7c86759c; workflow 37960402440:
6/81 CTest, 38/366 Python/source PASS; all 17 workflows terminal-success.
R19-I7 VERIFIED_COMPLETE at e31d5666; workflow 37961436672:
6/81 CTest, 44/372 Python/source, 171 actual asset audits, zero skips.
R19-I8 VERIFIED_COMPLETE at 73fd89dc; workflow 37962037380:
6/81 CTest, 50/378 Python/source; 8 clean generations / 12 corruptions rejected.
R19-I9 VERIFIED_COMPLETE at 42a5f4de; workflow 37962672669:
6/81 CTest, 57/385 Python/source; production smoke SOURCE_CONTRACT_PASS.
R19-I10 VERIFIED_COMPLETE at 9e301799; workflow 37963233981:
6/81 CTest, 63/391 Python/source; DEFERRED_METADATA_PASS, zero devices.
R19-T1/V1 VERIFIED_COMPLETE at fc83fd2; workflow 37964067675:
81 CTest, 397 Python, 171 asset audits; all 17 exact-head workflows succeeded.
R19-Closure-3 VERIFIED_COMPLETE at 1bd0c356; workflow 38010794841:
81 CTest, 400 Python, 171 asset audits; all seven source components passed.
R19-V1/G1 pre-real source acceptance VERIFIED_COMPLETE.
R19-D1 VERIFIED_COMPLETE at 1489aefd; workflow 38011198536:
81 CTest, 400 Python, 171 assets; all 17 workflows / 30 check runs succeeded.
R19-F1 VERIFIED_COMPLETE at 416f817c: workflow 38011903730, 81/400/171;
all 17 workflows / 31 checks succeeded. PR #23 merged at ab057754.
R19-M2 VERIFIED_COMPLETE: workflow 38012481196, actual 81/400/171;
all 16 exact-main workflows / 29 checks including CodeQL succeeded.
R20-P1 VERIFIED_COMPLETE at a7db2bbb (four docs only; exact publication verified).
R20-I1 VERIFIED_COMPLETE at 24e8779f; workflow 38014100330: actual 81/400/171;
all 16 workflows succeeded. Current checkpoint: **R20-I2**, dependency CI VERIFYING.
[Named implementation evidence](R20_PRODUCTION_IMPLEMENTATION.md).
[Production inventory and named gaps](R20_PRODUCTION_INVENTORY.md).
[Accepted R19 source/runtime boundary](R19_REGRESSION_COMPLETION.md).
[Contract](R19_REPLAY_CONTRACT.md) / [checkpoint evidence](R19_REGRESSION_IMPLEMENTATION.md).
No actual runtime/local asset obligation was closed by these source merges.

Real Unreal build remains R18 work. After R18 main verification, stop at
REAL UNREAL RUNTIME VALIDATION; do not automatically run physical Android
smoke, BrowserStack, final device matrix, R21 or R22.

