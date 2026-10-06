# Phase Execution Plan — Through Real Unreal Runtime Validation

Date: 2026-10-06
Status: active execution plan
Canonical roadmap source: `docs/ROADMAP.md`

This file defines the small, resumable execution units used to execute the
remaining roadmap. The execution map is independent of any particular chat,
scheduler or orchestration mechanism. It does not renumber roadmap phases and
does not replace the canonical product/architecture rules in ROADMAP/ARCHITECTURE.

## Controller contract

- Repository state is authoritative; chat state is not.
- Every run begins by re-reading branch HEAD, recent commits, CI, this plan,
  the owning phase docs/ledger, and the source/tests relevant to CURRENT_STATE.
- One run executes exactly one named subphase below.
- A write subphase should produce at most one cohesive Git transaction/commit.
- Do not use CI/workflows as a hidden transport that self-commits code.
- Unexpected concurrent drift is reconciled before work; conflicting work is
  never overwritten.
- Pinned Emerald/Vanilla+ production source is primary gameplay evidence.
- Tests/docs/verifiers are evidence, not a substitute for authoritative source.
- CI pending/in-progress/cancelled/failure blocks a gate.
- A failed gate enters only a gap-specific Closure step, then returns to the
  named verification/gate step. Closure never silently broadens scope.
- Every checkpoint reports HEAD, changed scope/files, and actual test/CI result.
- No direct commits to main. Each R phase uses a branch/PR and merges only when
  its final gate is green. Main CI must be terminal-success before the next R.
- ROMs, extracted commercial assets, keys, secrets and credentials never enter
  the public repository.
- R phases advance automatically without user approval through R18.
- Stop after R18 main verification at REAL UNREAL RUNTIME VALIDATION.
- If the execution environment cannot perform an R18 PC-only step, stop at that
  exact subphase as EXTERNAL_ENV_REQUIRED; never fake completion.

## State transition rule

At the end of a successful subphase, record and report the next named subphase.
The orchestration mechanism is non-authoritative and may change without changing
the phase map. Do not silently execute the next subphase in the same checkpoint,
and do not advance past a failed verification or gate.

---

# R16 — Vanilla+ QoL closure

Already complete before this plan:
- A1 contract/source verification
- A2 Move Relearner implementation
- A3 targeted CI
- Gate A = VERIFIED_COMPLETE
- B1 coverage audit
- B2 not required
- B3 regression closure commit: QOL-048 PARTY_FULL + BOX_FULL preservation

Remaining states:

### R16-B4 — Targeted verification
Verify the live B3 HEAD, targeted QoL tests, source audit, Unreal embed/source
boundary, and R16 workflow. No implementation changes. Terminal green -> B5.
Failure -> gap-specific R16-B-Closure.

### R16-B5 — Gate B
Independently reconcile QOL-025/027/045/046/048, 80-byte BoxPokemon
preservation, MAIL/PARTY_FULL/BOX_FULL/last-usable safeguards, tests, CI and
drift. Complete -> C1. Gap -> R16-B-Closure.

### R16-C1 — Decision ledger reconciliation
Audit all 48 QoL decisions and exact totals 22 ACCEPTED / 18 ALREADY_COVERED /
8 REJECTED against source, ownership and regression references. Read/audit only
unless a ledger-only correction is required.

### R16-C2 — Cross-R regression audit
Audit R1/R10/R11/R12/R13/R16 regression ownership and ensure R16 does not
duplicate or alter earlier authority.

### R16-C3 — Docs/runtime reconciliation
Reconcile docs against runtime for fishing mandatory rounds/optional zero,
Bag/PC sorting, VP5 21-byte metadata, held GIVE/TAKE, Move Relearner and final
ownership boundaries. Only docs/ledger corrections belong here.

### R16-C4 — Final CI matrix
Verify the complete R16 acceptance matrix at one live HEAD: behavior, item,
Move Relearner, source audit, Unreal embed/source boundary and required
cross-phase regressions. No feature work.

