# R8 — Input infrastructure source acceptance and completion

G1: **VERIFIED_COMPLETE** at `19f8716d9db199f7c7ae42f0ed214a6224503f74`.
D1: **VERIFIED_COMPLETE** at `ddd3ce193fb41a8db8beefd738ed6c1f5711c9d6`.
F1: **VERIFIED_COMPLETE** at `ea5825341b7b344199ac28679e1a90b02da1f713`;
all 16 final-head push/PR workflows including CodeQL succeeded.
M1/M2: **VERIFIED_COMPLETE**; PR #22 merged, all exact-main workflows succeeded.
Branch: `r8-input-infrastructure`. PR: [#22](https://github.com/illetyus/pokemon-emerald-remastered/pull/22).
Base main: `b8d789e697ee06c466c5546dfbebfb546b4e89e2`.
Authority: `illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`.
Contract: [R8_INPUT_CONTRACT.md](R8_INPUT_CONTRACT.md).
Detailed RED/native/source evidence: [R8_INPUT_IMPLEMENTATION.md](R8_INPUT_IMPLEMENTATION.md).
Matrix: [R8_INPUT_TEST_MATRIX.md](R8_INPUT_TEST_MATRIX.md).

## Canonical pre-real-UE acceptance gate

| Roadmap acceptance | Concrete evidence | Gate |
| --- | --- | --- |
| Platform-neutral action contract | Ten actions, bounded captures, focus/epoch/owner response policy; compiled Router and versioned schema/fixtures | PASS |
| Touch layout/source wiring | Ten nonoverlapping regions, normalization/capture, shared native endpoint and optional presentation InputConfig | PASS |
| Gamepad/keyboard equivalents | 25 compiled bindings pinned to schema; typed mapped Enhanced adapter with atomic per-key/stick coverage; Closure-1 preserves partial asset fallbacks | PASS |
| No Android-specific gameplay rule | Input classifies commands; existing R4 StepPlayer/R9/core request owners remain authority; no core/vendor or persistence/layout changes | PASS |
| Lifecycle-safe input reset | Independent background/inactive/pause, source epochs, platform flush, analog neutral, reconnect fence and owned delegate/context teardown | PASS |

Source gate closure: initial G1 found a partial Enhanced asset suppressing unmapped
native equivalents. Regression-first Closure-1 changed suppression to actual
physical-key coverage, rejected unsafe partial axes/conflicting reserved keys and
passed the new native/full matrix. That source acceptance gap is closed.

## Immutable source checkpoint ledger

| Checkpoint | Commit | Exact R8 push workflow | Targeted CTest / full CTest / full Python |
| --- | --- | --- | --- |
| P1 inventory | 261cef110a06fdedd9ec57dc66ffb026330d12e3 | Docs-only; no push trigger | Documentation validation |
| P2 contract | d9771a8da6611c9078dffb4aefeb4b62b1bac68c | Docs/data-only; no push trigger | Contract validation |
| I1 router | 1d45a91d64adc60ff250da1f2e122ec385d15ae4 | 37817943261 | 1 / 68 / 310 PASS |
| I2 touch | c18d820c4e34caa3b35a94c82595e1cc161f10f1 | 37846220404 | 2 / 69 / 310 PASS |
| I3 physical | a39c97a3387e079d04543aaae175c4dad2996ed3 | 37847802114 | 3 / 70 / 313 PASS |
| I4 focus | 3f04104edb78ec8e6e92cf6e152aaf85b48fbdd9 | 37848753892 | 4 / 71 / 317 PASS |
| I5 lifecycle | 6046cba6508a3624a80dcf99f6def2563fb49802 | 37849310238 | 5 / 72 / 320 PASS |
| I6 R9 repeat | 7194102a6dc6e991f8ef4ed2821b322acab580b2 | 37849841175 | 6 / 73 / 322 PASS |
| T1/V1 | d1730592c20decbf872d8d8a3dda72eeec6ac8c5 | 37850579912 | 7 / 74 / 326 PASS |
| Closure-1/V1/G1 | 19f8716d9db199f7c7ae42f0ed214a6224503f74 | 37851201049 | 8 / 75 / 328 PASS |

Final source implementation workflow jobs 113564239865 (native) and 113564240200
(Python/source) both succeeded. Actual job logs confirm 8 targeted CTest, all 75
CTest, all 328 Python tests with no skip report, and Unreal source architecture
validation PASS. The 47-case/221-observation fixture matrix executes the actual
compiled router/adapters/repeat bridge. R4/R9 authority regressions are included.
Local ASan/UBSan probes passed with leak detection excluded because instrumentation
prevents LeakSanitizer execution; no LeakSanitizer success is claimed.

No main/merge or final aggregate CI success is inferred from these source results.
F1 must reread live main/head, compare, docs/source/tests, reviews and exact CI;
M1 requires green final PR checks/CodeQL and M2 requires terminal-success main CI.

## Native input/host response boundary

One primary local controller leases native context/dispatch. Sources refresh host
facts before capture/delivery; one compiled selector invokes one owner. World
directions use existing R4 StepPlayer; modal/dialogue/recovery actions use accepted
R9. Field/battle callbacks are explicit native boolean owners and default to
unsupported. Host rejection never invokes another owner or Blueprint gameplay
fallback. Actual hosts validate pending core request/selection revisions and reset
or replace their owner on request changes.

R9's existing 60 Hz 24/3 Up/Down repeater uses common holds for all sources. Epoch,
focus, I/O, view/VM, map and setting changes clear counters/old presentation time.
It cannot repeat world steps, Confirm, battle commits or script completion.
MenuRepeat/FastText/touch configuration stay outside Emerald persistent domains.

## Deferred actual engine/host/device obligations

| Owner stage | Required actual evidence |
| --- | --- |
| R18 environment/build | Verified UE 5.8.3 project PC/toolchain; real UHT/C++ compile, cook, Android ARM64 package and package integrity |
| Cooked presentation/input assets | Actual Enhanced modifiers/triggers/mappings and layout resources; visible/usable touch controls and existing local R6/R7/R9/R14/R15 assets/imports |
| Real input/runtime validation | Device safe area/DPI, UMG pointer consumption, no duplicate controller/world touch delivery, keyboard/controller equivalence and latency |
| Platform lifecycle | Actual background/focus/pause/device callback order; flush/rebuild fresh-press behavior, neutral fence and timer/reset behavior |
| Authoritative request owners | Real field/script/battle host attachment, request/selection revision checks and resource ownership; no fake VM/battle success from callbacks |

These remain explicit obligations, not source-gate REDs. No UE/UHT executable,
Android package/device, BrowserStack or final device result is certified here.
After F1/M1/M2, next is **R19-P1**. Continue R19 → R20 → R18 under existing
authorization. If the real R18 environment is absent, stop EXTERNAL_ENV_REQUIRED.
After actual R18 and main CI, stop at REAL UNREAL RUNTIME VALIDATION; do not
automatically execute physical smoke, BrowserStack, final device matrix or R21/R22.


## R8-F1 — Fresh read-only final reconciliation

Re-read live main `b8d789e697ee06c466c5546dfbebfb546b4e89e2`, active branch
`ddd3ce193fb41a8db8beefd738ed6c1f5711c9d6`, recent commits, PR #22, compare,
canonical ROADMAP/ARCHITECTURE/PHASE_EXECUTION_PLAN and all owning R8 docs/source.
Compare was 11 ahead / 0 behind and 42 files. No concurrent drift, direct main
write, vendor/core edit, private payload or CI self-commit transport was present.
No unresolved review threads were present. This is a separate read-only
source/evidence reconciliation; it does not claim an external peer review.

D1 workflow 37851617574 jobs 113565626201/113565625848 confirmed 8/75/328 and
Unreal source guard PASS. All 16 exact-head push/PR workflows completed
successfully, including dynamic CodeQL workflow 37851617651 and actions, csharp,
c-cpp and python analyses. The final source implementation is unchanged since
Closure-1; D1 changed seven documentation files only.

The five source acceptance bullets, Closure-1 coverage, compiled/fixture proof,
owner rejection, lifecycle epoch and repeat/settings boundaries reconcile with
the versioned contract. Remaining actual engine/assets/host/device obligations
are carried above and are not falsely certified. Source final gate PASS.

This F1 documentation publication changes no implementation. Read its live
exact SHA/checks again after commit. Merge only after its final PR workflows and
required CodeQL contexts succeed. Then read the actual main merge SHA and all
main workflows to terminal-success. Only that M2 result permits R19-P1.

## R8-M1/M2 — Actual integration and main verification

PR #22 merged accepted head ea5825341b7b344199ac28679e1a90b02da1f713
into main 7e8d027d6c1f00677b0f270d56bfe7d4de83590d.
Merge tree 1d80571a5ecef3cb233b563c285b2795fb5198bb equals the accepted
source tree. Phase branch is preserved; live main...phase compare is 0 ahead /
1 behind / no file differences.

All 15 exact-main workflows completed successfully, including dynamic CodeQL
37853378547 and all four language analyses. No failed main check-run remains.
R8 main workflow 37853379312 jobs 113571650319/113571650592 confirm
8 targeted CTest, 75 full CTest, 328 Python tests and Unreal source guard PASS.
Main remained at the merge SHA in the post-CI live recovery.
R8 is VERIFIED_COMPLETE for its pre-real-UE source scope.
Next phase: R19-P1. The actual engine/assets/host/device obligations above remain.
