# R19-P1 — Regression infrastructure inventory

Scope: pre-real-Unreal source/test orchestration only.
State: P1 source inventory complete; versioned publication is its closure gate.
Active branch: r19-regression-infrastructure.
Audited baseline: 7e8d027d6c1f00677b0f270d56bfe7d4de83590d.
Baseline tree: 1d80571a5ecef3cb233b563c285b2795fb5198bb.
Production source pin: illetyus/pokezumrut-vanillaplus@
70db90c9077aed1272e746fc2537d9f12b95a91c.

## Recovery and prerequisite evidence

Live main and phase branch both resolve to the audited baseline, compare 0 ahead /
0 behind / no changed files. R8 PR #22 is merged; its accepted final head is
ea5825341b7b344199ac28679e1a90b02da1f713. Main tree equals that accepted tree.
All 15 exact-main workflows are completed/success, including dynamic CodeQL
37853378547 and its actions/c-cpp/csharp/python analyses. Main R8 workflow
37853379312 jobs 113571650319 and 113571650592 report 8 targeted CTest,
75 full CTest, 328 Python tests and Unreal source architecture validation PASS.
R8-M1/M2 are VERIFIED_COMPLETE. No conflicting concurrent drift was found.
Other open PRs #8 (BrowserStack), #4 (ImgBot), #2 (historical draft R0) are outside
this checkpoint.

## Existing entry points and coverage

The audited test subtree contains 144 versioned blobs, including 70 C/C++ source
files and 45 Python test modules. CMake defines 75 actual CTest entries.
The tools subtree contains 48 blobs; data contains 29 versioned JSON artifacts.
These are inventory counts, not test execution counts.

| Authority / phase | Reusable entry points | Existing evidence boundary |
| --- | --- | --- |
| R0 | core_smoke, core_replay, platform_mechanics_contract | Historical small RemasterState replay; not the production Emerald state hash |
| R1/R17 | emerald_save_compatibility, sector/domain/import/export/metadata/platform/atomic-file tests, r17_save_fixture_matrix | Matrix runs the compiled save probe; private VP019 replay is separately evidenced, never uploaded |
| R2 | emerald_script_vm/runtime/checkpoint/object, r2_opening_commands, r2_littleroot_integration | Pinned source conversion, male/female opening scripts and typed host completions; bounded slices, not full game |
| R3 | r3_full_world_probe.py; source/script/encounter catalogs; content audit and world_fingerprint.py | Full-Hoenn converted data integrity; canonical JSON hashes, not gameplay state hashes |
| R4 | movement/collision/events/warp/object tests, r4_littleroot_overworld | Real source-generated house/town/Route101 movement fixture; seed/preconditions explicit |
| R5 | metatile/world-grid native checks; descriptor/render/package/scene Python audits | Presentation geometry and source package contracts |
| R10 | r10_quest_progression, r10_quest_save_roundtrip, catalog audit | Objective derivation and persistence against authoritative flags/vars |
| R11 | codec/party/storage/bag/catalog checks | Pokemon and item domains, encrypted persistent data |
| R12 | rng/rules/ecosystem/special/replay/catalog tests | Fixed-seed encounter replay, immunity and Repel RNG ordering; needs representative matrix orchestration |
| R13/R16 | battle/AI/replacement/progression/effects/move-effects/encounter integration/replay; QoL behavior/source/decisions | Wild win, trainer replacement/win/loss replay; current raw-struct memcmp is same-build evidence only |
| R6/R7/R14/R15 | Native presentation readers/policy; Python manifest/package/source/receipt audits | Source contracts and synthetic/private asset evidence; no actual cooked assets, listening or UE execution |
| R9 | Native UI model and compiled fixture matrix | Real core read/validated action boundary; no UMG runtime certificate |
| R8 | Eight compiled CTest entries and 18 Python checks | 47 cases / 221 observations; first divergent input observation; not complete production state replay |