### R16-C5 — Final Gate
Independent final R16 gate: scope complete, no remaining RED, targeted/full
CI terminal-success, docs/ledger/runtime consistent, no unexpected drift.
Pass -> R16-M1. Gap -> R16-C-Closure-N -> C4 -> C5.

### R16-M1 — Merge to main
Re-check branch/main drift and final CI, open/update PR if needed, merge the
verified R16 branch into main. No new feature work.

### R16-M2 — Main verification
Verify resulting main HEAD and required main CI terminal-success. Pass -> R17-P1.

---

# R17 — Save compatibility / migration

### R17-P1 — Scope and existing-surface audit
Inventory current Emerald save reader/writer, sector/checksum logic, party/bag/
world/objective reconstruction, real-save fixtures, save metadata and tests.
Pin exact gaps against ROADMAP acceptance.

### R17-P2 — Source-format contract audit
Re-read pinned Emerald/Vanilla+ save structures, sector layout, save-slot
selection, checksums, security/encryption semantics and any accepted Vanilla+
save-domain deltas. Produce the implementation contract; no behavior write.

### R17-I1 — Slot/sector validation
Implement or close gaps in save-slot choice, sector IDs/signatures, section
ordering and checksum validation. No domain reconstruction changes.

### R17-I2 — Full domain import reconstruction
Close import gaps for world/progression, flags/vars, party/storage, bag/items,
RTC/objective derivation and other already-owned authoritative domains.

### R17-I3 — Export and round-trip preservation
Implement/close write-path gaps so import -> remaster -> export preserves valid
Emerald/Vanilla+ save semantics and untouched bytes/sections where required.

### R17-I4 — Migration/metadata boundary
Implement explicit forward-migration rules only where proven necessary.
Remaster settings remain outside Emerald gameplay save data unless compatibility
evidence requires otherwise. Audit Vanilla+ VP5 compatibility.

### R17-I5 — Corrupt/unsupported handling
Add safe rejection/fallback paths for corrupt sectors, checksum failures,
unsupported layout/version cases and incomplete saves. No silent repair unless
source/contract explicitly permits it.

### R17-T1 — Compatibility fixture matrix
Add/version representative real-save and synthetic fixtures covering import,
round-trip, progression, party, bag, objective, corruption and migration cases.

### R17-V1 — Targeted/full CI
Run R17 tests plus earlier core regressions and save-layout/source-boundary
checks at one HEAD.

### R17-G1 — Acceptance Gate
Verify all ROADMAP R17 acceptance bullets. Gap -> R17-Closure-N -> V1 -> G1.
Pass -> R17-D1.

### R17-D1 — Completion evidence
Write/update R17 completion evidence, exact fixtures, compatibility limits,
migration policy and acceptance HEAD/run.

### R17-F1 — Final Gate
Independent docs/runtime/CI/drift reconciliation. Pass -> R17-M1.

### R17-M1 — Merge to main
Verified branch/PR merge only.

### R17-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R6-P1.

---

# R6 — Character / NPC presentation + asset pipeline

### R6-P1 — Presentation identity inventory
Inventory all R3/R4 graphics/object identities needed for player, NPCs,
trainers and special objects; identify existing Unreal bridge surfaces.

### R6-P2 — Asset/provenance contract
Freeze public-repo vs local-asset boundary, manifest schema, hashes/provenance,
fallback rules, skeleton-family policy and deterministic generation rules.

### R6-I1 — Character manifest/resolver
Implement deterministic Emerald graphics/object identity -> presentation asset
identity manifest and resolver, with explicit fallback.

### R6-I2 — Player characters
Add Brendan/May presentation mappings, scale/ground offsets and player animation
identity contract without moving gameplay position/state out of R4.

### R6-I3 — NPC/trainer mappings
Add NPC/trainer-class mapping families and deterministic resolution coverage.

### R6-I4 — Animation contract
Implement idle/walk/run/turn/interact animation identity, shared skeleton family
and retarget metadata/fallback policy.

