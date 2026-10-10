# Phase Execution Plan — Through Real Unreal Runtime Validation

Date: 2026-10-10
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

Current preparation/pilot evidence: [R15_AUDIO_PREPARATION.md](R15_AUDIO_PREPARATION.md).
Pre-real-UE source/preparation G1 is passed; completion evidence is recorded in
[R15_AUDIO_COMPLETION.md](R15_AUDIO_COMPLETION.md). Source F1/PR/main gates passed.
386 normal cries, 4,632 special-mode candidates and 209 Original-style BGM/jingle
candidates are prepared. Actual assets/import, fidelity/listening, owner/platform
attachment and engine/device tests remain explicit R18/runtime obligations.

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

# Historical resume records

R13 and R16 are complete on `main`. R17 is the active phase.

The fresh R17 working branch is `r17-save-compat-mainline`, created from the
post-R16 `main` baseline. R17-P1 repo/scope recovery is complete. R17-P2 is
complete with the authoritative contract versioned in
`docs/R17_SAVE_COMPAT_CONTRACT.md`.

R17-I1 is VERIFIED_COMPLETE at
`d5d27bdd6db797d04d32a26122e18a76b3206509`: all nine workflow runs are
terminal-success, and R0 portable core passed 54/54 tests.

R17-I2 is VERIFIED_COMPLETE at
`98cad9ff84124cea41d95c6beba8430e5ff2ff22`: all nine workflow runs are
terminal-success; R0 portable core passed 55/55, including generated encounter
import cases and C++ embed smoke.

R17-I3 is VERIFIED_COMPLETE at
`4f5f35c12dc484c61fa4c282738bd28440311a6c`: all nine workflow runs are
terminal-success; portable core passed 57/57 and Windows native atomic
transport passed 24/24. Its source-format export/preservation and transactional
write evidence is in section 18 of `docs/R17_SAVE_COMPAT_CONTRACT.md`.

R17-I4 is VERIFIED_COMPLETE at
`d58955c4c1e4c48dd243b81b874d60463e40c7cd`: all nine workflow runs are
terminal-success; portable core passed 58/58 and Windows native atomic
transport passed 24/24. Production VP5, explicit legacy recovery, stock runtime
state and metadata-version boundaries are in section 19 of the contract.

R17-I5 is VERIFIED_COMPLETE at
`ac50b51674d5707caa0a2ac1d237d687a0af6d9e`: all nine workflow runs are
terminal-success; portable core passed 60/60, Windows native read 32/32 and
atomic write 24/24. It separates typed MISSING/ERROR, rejects corrupt/unsupported images
and stale store checkpoints, and requires explicit source-format load selection.
Its shared native read transport, host status/path wiring and independent
fixtures are in section 20 of the contract.

R17-I5-Closure-1 closes the concrete failure in R0 run `37529156535` at
`825e53f904f4f7211280a341cdd00cc512ca9184`: Linux directory size probes could
return OK. The native transport now requires an opened regular-file descriptor;
local native C++17 and ASan/UBSan pass 32/32. Initial CI was 59/60 Linux,
31/31 Windows read and 24/24 Windows atomic write; the exact closure-HEAD CI
subsequently succeeded and closed I5. The failed initial commit was not accepted.

R17-T1 is VERIFIED_COMPLETE at
`f0db6e28a06ebd104f9e184c475a0ec359ca31f6`: all nine workflow runs are terminal-success;
portable full suite passes 61/61, Windows native read 32/32 and atomic write
24/24. It versions 37 public recipes and the hash-pinned private VP019 replay
evidence. Strict/sanitizer native import/export/disk-reopen and the independent
byte/domain oracle pass; limits are in `docs/R17_SAVE_FIXTURE_MATRIX.md`.

R17-V1 and R17-G1 are VERIFIED_COMPLETE at the same unchanged HEAD. All five
ROADMAP acceptance requirements are reconciled against production source,
actual real-save replay and exact-head CI. No R17 implementation RED remains.

R17-D1 records the complete evidence in
`docs/R17_SAVE_COMPAT_COMPLETION.md`. Resume at **R17-D1 verification** until
this documentation commit's exact live CI is terminal-success; then execute
**R17-F1 — Final Gate**. Final drift/docs/source/CI reconciliation, PR merge
and main CI remain separate required checkpoints. If F1 passes: R17-M1 ->
R17-M2. Only after merged main CI terminal-success is the next phase **R6-P1**.
Pending/failing CI blocks progression and permits only a concrete closure.

