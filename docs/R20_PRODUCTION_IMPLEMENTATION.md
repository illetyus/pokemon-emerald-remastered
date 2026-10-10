# R20 — Named implementation evidence

## R20-P1 — Production-tree audit

VERIFIED_COMPLETE at a7db2bbb07e2cfdbf0021db9949b3c6de8007881.
Only four docs changed; live refs/compare, exact published strings/blobs and
scope were verified. Source inventory: 524 non-vendor blobs, 270 relevant source
blobs verified, complete module reference search. Baseline main all 16 workflows
succeeded; actual 81 CTest / 400 Python / 171 asset audits. PR #24 is draft.
Inventory and explicit I1-I7 gaps: R20_PRODUCTION_INVENTORY.md.
P1 exact-head PR workflow 38013904950 / job 114099865660 completed/success:
actual 81 CTest, 400 Python, 171 asset audits and all seven source components.
All 16 exact-head workflows succeeded before I1 publication.

## R20-I1 — Historical presentation excluded from production

State: VERIFIED_COMPLETE at 24e8779f18d16ee2387f1dc35c39d5aef9b9aec5.
Exact workflow 38014100330 / job 114100459405: actual 81/400/171 and all seven
source components passed, zero skips. All 16 exact-head workflows succeeded.
All ten published file records match; four moves preserve original Git blobs.
Before: unused R0HUD and R0PlayerController UCLASS pairs lived in unreal/Source
and were compiled/scanned by the production Unreal module despite having no
consumer outside their own pairs. The configured R0GameMode already selected
RemasterPlayerController/RemasterOverworldPawn and HUDClass nullptr.

Change: move four files intact to experiments/unreal-r0-reference, outside UBT/
UHT discovery. Preserve the active R0GameMode, RemasterCoreSubsystem platform/
save bootstrap, all portable C/C++ authority/fixtures and existing historical
SDL/Godot experiments. No gameplay, persistence or live input behavior changes.

Original/preserved Git blobs:
- R0HUD.h: 8ede80c6a433ac8e6f143d4f69c0ef310e9acaeb
- R0HUD.cpp: 668b912bf1f6c87eae8272826b074aeaeae4ab02
- R0PlayerController.h: 68ba3fe96735ba5250696a937d5beadceef9bd50
- R0PlayerController.cpp: 49493b54e4711c1f5f15b1467869f8d090d670cf

Source boundary regression: the existing Unreal architecture validator now
rejects any of the four historical files inside the runtime module and requires
the preserved reference paths. Before the move it failed with all four expected
messages (RED). After the move it passed; all four old/new byte sequences and
Git hashes are identical (GREEN). No new implementation-mirroring test was added
for this reversible move. Actual full portable/source CI remains mandatory.

Production module dependency/embedded-core and default R4/R8/R9 host guards
continue to pass locally. No real UE/UHT compile or private cooked asset
acceptance is claimed; that remains R18/runtime evidence.

Next: verify this exact published HEAD's R19 full-suite and earlier-phase source
guards, then R20-I2. M1/M2 remain NOT_STARTED.

## R20-I2 — Required production dependency surface

State: VERIFIED_COMPLETE at f842514922b7ea6a85910da7bac4954db43adaf8.
Exact workflow 38014432391 / job 114101460871: actual 81/400/171, seven components
and zero skips. All 16 exact-head workflows succeeded; all five blobs match.
The complete Unreal module source/header scan found no JsonUtilities or Projects
consumer; only Build.cs declared them. Remove those two explicit dependencies,
retain every currently consumed module, and document public source tools versus
engine/private/historical inputs. No engine-owned SDK/NDK/JDK version is invented.
No gameplay, core, vendor, generated owner catalog or package identity changes.
Local existing Unreal source authority guard passes with the retained modules.
The dependency removal is a reversible source cleanup; no mirror unit test is
added. Full portable/source CI is required, and actual Unreal linking remains R18.
Next after exact CI: R20-I3 production package generation/layout.

## R20-I3 — Fresh production package generation/layout