### R6-I5 — Materials/scale/LOD metadata
Implement presentation-only materials/textures references, scale, ground offset
and LOD policy metadata; no proprietary payloads.

### R6-I6 — Missing-asset/fallback audit
Generate deterministic audit showing every required character identity resolves
or has an explicit fallback; reject accidental local-only dependencies.

### R6-T1 — Source/manifest regressions
Pin deterministic manifest generation, identity coverage, fallback behavior,
asset-leakage checks and R4 gameplay-ownership boundary.

### R6-V1 — Targeted CI
Run R6 manifest/source-boundary tests and earlier regressions.

### R6-G1 — Acceptance Gate
Verify all pre-real-UE R6 acceptance bullets. Gap -> R6-Closure-N -> V1 -> G1.

### R6-D1 — Completion evidence
Record manifest coverage, local asset workflow, fallback counts and acceptance
evidence.

### R6-F1 — Final Gate
Independent CI/docs/source/drift reconciliation. Pass -> R6-M1.

### R6-M1 — Merge to main
Verified branch/PR merge only.

### R6-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R7-P1.

---

# R7 — Camera / environment presentation + asset pipeline

### R7-P1 — World visual identity inventory
Inventory R3 layouts/tilesets/metatile semantic identities and current R5
renderer bridge surfaces; identify required environment presentation classes.

### R7-P2 — Environment/mobile contract
Freeze manifest schema, local asset/provenance boundary, fallback rules,
Android-first budgets and the invariant that meshes never own gameplay collision.

### R7-I1 — Environment manifest/resolver
Implement deterministic world visual identity -> presentation asset resolver.

### R7-I2 — Static environment families
Implement mapping metadata for buildings, trees, rocks, grass, signs, fences,
caves and indoor props with explicit fallbacks.

### R7-I3 — Water/lighting/day-night presentation
Implement presentation-only water, lighting/shadow and non-gameplay day/night
infrastructure contracts.

### R7-I4 — Camera follow/framing
Implement camera follow, framing and camera-state source contract driven by
authoritative world state.

### R7-I5 — Camera occlusion
Implement presentation-only occlusion behavior without changing core collision
or movement decisions.

### R7-I6 — LOD/HLOD/instancing budgets
Implement deterministic LOD/HLOD/instancing metadata and Android-first budget
rules; no desktop-only feature becomes correctness-critical.

### R7-I7 — Representative world audit
Audit representative indoor/outdoor/cave/water maps for complete identity
resolution and fallback behavior.

### R7-T1 — Environment/source regressions
Pin deterministic package generation, missing identity detection, mobile budget
rules and gameplay-collision boundary.

### R7-V1 — Targeted CI
Run R7/R5 world renderer/source-boundary tests and earlier regressions.

### R7-G1 — Acceptance Gate
Verify all pre-real-UE R7 acceptance bullets. Gap -> R7-Closure-N -> V1 -> G1.

### R7-D1 — Completion evidence
Document mapping coverage, budgets, fallbacks and acceptance run.

### R7-F1 — Final Gate
Independent reconciliation. Pass -> R7-M1.

### R7-M1 — Merge to main
Verified branch/PR merge only.

### R7-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R9-P1.

---

# R9 — UI / HUD / menu infrastructure

### R9-P1 — Screen/state inventory
Inventory required HUD/menu/map/quest/party/bag/summary/settings/save-load/
dialogue screens and existing portable snapshots/events/actions.

### R9-P2 — UI authority/input contract
Freeze UI-consumer boundary, platform-neutral UI actions, fixture contract and
presentation-only QoL ownership from R16 classification.

### R9-I1 — HUD/dialogue shell
Implement source-level HUD and dialogue presentation hooks driven only by core
state/events.

### R9-I2 — Menu/navigation shell
Implement start/menu shell, abstract navigation/focus/confirm/cancel and menu
state without duplicating gameplay truth.