Real Unreal compilation/Android execution remains R18 work; native transport
checks are not a substitute for that gate.

P2 closure remeasured the production AGBCC layout: existing remaster block sizes
`0xF44` / `0x3DC8` are correct for pinned Vanilla+; the earlier P2 contract had
mistaken stale comments for compiled sizes. Production VP5 metadata is at
compiled `SaveBlock1.unused_3598=0x35D8`; I4 confines the old remaster literal
`0x3598` to explicit recovery rather than normal metadata access. Evidence and
format policy are in `docs/R17_SAVE_COMPAT_CONTRACT.md`.
I1 verified format-specific checksum spans, slot choice, sector
IDs/signatures, rotation and the explicit counter-coherence policy. No
gameplay/implementation change belongs to P2.

## Current R6 checkpoint — 2026-10-07

This checkpoint supersedes the earlier R17 resume pointer above. R17 is merged
at main `a7ed422da7817c05aa1eac21bdbca6bd6edced5e`. User-directed continuation
completed the remaining R6 implementation slices in one cohesive branch commit:
`f9dbf2d20bb6bd38ae921a9aa2225aa969c984b8`.

R6-I3/I4/I5/I6, T1, V1 and G1 are VERIFIED_COMPLETE for the pre-real-Unreal
source/preparation scope. Hosted run 37596293052 completed with success:
52 R6 tests, all 181 Python regressions, source/package guards and 62/62 full
CMake/CTest checks. No commercial payloads were added.

R6-D1 evidence and F1 reconciliation are in `docs/R6_PRESENTATION_COMPLETION.md`.
Verify this documentation checkpoint's exact committed-HEAD CI before the
separately authorized R6-M1 PR/merge and R6-M2 main CI gates. Current authorization
retains the PR/merge/main boundary; automatic phase advancement does not override
that restriction. After R6-M2, next is R7-P1. Actual Unreal/UHT, normalized asset
imports, clips/retarget/props and Android validation remain external R18 work.


## R7 direct-authorization checkpoint — 2026-10-07

The user explicitly instructed completion of R7 while R6 merge/main remained
outside current authorization. The dependent R7 branch therefore starts from R6
`db8c50c76365bd656fc5db3bad2eba25a336b83c`; this is a direct user-authorized
preparation-sequence exception. Existing R6/R7 M1/M2 boundaries remain.

R7-P1/P2, I1-I7, T1, V1/G1 and D1/F1 source/preparation implementation and
acceptance are VERIFIED_COMPLETE on the dependent branch. Full registry
coverage: 518 maps, 441 layouts, 75 tilesets, 18,318 source descriptor identities
plus 85 missing-descriptor visible fallbacks in an unused layout. No used map has
an unresolved identity. Zero modern 3D environment imports are claimed.

Local validation passed 21 R7 Python tests, all 202 Python regressions and 53 shared
native presentation checks. Hosted run 37601156961 completed with success at
`0e9133de277bb9f884e7d32105cead205b9034f1`: all 202 Python tests, deterministic
package/source guards and 63/63 full CMake/CTest checks. Independent raw-source
reconciliation verified all 324,579 active tiles and all 518 map records; all
23 published blobs match local bytes. Verify this final documentation-head CI
before any separately authorized R6/R7 M1/M2 integration. D1/F1 evidence, actual-engine deferrals and dependency details:
`docs/R7_ENVIRONMENT_COMPLETION.md`. Next preparation phase after R7 is R9; do not
start it under this R7-only instruction.

## R9 direct-authorization checkpoint — 2026-10-07

The user's next-phase completion instruction supersedes the R7-only resume
restriction for R9 source/preparation. Dependent branch `r9-ui-presentation`
starts from verified R7 head `32850179f41be4c12e2921d956fa52f7f0f0536b`.
R6/R7/R9 PR/merge/main boundaries remain separately authorized integration gates.