State: VERIFIED_COMPLETE at 8a809ef1c27c3890c250992276b28ca7d4333701.
R20 workflow 38014899367 / job 114102889539 passed all eight isolation tests and
actual complete owner generation. R19 workflow 38014902328 / job 114102898257
passed actual 81 CTest / 408 Python / 171 assets, seven components, zero skips.
All 18 exact-head workflows succeeded; all seven published blobs match.
Missing entry point was RED; the eight new output isolation/transaction tests
initially failed to import that missing module. After implementation all eight
pass locally on explicit synthetic callbacks. Actual default production source
generation is NOT_RUN locally because this workspace is a partial source cache.
Hosted R20 source-packages runs the real complete source pipeline, and R19 runs
full actual native/Python/asset/source regressions after publication.

One command delegates to unchanged R3/R5/R6/R7 owners, source/audit checks and
compiled R9/R14/R15 catalogs. Four runtime directories preserve owner formats.
Output is staged transactionally: existing/source/symlink destinations are
rejected; failed/missing-owner and concurrently created output remain safe.
The accepted vendor tree/clean working state is checked before default generation.
No vendor/gameplay/owner-format change or private payload upload/self-commit.
I4 aggregate index/provenance/verification, I6 leakage and I7 staging/preflight
remain open. No real UE/UHT/compile/cook/package/device evidence is inferred.
Next after actual exact-head CI: R20-I4.

## R20-I4 — Whole-package bytes and trusted input provenance

State: VERIFIED_COMPLETE at aefbe4cf761d076e491b6856f520c64ffaf45f6b.
R20 workflow 38015465245 / job 114104626001 passed actual generation and trusted
verification: 2,473 files / 13,093 input files; index SHA-256
33ae24747001bd35dbb2a4ffebed3f541596104f82b8be1c59f7c5f2128ff359.
R19 workflow 38015468972 / job 114104637518 passed actual 81 CTest / 418 Python /
171 asset audits, seven components, zero skips. All 18 exact-head workflows
succeeded and all eight published files match verified strings/blobs.
Ten new integrity regressions initially failed for the absent aggregate module.
All 18 R20 isolation/integrity tests pass locally on explicit tiny fixtures.
Actual complete generation/seal/verification remains the hosted default pipeline.

The canonical index covers every owner file and a conservative complete tracked
source/generator/input inventory. External generation receipt binds index SHA-256;
rewriting both file and adjacent index cannot self-bless. Unsafe/missing/extra/
corrupt/symlink/unsupported/noncanonical data and source/input drift fail.
Default source inputs must be clean, are measured before/after generation, and
are sealed inside I3's transaction before publication. No owner format, vendor,
gameplay/save field change or private payload upload. Runtime certification false.
CLI --verify without an explicit trusted hash fails before touching output.
Next after actual exact-head CI: R20-I5 documentation/prerequisite reconciliation.

## R20-I5 — Canonical build and prerequisite reconciliation

State: VERIFIED_COMPLETE at d4de20e159cc6f786f0b7ed634b83c7ac8566af2.
Exact R19 workflow 38016057425 / job 114106441693: 81 CTest / 418 Python / 171
assets, seven components, zero skips. R20 workflow 38016054252 actual generation
and trusted verification repeat I4's identical index/package hashes and counts.
All 18 exact-head workflows succeeded; all 12 documentation blobs match.
README/Unreal guide now identify completed source main phases and the exact
R20 -> R18 -> STOP boundary. BUILD.md lists every public clean-checkout/tool/
generation/verification requirement and all optional private save/model/audio
instructions. No private input is mandatory for public source acceptance.
Canonical docs distinguish current main integration from historical snapshots;
phase evidence gains explicit superseding banners without deleting its history.
No gameplay, vendor, owner package, dependency or runtime configuration changes.
Existing source/full CI and package generation remain actual verification; no
implementation-mirroring tests were added for documentation edits.
Next after exact-head CI: R20-I6 leakage/security.

## R20-I6 — Public source leakage/security boundary