### R9-I3 — Map + quest UI
Implement R10 objective/map/marker presentation consumers.

### R9-I4 — Party + bag UI
Implement party and item/bag presentation/actions over authoritative R11/R16
APIs.

### R9-I5 — Pokémon summary UI
Implement summary presentation including accepted IV/EV/Hidden Power
information and nickname/relearner entry hooks without duplicating logic.

### R9-I6 — Settings + RTC presentation
Implement settings shell and R1-owned RTC option presentation; no duplicate RTC
core logic.

### R9-I7 — Save/load presentation
Implement save/load status/error/progress presentation over R17 APIs.

### R9-I8 — Presentation-only QoL
Implement the R16-classified UI/presentation deltas owned by R9, including fast
text/menu-repeat/related option surfaces where applicable, while keeping
gameplay outcomes independent of timing.

### R9-T1 — Fixture-driven UI regressions
Test screens from portable fixtures, navigation/action contracts and authority
boundaries without requiring real UE runtime.

### R9-V1 — Targeted CI
Run UI source/static validators plus relevant R10/R11/R16/R17 regressions.

### R9-G1 — Acceptance Gate
Verify ROADMAP R9 acceptance. Gap -> R9-Closure-N -> V1 -> G1.

### R9-D1 — Completion evidence
Document supported screens/actions and deferred real-runtime checks.

### R9-F1 — Final Gate
Independent reconciliation. Pass -> R9-M1.

### R9-M1 — Merge to main
Verified branch/PR merge only.

### R9-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R14-P1.

---

# R14 — Battle presentation + Pokémon asset pipeline

### R14-P1 — Battle presentation inventory
Inventory R13 battle events, species/trainer presentation identities and current
Unreal bridge surfaces.

### R14-P2 — Pokémon asset/provenance contract
Freeze local extraction/manifest rules, species #001-#386 coverage schema,
skeleton/animation/material/visibility special-case policy and fallbacks.

### R14-I1 — Species resolver/manifest
Implement deterministic species -> presentation asset manifest/resolver.

### R14-I2 — Scale/position metadata
Implement species scale, ground offset, side/slot positioning metadata.

### R14-I3 — Core animation identities
Implement idle/entry/attack/hit/faint presentation identities and fallback
families.

### R14-I4 — Special animation handling
Represent missing skeletal animation and material/visibility animation
requirements explicitly; never silently ignore unsupported cases.

### R14-I5 — Trainer presentation
Map trainer presentation identities and battle positioning without affecting R13
battle state.

### R14-I6 — Battle event sequencing
Implement presentation sequencing over R13 events; animation completion cannot
control authoritative battle resolution.

### R14-I7 — Battle camera
Implement source-level camera shot/state contract driven by battle events/state.

### R14-I8 — VFX contract
Implement semantic VFX identities and deterministic fallback/missing audit.

### R14-T1 — #001-#386 asset audit
Generate complete species audit plus event-authority/source-boundary regressions.

### R14-V1 — Targeted CI
Run R14 manifest/source validators and R13 battle regressions.

### R14-G1 — Acceptance Gate
Verify all pre-real-UE R14 acceptance bullets. Gap -> R14-Closure-N -> V1 -> G1.

### R14-D1 — Completion evidence
Record species/missing/special-animation audit and acceptance evidence.

### R14-F1 — Final Gate
Independent reconciliation. Pass -> R14-M1.

### R14-M1 — Merge to main
Verified branch/PR merge only.

### R14-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R15-P1.

---

# R15 — Audio

### R15-P1 — Semantic audio inventory
Inventory gameplay/presentation music, SFX, jingle, cry and ambience IDs/events
and existing playback bridge surfaces.

### R15-P2 — Audio asset/provenance contract
Freeze local render/conversion pipeline, public-repo boundary, manifest schema,
loop metadata and deterministic validation rules.

### R15-I1 — Audio manifest/resolver
Implement semantic ID -> local presentation asset resolver with explicit missing
behavior.