R9-P1/P2, I1-I8 UI infrastructure, T1, V1/G1 and D1/F1 are VERIFIED_COMPLETE
in the pre-real-Unreal source/preparation scope. Shared native screen/action
model, const core views, staged native commands, typed script request hooks,
native UMG safe-area/scroll/marker shell and per-action input fallbacks are ready.
Actual script resource/naming/field-effect hosts, R6/R7/R15 animation/audio timing
and real Unreal/UHT/touch/Android checks remain explicit integration/runtime work.

Local acceptance: 10 R9 Python tests, all 212 Python regressions, 215 native UI
checks and the 10-screen native fixture matrix passed. Hosted run 37635394388
reached terminal success at `0c99764d960654fe0c71309d9cd4946362b76101`: all 212
Python tests, source/catalog guards and 65/65 CMake/CTest checks. All 21 published
blobs match local bytes; no core/vendor files changed. Verify this final
documentation-head CI before handoff. D1/F1 details: `docs/R9_UI_COMPLETION.md`.
Main is `a7ed422da7817c05aa1eac21bdbca6bd6edced5e`; next preparation phase is R14,
which does not start under this R9-only instruction.

## R14 direct-authorization checkpoint — 2026-10-07

The user's explicit R14 start instruction supersedes the previous R9-only resume
boundary. Dependent `r14-battle-presentation` starts from verified R9 head
`4b0da018913867e9f8e83d76c0cb3662501ba3cd`; main remains
`a7ed422da7817c05aa1eac21bdbca6bd6edced5e`. Existing PR/merge/main gates remain.

R14-P1/P2, I1-I8, T1, V1/G1 and D1/F1 are VERIFIED_COMPLETE in the
pre-real-Unreal source/preparation scope. Local acceptance passed 24 targeted
Python tests, all 236 Python regressions, 1,251 native checks, byte-exact generation
and source guards. Hosted run 37643401453 reached terminal success at
`f1d7a44c575723ddc3a22c462b276f8557fa59a6`: all 236 Python tests and 66/66 full
CMake/CTest checks. All 23 blobs match local bytes, the sole parent is the R9
head above and no core/vendor files changed. Verify this documentation-head CI
before handoff. The source audit covers 386 species,
832 exact form/skin keys, 93 trainer pictures and 25 canonical event/VFX kinds.
All 386 modern species models are missing; zero Unreal imports are claimed.

Contract, local pipeline, event-batch snapshot limits, runtime deferrals and
evidence: `docs/R14_BATTLE_PRESENTATION_COMPLETION.md`. R14-M1/M2 remain pending;
next preparation phase is R15, which does not start under this instruction.

# Current resume point — 2026-10-08

The current user instruction authorizes applying the full agreed plan. Historical
phase-only preparation restrictions above describe their earlier checkpoints.
Canonical branch/PR/final-gate/main-CI rules and the R18 runtime stop remain.

Main: `ab057754f97fc1d8f5e9396ff8499337bb533dc3`, R19 PR #23.
R6 → R7 → R9 → R14 → R15 → R8 M1/M2 are VERIFIED_COMPLETE; each exact main CI
reached terminal success before the next integration (including CodeQL).
The versioned merge/CI ledger is in [R8_INPUT_INVENTORY.md](R8_INPUT_INVENTORY.md).
R19 source acceptance and M1/M2 are complete. R20 is active on
`r20-production-polish` from the verified R19 main.
Main workflow 37853379312: 8 targeted / 75 full CTest, 328 Python/source PASS;
all 15 main workflows, including CodeQL 37853378547, completed successfully.
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
all 16 workflows succeeded. R20-I2 VERIFIED_COMPLETE at f8425149;
workflow 38014432391: actual 81/400/171, all 16 workflows succeeded.
R20-I3 VERIFIED_COMPLETE at 8a809ef1; actual generation and 81/408/171;
all 18 workflows succeeded. R20-I4 VERIFIED_COMPLETE at aefbe4cf;
actual trusted generation/verification: 2,473 files / 13,093 source inputs.
R19 workflow 38015468972 passed 81/418/171; all 18 exact-head workflows succeeded.
R20-I5 VERIFIED_COMPLETE at d4de20e1; workflow 38016057425: actual 81/418/171,
identical trusted package hashes; all 18 exact-head workflows succeeded.
R20-I6 VERIFIED_COMPLETE at 8c940d8b; actual 81/431/171, complete source/history
audit zero findings/skips; all 18 exact-head workflows succeeded.
R20-I7 VERIFIED_COMPLETE at 14f2cbf3; actual 81/443/171, complete package/security/
source-release checks; all 18 exact-head workflows succeeded.
R20-T1/V1 at b19fc382: NEEDS_CLOSURE; actual R19 81/451/171 passed, but
real negative harness expected the wrong CLI exit. G1/merge remain blocked.
R20-Closure-1 / T1 / V1 / G1 VERIFIED_COMPLETE at 1c671677;
workflow 38025616374 passed actual 81/453/171 + 53 R20 tests, two independent
clean packages, nine real negatives and complete zero-finding/skip security.
All 18 workflows / 32 check runs succeeded. Current checkpoint: **R20-D1**,
completion publication VERIFYING; then independent F1 -> PR/main M1/M2.
[Accepted R20 source scope and actual evidence](R20_PRODUCTION_COMPLETION.md).
[Named implementation evidence](R20_PRODUCTION_IMPLEMENTATION.md).
[Production inventory and named gaps](R20_PRODUCTION_INVENTORY.md).
[Accepted R19 source/runtime boundary](R19_REGRESSION_COMPLETION.md).
[Inventory](R19_REGRESSION_INVENTORY.md), [contract](R19_REPLAY_CONTRACT.md),
[evidence](R19_REGRESSION_IMPLEMENTATION.md).