## Versioned fixtures, snapshots and provenance

- R2 fixture source tree pins VANILLAPLUS_COMMIT.txt and actual opening script,
  constants, command-table and macro inputs. CMake generates its native registry.
- R3 vanillaplus_catalog.json pins the converted world catalog.
- R4 CMake uses build_r4_littleroot_world_fixture.py against vendor/vanillaplus
  to generate real map/layout/event inputs, not a fabricated replacement map.
- R17 tests/fixtures/r17/matrix.json specifies synthetic recipes, source blob
  identities, import/export results and private real-save provenance. The private
  save bytes are outside the repository and are optional in public CI.
- R9 ui_expectations.json and R8 input_matrix.json execute compiled probes.
- R6/R7/R14/R15 data contracts and R15 source/pack/render receipts remain
  versioned metadata only. Private payloads are not required public CI inputs.
- Script runtime already exposes a versioned checkpoint serializer. Reuse it for
  VM state; never hash C padding, addresses or presentation-owned clocks.

## CI orchestration and current limits

Twenty workflow files are present. Fourteen production source phase workflows
plus dynamic GitHub CodeQL ran on this main; historical SDL/Godot Android/desktop
experiments are separate. r0-core.yml runs the full CTest suite, Windows native
atomic/file-read checks, converter checks and the R3 full-world probe.
r8-input.yml runs full CTest and Python discovery, mandatory FFmpeg/ffprobe and
the Unreal source guard. Thus the 75/328 baseline is actual hosted evidence.

Most phase push triggers cover their own phase branches; the new r19 branch
does not implicitly inherit those push triggers. A main-targeted PR runs earlier
phase workflows. A dedicated R19 workflow/entry point must make execution explicit.
No workflow may write implementation commits.

unreal-build.yml is workflow_dispatch on a self-hosted linux/unreal-5.8 runner.
It describes real build/cook/package commands; its presence proves no available
runner or successful UE5.8.3 build. Do not dispatch it as a source validation
substitute or expose an untrusted fork to a self-hosted machine.

## Gaps and owning named checkpoints

| Checkpoint | Required closure |
| --- | --- |
| P2 | Versioned replay commands, initial/data/source provenance, canonical hash domains, bounded execution and first-divergence contract |
| I1 | Ordered production-authority replay runner; reject invalid commands and incomplete execution without silently skipping |
| I2 | Padding/address-independent state hash and first step/domain mismatch diagnostics; immutable expected observations, no automatic blessing |
| I3 | Explicit source-backed opening/world/progression slices using R2/R3/R4/R10; setup preconditions separated from gameplay inputs |
| I4 | Execute existing R17 compiled compatibility matrix in the aggregate, preserving private-fixture boundary |
| I5 | Representative deterministic R12 seeds/scenarios and RNG-consumption observations |
| I6 | Representative R13 seeds/scenarios including replacement/loss and accepted R16 deltas |
| I7 | Aggregate source manifest audits for R6/R7/R14/R15 without requiring commercial payloads or asserting import readiness |
| I8 | Generated package/manifest/hash integrity and deliberate corruption detection |
| I9 | Source smoke marker/harness contract; clearly deferred actual UE executable result |
| I10 | Later Android API/GPU/ABI/device matrix metadata; no physical/BrowserStack execution now |
| T1 | One deterministic full-suite command/workflow with fail-fast component diagnostics and required dependencies |
| V1/G1/F1 | Actual CI execution, roadmap acceptance and independent fresh reconciliation before merge |
| M1/M2 | PR integration then all exact-main CI terminal-success before R20 |

A full-game end-to-end story playthrough, actual UE hosts/assets, executable
smoke, performance and final device acceptance remain absent. Representative
portable slices must not be labelled those results. No implementation or
gameplay/persistent layout behavior is changed by this P1 checkpoint.
Next: R19-P2 — Replay/hash contract.