### R15-I2 — BGM/loop/transition metadata
Implement BGM mapping, loop points, transitions and crossfade metadata.

### R15-I3 — SFX/jingle/cry/ambience mapping
Implement remaining semantic audio categories and deterministic mapping audit.

### R15-I4 — Concurrency/priority
Implement presentation-only concurrency/priority policy without influencing
gameplay outcomes.

### R15-I5 — Audio settings
Implement master/music/SFX settings presentation/storage at the proper
non-gameplay boundary.

### R15-I6 — Android lifecycle contract
Implement pause/resume/audio-focus lifecycle handling at source/platform
boundary.

### R15-T1 — Audio manifest regressions
Pin required-ID coverage, missing-ID behavior, deterministic loop/transition
metadata and no core asset-path dependency.

### R15-V1 — Targeted CI
Run R15 source/manifest validators and relevant event regressions.

### R15-G1 — Acceptance Gate
Verify all pre-real-UE R15 acceptance bullets. Gap -> R15-Closure-N -> V1 -> G1.

### R15-D1 — Completion evidence
Document semantic coverage, local asset workflow and deferred runtime checks.

### R15-F1 — Final Gate
Independent reconciliation. Pass -> R15-M1.

### R15-M1 — Merge to main
Verified branch/PR merge only.

### R15-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R8-P1.

---

# R8 — Android input infrastructure

### R8-P1 — Common action inventory
Inventory core gameplay commands, R9 UI actions and existing Unreal/platform
input surfaces.

### R8-P2 — Platform/lifecycle contract
Freeze platform-neutral action schema, focus ownership, touch/gamepad/keyboard
mapping rules and lifecycle-reset semantics.

### R8-I1 — Common action router
Implement the platform-neutral action contract/router; no Android gameplay rule.

### R8-I2 — Touch source wiring
Implement source-level touch layout/action wiring and presentation-only touch
configuration.

### R8-I3 — Gamepad/keyboard mappings
Implement equivalent gamepad/keyboard action mappings.

### R8-I4 — UI/gameplay focus routing
Implement deterministic routing between R9 UI actions and core gameplay
commands without duplicate truth.

### R8-I5 — Lifecycle reset/reconnect
Implement safe input-state reset on pause/resume/focus loss and controller
disconnect/reconnect.

### R8-I6 — Presentation timing QoL wiring
Integrate R9/R16-owned key-repeat/input presentation settings at the platform
boundary without changing gameplay timing.

### R8-T1 — Input contract regressions
Pin action equivalence, focus/routing and lifecycle reset with source fixtures.

### R8-V1 — Targeted CI
Run input/source validators and relevant R4/R9 regressions.

### R8-G1 — Acceptance Gate
Verify all pre-real-UE R8 acceptance bullets. Gap -> R8-Closure-N -> V1 -> G1.

### R8-D1 — Completion evidence
Document action matrix and deferred real-device checks.

### R8-F1 — Final Gate
Independent reconciliation. Pass -> R8-M1.

### R8-M1 — Merge to main
Verified branch/PR merge only.

### R8-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R19-P1.

---

# R19 — Regression / test infrastructure expansion (pre-real-UE portion)

### R19-P1 — Coverage/orchestration inventory
Inventory all existing phase test entry points, fixtures, state snapshots,
manifests and CI workflows; identify full-game evidence gaps.

### R19-P2 — Replay/hash contract
Freeze deterministic input replay format, canonical state-hash boundaries,
divergence reporting and fixture versioning.

### R19-I1 — Input replay harness
Implement deterministic ordered-input replay runner over portable authority.

### R19-I2 — State hash/divergence reporting
Implement canonical state hashes and first-divergent-state diagnostics.

### R19-I3 — Story/progression replay slices
Add canonical progression slices using R2/R3/R4/R10 state.

### R19-I4 — Save fixture matrix
Orchestrate R17 compatibility fixtures into the full-suite matrix.