R15-V1-Closure-1 is VERIFIED_COMPLETE at
`3b9b208f934ecba9ac9be857cd40e38e88cbfec0`.
Workflow 37763598815: 35/35 targeted Python, 271/271 full Python, 67/67 CTest,
zero skips; required FFmpeg transform ran in both Python suites.

R15-I3-Closure-1 is VERIFIED_COMPLETE at
`9df9fae1d56420259f9d48ab950183800a5eea06`.
Local historical master/normal hashes, full special manifest, 5,018 private
candidates and both ZIP contents passed. New container hashes are explicit.
Published-head workflow 37766890311 reached terminal success: 43/43 targeted
Python, 279/279 full Python, 67/67 CTest, zero skips. Evidence and reproduction:
[R15_AUDIO_PACK_RECOVERY.md](R15_AUDIO_PACK_RECOVERY.md).

R15-I2-Closure-1 is VERIFIED_COMPLETE at
`4f8ef9a9769b0f4211e129c80e0dc6ae8181e72e`.
Source recipes/timelines cover 191 real music jobs, 18 jingles and 80 reserved
zero-track rows; historical resolver/cry pack contracts remain unchanged.
Local 13/13 tests and all 209 compiled pinned-converter recipe oracles pass.
Exact-head workflow 37769695700 reached terminal success: 56/56 targeted Python,
292/292 full Python and 67/67 CTest; zero Python skips. Evidence:
[R15_BGM_SOURCE_PLAN.md](R15_BGM_SOURCE_PLAN.md).

