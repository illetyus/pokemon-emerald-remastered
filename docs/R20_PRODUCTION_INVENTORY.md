# R20 — Production-tree inventory

Date: 2026-10-10
Checkpoint: R20-P1, source audit COMPLETE; versioned publication VERIFYING.
Branch: r20-production-polish.
Baseline main: ab057754f97fc1d8f5e9396ff8499337bb533dc3.
Baseline tree: 05df5b0099a258d00984afdaff9f860a58cfc117.
Authoritative Vanilla+: illetyus/pokezumrut-vanillaplus@
70db90c9077aed1272e746fc2537d9f12b95a91c.
Vendored snapshot tree: 5a551f1f9e40184278c57dfb8d25f68a0a1c99dc.

## Recovery and accepted handoff

Fresh live main, previous R19 branch, history/compare, open PRs and exact-main
CI were re-read. PR #23 is merged. R19 final head 416f817c passed workflow
38011903730 / job 114093555507: 81 CTest, 400 Python, 171 asset audits and all
seven components, zero skips; all 17 workflows / 31 check runs succeeded.
Merged main passed workflow 38012481196 / job 114095342449 with the same actual
counts; all 16 workflows / 29 check runs including four CodeQL analyses succeeded.
The merge tree equals the accepted final branch tree. No conflicting drift.
Open PRs #8/#4/#2 and old phase branches are unrelated and untouched.

## Audited tree and production boundary

All non-vendor subtree listings are complete, not truncated: 524 tracked blobs,
8,421,143 bytes. Counts: core 43, data 37, docs 56, experiments 20, external 1,
shared 1, tests 164, tools 57, Unreal 121, .github 21 and three root files.
270 relevant cached source blobs were checked against exact live Git blob hashes,
including all 121 Unreal files and the tooling/test sources used by this audit.
This is a source cache audit; the actual complete build/asset suite ran on GitHub.

Production is portable C/C++ authority plus the Unreal runtime module. SDL3 and
Godot remain under experiments, with CMake options OFF by default. Their pinned
reference workflows are historical, not Android production acceptance.

R0GameMode is the configured live bootstrap: it selects RemasterPlayerController,
RemasterOverworldPawn, world/NPC/environment/camera/battle actors, with no legacy
HUD. RemasterCoreSubsystem still installs the platform and is a declared save
subsystem initialization dependency. Its adapter/embed/mechanics and historical
R0 state remain explicit compatibility surfaces; do not delete based on names.
R0HUD and R0PlayerController have no source references outside their own pairs
across the complete Unreal tree. They are candidates to move intact outside the
production module; private cooked Blueprint migration is a real-engine concern.

## Findings assigned to named checkpoints

| Checkpoint | Concrete audit finding / smallest intended closure |
| --- | --- |
| I1 | Move the unused R0 HUD/controller pairs intact to historical reference material. Keep the live game mode, platform/save bootstrap, portable R0 regressions and default-OFF experiments. |
| I2 | JsonUtilities and Projects have no source consumer in the complete module. Remove those explicit dependencies; document retained modules, C99/C++17, CMake/CTest, Python, FFmpeg and engine-owned toolchain requirements. Historical Godot/SDL pins remain separate. |
| I3 | Four runtime packages are generated through separate commands: World, Render, Characters, Environment. Add one clean, safe production generation/verification entry point without modifying vendor, gameplay or in-place private assets. Native R9/R14/R15 catalogs remain checked source metadata. |
| I4 | Add a deterministic whole-package inventory and source/generator provenance tying actual complete files to the pinned input. Reject missing/extra/corrupt files and symlinks; preserve each owner's formats. |
| I5 | README describes an obsolete R14 branch/next-R15 state; per-phase roadmap summaries still show past merge-pending states; unreal/README puts physical-device smoke inside R18. Reconcile current docs and list every required public/private build input. |
| I6 | Generated World/Render outputs lack ignore/staging consistency; current ignore rules cover several private roots but not arbitrary ROM/archive/payload/output placements. Add tracked-tree leakage/secret checks with bounded, redacted findings and explicit source-fixture/vendor policy. |
| I7 | DefaultGame stages only Characters/Environment although runtime reads Generated/World and Generated/Render. Existing preflight accepts executable Build/UAT without checking exact 5.8.3 and only warns on missing Android tools. Prepare fail-closed source build gates and release metadata; preserve current package identity/API settings until real toolchain compatibility is verified. |

These are audit findings, not implementation success. No code, CMake, workflow,
vendor, binary asset, save format or gameplay behavior changes belong to P1.
Security audit is NOT_RUN until I6/T1; source inspection is not a secret-scan result.

## Required acceptance and evidence

I1-I7 remain separate scoped commits. Appropriate source/package/security
regressions precede or accompany each change; pure documentation does not need
implementation-mirroring tests. T1 runs repeatable clean package generation and
negative integrity/security cases. V1 runs the actual full R19 suite plus R20
package/security checks. G1 reconciles all four ROADMAP R20 acceptance bullets;
concrete gaps route through named closures, not an inferred completion claim.
D1/F1 reconcile actual logs, file scope, provenance, CI and live drift before
PR merge. M2 requires every exact-main workflow terminal-success.

Next: R20-I1 after P1 is actually versioned and exact changed-file scope verified.
After R20-M2: R18-P1 actual project-PC/UE/Android environment audit. Missing
environment -> EXTERNAL_ENV_REQUIRED. After actual R18/main success stop at
REAL UNREAL RUNTIME VALIDATION; no automatic physical Android, BrowserStack,
final device matrix or R21/R22. No actual UE compile/cook/package/device result
is produced by R20.