### R19-I5 — Encounter seed matrix
Build representative R12 encounter seed/scenario matrix.

### R19-I6 — Battle seed/scenario matrix
Build representative R13 battle matrix including R16 deltas.

### R19-I7 — Asset manifest audit orchestration
Aggregate R6/R7/R14/R15 manifest audits under one CI-gated entry point.

### R19-I8 — Package integrity audit
Add generated package/manifest/hash integrity checks.

### R19-I9 — Unreal smoke harness preparation
Define source-level smoke harness and required marker contract without claiming
a real UE executable.

### R19-I10 — Device-matrix definition
Define final Android device/API/GPU/ABI matrix metadata for the later
post-runtime-validation R19 device stage; do not execute it yet.

### R19-T1 — Portable full-suite entry point
Create one deterministic full-suite command/workflow that runs the pre-real-UE
matrix and reports first failure clearly.

### R19-V1 — CI orchestration verification
Verify all matrix components execute in CI and earlier phase workflows remain
green.

### R19-G1 — Acceptance Gate
Verify ROADMAP pre-real-UE R19 acceptance. Gap -> R19-Closure-N -> V1 -> G1.

### R19-D1 — Completion evidence
Document suite entry point, matrices and deferred real-device portion.

### R19-F1 — Final Gate
Independent reconciliation. Pass -> R19-M1.

### R19-M1 — Merge to main
Verified branch/PR merge only.

### R19-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R20-P1.

---

# R20 — Code / package polish

### R20-P1 — Production-tree audit
Inventory dead/experimental production paths, dependencies, generated package
layout, docs inconsistencies and release-prep gaps.

### R20-I1 — Dead-code/historical boundary cleanup
Remove dead production code while preserving R0 experiments clearly as
historical/reference material.

### R20-I2 — Dependency cleanup
Remove unused dependencies and pin/document required production dependencies.

### R20-I3 — Deterministic package structure
Normalize generated package layout and reproducible generation entry points.

### R20-I4 — Manifest/hash/provenance polish
Normalize deterministic manifests, hashes and provenance records across asset/
data packages.

### R20-I5 — Documentation consistency
Reconcile canonical docs, build steps and local-only asset prerequisites so no
required production step is undocumented.

### R20-I6 — Public-repo leakage/security audit
Strengthen/check asset leakage, secret scanning and ignored local proprietary
outputs. No credentials in repo.

### R20-I7 — Release configuration preparation
Prepare source-level release configs/settings required before the expensive
real-engine stage, without claiming a real UE build.

### R20-T1 — Reproducibility/security regression
Run clean-generation, package-integrity, leakage and secret-scan checks.

### R20-V1 — Full portable CI
Run the full R19 suite plus package/security checks.

### R20-G1 — Acceptance Gate
Verify all ROADMAP R20 acceptance bullets. Gap -> R20-Closure-N -> V1 -> G1.

### R20-D1 — Completion evidence
Record clean production-tree/reproducibility/security evidence.

### R20-F1 — Final Gate
Independent reconciliation. Pass -> R20-M1.

### R20-M1 — Merge to main
Verified branch/PR merge only.

### R20-M2 — Main verification
Main HEAD + CI terminal-success. Pass -> R18-P1.

---

# R18 — Real UE 5.8.3 / Android production build on project PC

R18 is the first phase that requires the actual project-PC Unreal/Android
environment. Source-only CI cannot substitute for these steps.

### R18-P1 — PC/toolchain prerequisite audit
On the project PC, verify available OS/storage/tool access and re-read current
official UE 5.8.x Android requirements tracked by the project. If project-PC
control is unavailable to the executing agent, stop here as
EXTERNAL_ENV_REQUIRED.

### R18-P2 — Unreal Engine verification
Install/verify the project-selected Unreal Engine 5.8.3 installation and record
the exact engine/build identity.

### R18-P3 — Android toolchain verification
Install/verify the Android SDK/NDK/JDK/toolchain versions required by the
selected UE 5.8.x build; record exact paths/versions without committing secrets.