R15-I2-Closure-2 is VERIFIED_COMPLETE at
`2ae0e2602ce3e01479ddd748bbce84e26059ffd9`.
Four actual private candidates passed source blob
integrity, 707 renderer engine tests, decoded PCM probes and native/rational
loop-frame comparisons. Fresh-build WAV hashes repeat exactly; 12/12 new local
regressions pass. Exact-head workflow 37777810560 completed successfully:
68/68 targeted Python, 304/304 full Python and 67/67 CTest, zero Python skips. Evidence:
[R15_BGM_RENDER_PILOT.md](R15_BGM_RENDER_PILOT.md).
R15-I2-Closure-3 is VERIFIED_COMPLETE at
`5a5475103b736ae5d9c8b0abf442238b8edc05be`.
The private full-coverage pipeline binds the 380 bank inputs and all
209 MIDI inputs to versioned source receipts, reuses the pinned renderer and
keeps runtime-loop/listening/import readiness false. Exact-head workflow
37779942858 passed 74 targeted Python, 310 full Python and 67 CTest, zero skips.
Evidence: [R15_BGM_RENDER_COVERAGE.md](R15_BGM_RENDER_COVERAGE.md).
R15-G1/D1 is VERIFIED_COMPLETE at `695d01b56b6afdc2d73ad8b066ca12479125549a`;
workflow 37780786306 passed 74 targeted Python, 310 full Python and 67 CTest,
zero Python skips.
The canonical four source acceptance bullets pass; runtime/local asset obligations
remain explicitly open in [R15_AUDIO_COMPLETION.md](R15_AUDIO_COMPLETION.md).
R15-F1 publication `515dde0` passed workflow 37781258584: 74 targeted Python,
310 full Python, 67 CTest, zero skips. PR #21 merge and main CI then passed.
R8-P1 is VERIFIED_COMPLETE at `261cef11`: versioned source/action inventory,
no implementation changes and no push workflow trigger.
Historical R8-P2 — Platform/lifecycle contract, frozen in
[R8_INPUT_CONTRACT.md](R8_INPUT_CONTRACT.md) and `data/r8/input_contract.json`.
This is a policy/doc checkpoint; no input/gameplay behavior changes.
R8-P2 is versioned at `d9771a8d`; I1 is VERIFIED_COMPLETE at `1d45a91d`.
Workflow 37817943261 passed targeted 1/1 and full 68/68 CTest, 310/310 Python
and source guards; all 16 exact-head workflows including CodeQL passed.
R8-I2 is VERIFIED_COMPLETE at `c18d820c`; workflow 37846220404 passed targeted
2/2 and full 69/69 CTest, 310/310 Python/source checks; all 16 workflows passed.
R8-I3 is VERIFIED_COMPLETE at `a39c97a3`; workflow 37847802114 passed 3/70/313.
R8-I4 is VERIFIED_COMPLETE at `3f04104e`; workflow 37848753892 passed 4/71/317.
R8-I5 is VERIFIED_COMPLETE at `6046cba6`; workflow 37849310238 passed 5/72/320.
R8-I6 is VERIFIED_COMPLETE at `7194102a`; workflow 37849841175 passed 6/73/322.
R8-T1/V1 passed at `d1730592`; workflow 37850579912 passed 7/74/326.
R8-Closure-1/V1/G1 is VERIFIED_COMPLETE at `19f8716d`; workflow 37851201049
passed 8/75/328 and source guards. R8-D1 is VERIFIED_COMPLETE at `ddd3ce19`;
workflow 37851617574 passed 8/75/328; all 16 workflows including CodeQL succeeded.
R8-F1 is VERIFIED_COMPLETE at `ea582534`; all 16 final-head workflows succeeded. [Acceptance/runtime boundaries](R8_INPUT_COMPLETION.md).
R8-M1 PR #22 merge and R8-M2 exact-main terminal-success are VERIFIED_COMPLETE.
R19 pre-real source and integration gates are complete; R20-D1 publication and remaining F1/M1/M2 gates are active.
R18 has not started.
The historical phase-only restrictions above are superseded by the current
full-plan authorization. Continue named subphases/closure gates through R19,
R20 and R18; stop at the actual environment/runtime boundary. All deferred
asset/host/engine/device obligations remain explicit.

R20-I5 canonical documentation links [BUILD.md](BUILD.md) and the production
package contract. The R6/R7/R9/R14/R15 completion documents preserve their
historical implementation snapshots under explicit superseding integration
banners; old per-chat permission restrictions do not govern current execution.

# Latest resume — R18-P1 actual environment boundary, 2026-10-10

R20-D1/F1 VERIFIED_COMPLETE at 56e14d74: actual 81/453/171 + 53 targeted,
18 workflows / 32 checks terminal-success. R20-M1 PR #24 merged at 466aa182;
R20-M2 verified identical accepted/main tree and all 17 workflows / 30 checks,
including actual main R20 workflow 38039922255 / job 114177936315.
R20 source/integration gates are complete. Historical pending snapshots above
are superseded by this live integration evidence.

CURRENT_STATE: **R18-P1 — EXTERNAL_ENV_REQUIRED / BLOCKED**.
[Actual environment audit](R18_ENVIRONMENT_AUDIT.md) is the owning checkpoint.
Cloud Linux lacks project-PC control, accessible verified UE 5.8.3 and Android
SDK/NDK/JDK setup. No actual engine/build/device operation ran. R18-P2 onward
remains NOT_STARTED; source-only CI cannot close this actual gate. Stop at this
exact checkpoint, resume P1 only with the real attached environment, and retain
the later REAL UNREAL RUNTIME VALIDATION / physical-device STOP contract.