State: VERIFIED_COMPLETE at 8c940d8bfa417df3ef94323dc1328e7947cf43f7.
Actual R20 workflow 38016578180 / job 114108075307: 31 targeted tests; package
2,473 files / 13,094 inputs verified. Security: 13,376 tracked files, 13,107
unique reachable blobs / 103,149,273 bytes, zero findings/skipped blobs.
R19 workflow 38016582804 / job 114108087985: actual 81/431/171, seven components,
zero skips. All 18 exact-head workflows succeeded; all nine published blobs match.
Absent security module was RED; 13 temporary-Git negative/coverage regressions
now pass locally. Full real-repository scanning is NOT_RUN in the partial cache.
The default hosted audit measures tracked paths and all reachable HEAD blobs,
including vendor/binary bytes, redacts matched values and fails on incomplete
coverage. Expanded ignore rules protect missing World/Render/index and private
saves/ROMs/archives. Manual Unreal jobs are trusted-main only; APK upload removed.
No gameplay, vendor, owner format or native package configuration changes.
Scope/limits and exact run command: R20_PUBLIC_SOURCE_SECURITY.md.
Next after actual CI: R20-I7 release configuration and real-install preflight.

## R20-I7 — Release source staging and installation preflight

State: VERIFIED_COMPLETE at 14f2cbf3606729b929a8f9411f0aed940b290611.
R20 workflow 38024760954 / job 114133135123: 43 targeted tests, complete package
2,473 files / 13,095 inputs verified; security 13,379 paths / 13,118 blobs,
103,266,921 bytes, zero findings/skips; source release intent passed.
R19 workflow 38024762455 / job 114133140070: actual 81/443/171, seven components,
zero skips. All 18 exact-head workflows succeeded; all 11 published blobs match.
Missing preflight module was RED; 12 fake-installation/source regressions pass.
World/Render now stage with Characters/Environment; Entry/basic-shape cook intent
is explicit. Source Android identity/ABI/SDK values are retained. Installation
preflight requires exact 5.8.3 metadata and complete target tools; missing Android
prerequisites fail instead of warning. Linux/Win64 host paths are distinguished.
Manual trusted-main workflow prepares/verifies content before real commands;
no actual dispatch/build, imported asset readiness or APK result is claimed.
Source-only and installation-presence receipts explicitly deny actual build and
toolchain compatibility. Source/docs details: R20_RELEASE_PREPARATION.md.
Next after actual CI: R20-T1 clean reproduction and security/integrity negatives.

## R20-T1 / R20-V1 — Clean independent reproduction and full source suite

State: VERIFYING actual hosted default execution after publication.
Eight receipt-gate regressions were RED for the absent full-suite entry point;
all 51 targeted R20 tests now pass locally. Full real native/source/checkouts
are NOT_RUN in this partial cache. The new default entry runs the entire R19
suite plus mandatory R20 regression coverage, two real clean detached worktree
productions and external-witness verification, nine real rejection/restoration
cases, actual whole tracked/history audit and source release check. No mock
generator/default fixture replaces actual production. Metadata-only CI upload.
Contract: R20_REPRODUCTION_CONTRACT.md. G1 waits for actual terminal-success;
concrete failures use named closure, then repeat V1/G1.

## R20-Closure-1 — Actual production validation exit contract

T1/V1 at b19fc382 entered NEEDS_CLOSURE: R20 workflows 38025287826/38025289611
failed at clean_package_reproduction after actual R19 81/451/171 and all 51
R20 tests passed. G1/merge did not advance. Production validation correctly
returned exit 1; the negative harness incorrectly required parser/misuse exit 2.
The unchanged CLI's malformed-index invocation reproduces exit 1 locally.

State: RUNNING_CLOSURE -> VERIFYING actual default hosted rerun.
Two new regressions pin the real CLI contract and require both exit 1 and the
exact case-specific validation message. Success, parser errors, traceback/crash
and wrong validation reasons fail. The old harness was RED for that contract;
all 53 targeted R20 tests pass after the narrow harness fix. Production validators,
generation, source inputs, core/vendor and acceptance boundaries are unchanged.
Next: actual V1 -> G1; no success is inferred from synthetic tests alone.