### R18-I1 — Production Unreal target compile
Compile the production target. Capture failures as source issues; do not skip
or downgrade compile errors.

### R18-C1 — Compile-failure closure
Only when I1 fails: fix the smallest source/configuration gap on a branch,
regression-test it, merge after green CI, then repeat I1.

### R18-I2 — Cook
Cook the Android production content/configuration. Failure -> R18-C2 closure ->
repeat I2.

### R18-I3 — Android ARM64 package
Package an installable Android ARM64 APK using the verified toolchain.
Failure -> R18-C3 closure -> repeat I3.

### R18-I4 — Package integrity verification
Verify APK exists, expected ABI/package identity/configuration is present, and
record reproducible build metadata/hashes outside any secret-bearing data.

### R18-I5 — Local launch smoke
Launch on the project PC/emulator only if that is part of the validated R18
environment; this is a build-stage smoke, not the later full real-device/runtime
acceptance. Record failures and close them through normal source workflow.

### R18-G1 — Build acceptance Gate
Require real UE compile, cook and Android ARM64 package success plus package
integrity evidence. Gap -> corresponding R18 closure/retry. Pass -> R18-D1.

### R18-D1 — Build completion evidence
Record exact engine/toolchain/build/package evidence and all source fixes.

### R18-F1 — Final Gate
Independent reconciliation of R18 build evidence, repo HEAD/main CI and package
metadata. Pass -> R18-M1 if R18 source/docs changes remain unmerged; otherwise
R18-M2.

### R18-M1 — Merge any final R18 source/docs changes
Verified branch/PR merge only. Do not commit APK/commercial assets/secrets.

### R18-M2 — Main verification and STOP
Verify main HEAD/CI after all R18 source fixes. Then disable the carrier and stop
at **REAL UNREAL RUNTIME VALIDATION**. Do not automatically execute the runtime
validation, local physical-device acceptance, BrowserStack, final R19 device
matrix, R21 or R22.

---

# Generic closure rule

A `*-Closure-N` state may be created only by a named Gate or Verification step
that has identified a concrete missing behavior/test/doc/build requirement.
The closure prompt must name that exact gap, touch only the minimum required
surface, and route back to the verification/gate that detected it.

# Current resume point

R13 and R16 are complete on `main`. R17 is the active phase.

The fresh R17 working branch is `r17-save-compat-mainline`, created from the
post-R16 `main` baseline. R17-P1 repo/scope recovery is complete. R17-P2 is
complete with the authoritative contract versioned in
`docs/R17_SAVE_COMPAT_CONTRACT.md`.

R17-I1 now has explicit-format sector validation and counter-coherence
hardening, pinned by `r17_save_sector_validation`. The existing gameplay
decoder retains pinned Vanilla+ geometry; stock validation does not yet perform
stock domain reconstruction. Source/fixture/local verification evidence is in
section 16 of `docs/R17_SAVE_COMPAT_CONTRACT.md`.

Resume at **R17-I1 verification** until the exact live I1 commit's targeted and
complete relevant PR CI are terminal-success. Then the next named subphase is
**R17-I2 — Full domain import reconstruction**. Pending/failed CI keeps I1 open
and permits only a concrete gap-specific closure, not I2 implementation.

P2 closure remeasured the production AGBCC layout: existing remaster block sizes
`0xF44` / `0x3DC8` are correct for pinned Vanilla+; the earlier P2 contract had
mistaken stale comments for compiled sizes. Production VP5 metadata is at
compiled `SaveBlock1.unused_3598=0x35D8`; the remaster literal `0x3598` is an I4
gap. Evidence and format policy are in `docs/R17_SAVE_COMPAT_CONTRACT.md`.
I1 must verify format-specific checksum spans, slot choice, sector
IDs/signatures, rotation and the explicit counter-coherence policy. No
gameplay/implementation change belongs to P2.
