# R19 — Named implementation checkpoint evidence

Baseline main: 7e8d027d6c1f00677b0f270d56bfe7d4de83590d.
Branch: r19-regression-infrastructure.
Scope/remaining gaps: R19_REGRESSION_INVENTORY.md.
Replay/hash authority: R19_REPLAY_CONTRACT.md and data/r19/replay_contract.json.

## Closed planning checkpoints

P1 VERIFIED_COMPLETE at 915fee9253f93241b060882467b4742bd81d63d8:
four docs-only files; source/provenance/count validation PASS, no push trigger.
P2 VERIFIED_COMPLETE at 605a8378fe703ae841daa0e388da85ca80e0a952:
two contract artifacts; version/pin/domain/limit/immutable-expectation validation
PASS, no push trigger.

## R19-I1 — Ordered production replay

State: VERIFIED_COMPLETE at ab3196cca84e834629929ed826bc8c02071b7068.
Exact-head push workflow 37855418711 completed/success; actual job logs
113578298804/113578299112 confirm 1 targeted / 76 full CTest, 9 targeted /
337 full Python and Unreal source guard PASS.
RED: test_r19_replay.py failed with FileNotFoundError for the absent runner.
The new native probe calls remaster_emerald_overworld_step_action on a versioned
explicit synthetic 3x3 setup. The immutable trace has seven ordered commands and
eight observations, including impassable-tile rejection and a return-to-start loop.
It runs twice from fresh process state. Python and native boundaries reject unknown
commands, invalid directions, extra arguments, empty/truncated execution and source
recipe drift. A deliberate observation change reports the first step and field.
The source recipe is bound to its exact LF Git blob SHA.

Local actual native probe compilation passed C99 Wall/Wextra/Wpedantic/Werror
against the verified unchanged portable source cache. Compiled trace: 1 case /
8 observations PASS. Nine orchestration regressions PASS.
CMake adds r19_ordered_production_replay; expected full count is 76 after CI.
Dedicated read-only R19 workflow runs actual targeted/full CTest, targeted/full
Python, required FFmpeg/ffprobe and Unreal source guards. No commit-writing CI.
The I1 trace is a production API probe over synthetic setup, not real-map/story,
state-hash or Unreal/device evidence. Those named checkpoints remain separate.
Next after exact-head CI: R19-I2 — State hash/divergence reporting.

## R19-I2 — Canonical state hash and first divergence

State: VERIFIED_COMPLETE at af9ffdea2c476a1fe3339f85a59f0bdf2db8b460.
Exact-head workflow 37857133241 completed/success. Native/source jobs
113583895591/113583895775 confirm 1 targeted / 76 full CTest, 19 targeted /
347 full Python and Unreal source architecture guard PASS.
RED: test_r19_state_hash.py failed with FileNotFoundError for the absent module.
Native snapshots serialize explicit version/format, full production Vanilla+
SaveBlock2 (0xF44), SaveBlock1 (0x3DC8), storage (0x83D0), all seven domain
identities and movement observations. The sizes are the actual production AGBCC
measurements documented by R17, not stale stock-sized source comments.
C padding/addresses and counter/selected-slot/status/last-sector metadata never
enter the snapshot. Separate process runs and transport metadata perturbation
produce the same snapshots. All attached runtimes are presently absent/null;
unsupported active script/object/encounter/battle state is rejected until its
own named checkpoint adds a complete serializer. Stock canonical gameplay
hashing is not certified here; R17 stock compatibility remains separately tested.

SHA-256 uses canonical UTF-8 JSON, sorted keys, compact separators and trailing
LF, rejecting floats/noncanonical types. Immutable expected domain/state hashes
were created once outside verification after all eight old I1 semantic
observations were checked unchanged. CI has no bless/update path.
Deliberate persistent-byte noise fails at snapshot 0 / save_block1. A mutated
first command fails at snapshot 1. Expected-hash/observation mutation, missing
domain, invalid length/format and schema checks fail; first failure reports the
case/index/field or domain and expected/actual digest. Identical domain data from
a different allocation/key order hashes identically.

Local strict native build and actual compiled eight-snapshot trace/negative
probes PASS. Nineteen targeted Python checks PASS. Expected published full count
was recorded as 346 Python / 76 CTest; the actual full count is 347 Python.
The actual hosted results above close this source checkpoint.
Next after exact-head CI: R19-I3 — Story/progression replay slices.

## R19-I3 — Source-generated build-input readiness

State: RUNNING; story/progression implementation and immutable trace gate remain open.
Live recovery found main unchanged, branch four ahead / zero behind, draft PR #23.
All fifteen phase PR workflows passed on I2; final CodeQL/merge remain separate.
Canonical phase docs and production R2/R4/R10 source surfaces were re-read.
Verified baseline R2 generated-source artifact 11582947405 / R3 provenance artifact
11583196819 bind main workflow 37853379281 and main 7e8d027d.

The connector's UTF-8 binary-file representation changed some map bytes; exact
Git blob validation rejected them. No malformed map data or invented substitute
is used. The native CI job now retains its already-generated R2/R4 fixture sources
and compiled portable library after full CTest PASS, with exact checkout/tree SHA,
source pin, core source blob listing and per-file SHA-256 receipt. This makes the
actual build inputs available for local replay expectation review.
Artifact generation/upload is read-only and writes no implementation commits.
These are test build inputs, not commercial extracted assets, ROM, APK or UE
executables. They are never committed to the public repository.
This preparation checkpoint does not close I3 or advance to I4.

## R19-Closure-1 — Emitted WORLD checkpoint transport

I3 verification -> NEEDS_CLOSURE -> RUNNING_CLOSURE -> VERIFYING.
The source-generated male truck script immediately yields WORLD/LOCK_ALL (type
16 / action 2). The existing version-1 checkpoint writer/read validator capped
pending request types at SPECIAL (15), so production-emitted requests could not
be checkpointed. This is a concrete runtime serialization gap, not a fake map,
host acknowledgement or external-environment blocker.

RED: the new native source-generated request regression exited nonzero:
WORLD checkpoint write failed: gender=0 type=16.
Closure changes only both request-type upper bounds in
core/src/emerald_script_checkpoint.c to the existing WORLD enum member.
Version remains 1 and size remains 298 bytes; no Emerald persistent layout,
script gameplay result or host callback behavior changes.
Native regression covers both male/female emitted requests, write/read/rewrite,
all serialized pending fields/resource identity, stale completion rejection and
unsupported request-type rejection. It does not acknowledge actual WORLD host
execution. Strict local C99 build and both gender cases PASS.
CMake adds r19_source_world_request_checkpoint; published targeted/full CI
must verify the actual expected 2 / 77 CTest and 19 / 347 Python/source gates.
Closure-1 VERIFIED_COMPLETE at c76443b09dedbe4a45410d6e7674c11c04453748.
Actual workflow 37953441780 and jobs 113897841952/113897842288 completed/success:
2 targeted / 77 full CTest and 19 targeted / 347 full Python/source PASS.
All 17 exact-head workflows including CodeQL reached terminal success.
The unversioned I3 probe/matrix work was excluded from the closure commit.


## R19-I3 — Source-generated story/world/progression replay slices

State: VERIFIED_COMPLETE at de446656d9380328f384577700c49a5fe566fd22.
Actual workflow 37955312626 and jobs 113904213881/113904214180 succeeded:
3 targeted / 78 full CTest and 22 targeted / 350 full Python/source PASS.
All nine published Git blobs matched local bytes; all 17 exact-head workflows
including CodeQL reached terminal success before I4 began.
RED: the source-slice test rejected the unsupported active script hash domain and
missing versioned story matrix. Closure-1 above resolved the actual emitted WORLD
checkpoint transport gap before this source checkpoint resumed.

Three immutable cases execute 138 ordered commands / 141 snapshots:
- Both source-generated Brendan/May truck scripts advance source VAR_INTRO_STATE
  (0x4092) from 0 to 3; actual pending WARP type 12 / sequence 29 is pinned.
  Typed presentation/object/movement acknowledgements and an explicit headless
  lock owner are fixture inputs. No attached object runtime/motion, actual host
  playback, follower mutation or script warp application is certified. DOMAIN,
  STARTER_SELECTION, WARP and mutating WORLD requests cannot be acknowledged.
- Real R4 house exit resolves the source warp to Littleroot (5,8); 15 source-map
  moves reach the north edge, then the real connection lands on Route101 (10,19).
  Explicit synthetic traversal setup uses town var 2 (the accepted nonblocking
  R4 setup), lab var 3, RescuedBirch/PokemonGet and valid level-5 Treecko.
  Setup is not a played starter or full-story proof. R10 objective 1 remains
  derived from persistent state through the actual map transition and save.
- Each case performs an R17 in-memory encode/decode and pins unchanged full
  SaveBlock2, SaveBlock1 and storage bytes. No platform I/O claim is made.

Active VM state uses the existing version-1 298-byte checkpoint on a normalized
copy; inactive frames and absent pending storage are zeroed. Header field coverage
guards require serializer/fixture review when the declared VM/runtime changes.
Persistent domains retain actual R17 VP lengths; all inactive domains are null.
Probe recipe is bound to its exact Git blob. Generated R2/R4 source inputs are
bound to SHA-256 from verified build artifact 11626049917 (workflow 37953441780).
Archive SHA-256: 103333ce197b6508d0998874d8c5cf9fe942a527d9bc3e732bdfde19bae67423.
Its checkout/tree, full core blob list and every file digest passed before use.

Reviewed expectations were frozen outside verification after the source semantic
endpoints above were checked. No bless/update execution path exists. Each case
runs twice in separate processes; native malformed commands and missing/wrong
typed completions fail. A PC byte mutation changes the script domain hash.
Strict local C99 source-fixture build, 3/141 actual replay, existing 1/8 movement
replay, both Closure-1 gender cases and 22 targeted Python checks PASS.
CMake adds r19_source_story_world_replay; exact hosted targets/full counts must
verify the expected 3 / 78 CTest and 22 / 350 Python/source gates.
Remaining broader full-game, object host, actual UE/Android and device obligations
are unchanged. Next after terminal-success: R19-I4 — Save fixture orchestration.


## R19-I4 — Complete R17 save fixture orchestration

State: VERIFIED_COMPLETE at 339456009d610260be1357da637769a65303c80a.
Workflow 37956206672 and jobs 113907241038/113907241135 completed/success:
4 targeted / 79 full CTest and 28 targeted / 356 full Python/source PASS.
Verbose native log confirms all eight required names, exact R17 fixture blob,
37 recipes and NOT_RUN_IN_CI private status. All eight published blobs match.
Recovery: main unchanged, branch seven ahead / zero behind, draft PR #23,
I3 exact source/full tests and all 17 workflows completed/success.
RED: the new orchestration test module initially failed importing the absent
save matrix orchestrator (ModuleNotFoundError).

The R19 component binds the unchanged R17 matrix to its exact Git blob and
requires all 37 ordered synthetic recipe IDs, all seven exhaustive owner tests
and the existing r17_save_fixture_matrix entry (eight actual CTest executions).
Coverage includes stock/VP progression/domain import, checksum/sector/counter
validation, byte-exact untouched/special-sector export, VP5/migration, corrupt/
unsupported status, MISSING/ERROR and native write/read failure safeguards.
The original source pin, format geometry, independent oracle and real private
save evidence remain owned by R17; no persistent or platform behavior changes.

The new entry point discovers compiled commands through actual CTest JSON,
executes only the exact required names with no-tests=error and validates complete
JUnit evidence. Missing/duplicate owners, missing executable commands, nonzero
process result, failure/error/skip nodes, truncated evidence, recipe deletion,
original fixture drift and a forged private-run policy cannot report success.
The targeted CI step is verbose so its actual component receipt is visible.
The receipt includes exact executed names, fixture hash, 37 recipe count and
private_real_replay=NOT_RUN_IN_CI. It never embeds private save paths/payloads.
Existing private real replay remains optional local R17 work; its historical
hash/evidence is preserved and no new CI replay is claimed.

Local six negative/coverage regressions and all 28 R19 Python checks PASS.
The unchanged R17 native probe compiled against verified core inputs and ran
the actual 37 recipe independent byte/domain oracle: zero failures.
CMake/CTest are unavailable locally; actual orchestration execution is a required
hosted gate, not inferred from unit mocks or merely finding test registrations.
Expected exact hosted totals: 4 targeted / 79 full CTest and 28 targeted /
356 full Python/source checks. Next after terminal-success: R19-I5 encounter
seed/scenario matrix.


## R19-Closure-2 — Versioned transition command roster

State: VERIFIED_COMPLETE at 4661197566efd8ac9785d99cfa0756c75514c3f8.
Workflow 37956783169 and jobs 113909206167/113909205768 completed/success:
4/79 CTest, 29/357 Python/source gates. All six published blobs match;
all 17 exact-head workflows including CodeQL reached terminal success.
I5 contract recovery found the already tested I3 connection command absent from
the P2 machine-readable command roster. No runtime or fixture result changed.
RED: the new versioned-command guard fails with {'connection'} undeclared.
Closure adds only the existing source-fixture connection owner/declaration and
the roster guard, preserving version 1 and every immutable replay expectation.
Contract prose now explicitly binds warp/connection to the pending source event.
Local 29 R19 Python tests PASS. Expected exact CI: 4/79 CTest, 29/357 Python/
source gates. After terminal-success, resume R19-I5 encounter seed/scenario matrix.


## R19-I5 — Representative encounter seed/scenario matrix

State: VERIFIED_COMPLETE at ff8858d02e4e4d329b5cfbcd3fab2f4a8a5dc6e3.
Workflow 37958197475 and jobs 113913999481/113913999997 completed/success:
5 targeted / 80 full CTest, 34 targeted / 362 full Python/source PASS.
Verbose CI confirms actual 32/256 replay and native negatives; all 13 scoped
published Git blobs match. All 17 workflows including CodeQL reached success.
Recovery: unchanged main, branch nine ahead / zero behind, draft PR #23;
canonical docs, contract, active source/fixtures and exact closure CI re-read.
Primary source re-check: pinned src/wild_encounter.c blob
5e5f3939fea73233b5ccff609eb70c805d7a7a51 and src/phase9_wild_ecosystem.c.
Accepted R12 generator/catalog, core runtime and source tests remain authoritative.
RED: active encounter snapshots were rejected and the versioned matrix was absent.

32 immutable cases / 224 ordered commands / 256 snapshots use eight scenarios
at seeds 0, 1, 0x12345678 and 0xFFFFFFFF:
Route101 land, Repel expiry, disabled land, Route102 water, Old/Good/Super Rod
Route102 generation, Route111 Rock Smash. Setup uses an explicit synthetic valid
level-20 Treecko/zeroed VP save and source-generated R12 map context.
No source gameplay, platform behavior or persistent layout is changed.
These are trigger/generation calls, not an actual R4 walk, fishing minigame,
battle loop or Unreal/device proof. Broader ability/outbreak/roamer/special-map
tests remain in existing R12 coverage; this matrix claims representative scenarios.

The native probe serializes every declared runtime scalar/array, RNG state/calls,
all 64 species bag slots and three-by-two roamer history. Result scalar fields
and complete 100-byte Pokemon packets use the existing R11 party codec.
Hash validation enforces exact field rosters, integer bounds and array lengths.
Header field coverage guards make added runtime/result fields require review.
No sizeof(struct), raw struct memcmp, padding or host pointer enters a hash.

Reviewed source endpoints before freezing immutable expectations:
four initial land/water immunity steps consume zero RNG; seed-zero fifth steps
produce a regular encounter; Repel expiry consumes zero RNG and persists zero;
disabled calls produce no encounter/RNG; fishing retains Repel=100 and produces
rod-specific post-minigame results; all occurring levels are within 5..35.
Observed occurrences across seeds: land 5, Repel 3, disabled 0, water 3,
Old/Good/Super Rod 16 each, Rock Smash 6. These are fixture evidence counts.

Each case runs in fresh processes twice, retains hashes under transport metadata
noise and rejects native runtime bag-slot mutation at snapshot 0 / encounter.
Malformed typed commands/rods, unsigned seed overflow/bool values, missing/extra
runtime fields and truncated bag/history fail. Contract declares the new typed
fishing/Rock Smash owners; the command-roster guard includes encounter fixtures.
Expectations have no verification/CI bless path.

Strict local C99 Werror build against verified full core inputs, actual 32/256
replay/negative probes, earlier 1/8 movement and 3/141 source story matrices,
and all 34 targeted Python checks PASS.
Expected exact hosted totals: 5 targeted / 80 full CTest and 34 targeted /
362 full Python/source gates. Next after terminal-success: R19-I6 battle
seed/scenario matrix including accepted R16 deltas.


## R19-I6 — Complete battle field snapshots and representative seed matrix

State: VERIFIED_COMPLETE at 7c86759cf1e85a4971f2ef6a2c276c3f663caca8.
Workflow 37960402440 and jobs 113921490032/113921490363 completed/success:
6 targeted / 81 full CTest, 38 targeted / 366 full Python/source PASS.
All 14 published blobs match; all 17 exact-head workflows succeeded.
Recovery re-read live refs/compare/history/draft PR #23/CI, canonical and owning
R13/R16/contract source files. Main unchanged; ten ahead / zero behind.
RED: the new battle test could not import the absent battle domain validator.

Primary source re-check at the fixed VP pin:
src/battle_util.c blob 68a5b6adaae9c5941db66c6a1856425e7727d7fb,
src/battle_script_commands.c blob 921e1ccae078d6aab633c36a9d86539052df9e3b,
include/constants/items.h blob daf129c76f4d5a3775489603b61955fa865531e6.
Source player-side DOUBLE_PRIZE entry sets money multiplier 2; trainer reward
uses it. Amulet Coin is item 189. Existing R13 and accepted R16 owners remain
unchanged; no gameplay, persistent layout or platform changes are made.

16 immutable cases / 68 ordered commands / 84 snapshots use four scenarios
at seeds 0, 1, 0x12345678 and 0xFFFFFFFF:
wild win; trainer faint/replacement/win; trainer loss/whiteout; R16 held swap
before trainer battle and doubled reward. Synthetic Pokemon have explicit
stats/moves/OT/IVs and one-HP edges. Trainer 1 names the source reward/finalizer
metadata; the synthetic opponent party is not a played source trainer encounter.
Broader status/ability/capture/double/R16 safety mechanics stay in existing tests.

The frozen version-1 schema binds both battle and codec headers. Serializer
covers every declared member: State 38, Mon 57, RNG 2, Event 6, Action 5;
all four battlers, complete 2x6 party/caught packets, all future/side/status arrays,
RNG state/calls and the active ordered event vector. R11 100-byte codec is reused.
Explicit JSON/integers/arrays replace raw struct memory. C padding/pointers and
inactive event capacity are excluded; null is allowed only before attachment.
Missing/extra members, malformed arrays/packets, event-count mismatch/overflow,
float RNG and header field changes cannot be certified.

Source endpoints were reviewed before freezing:
normal replacement leaves the turn counter unchanged; faint precedes EXP;
loss commits zero HP and requests whiteout; wins commit surviving party and
trainer flag; R16 swap moves Amulet Coin 189 from reserve to lead before start,
preserves party packets and source reward increases 840 -> 1680 (money
1000 -> 1840 vs 2680). Every case reaches its explicitly expected outcome.
No actual Unreal/battle presentation or full-game device execution is claimed.

Each case repeats in separate processes, ignores save transport metadata noise,
and rejects a native inactive-slot future_damage mutation at its first attached
snapshot / battle domain. Native malformed actions/lifecycle and late QoL input
fail. Typed start/turn/commit/QoL command owners are declared by the contract and
included in the roster guard; serializer schema and probe source blobs are pinned.
No verification/CI bless/update path exists.

Strict C99 Werror native build, actual 16/84 battle/negative replay, prior 32/256
encounter, 3/141 story and 1/8 movement matrices and 38 targeted Python checks PASS.
Expected exact CI: 6 targeted / 81 full CTest, 38 targeted / 366 full Python/
source gates. Next after terminal-success: R19-I7 manifest audit orchestration.


## R19-I7 — Asset manifest/source audit orchestration

State: VERIFIED_COMPLETE at e31d5666da93e310c10982b58d5ba59ac6af8e5f.
Workflow 37961436672 jobs 113924967799/113924968452 completed/success:
6/81 CTest, 44/372 Python/source; explicit 52/21/24/74 asset audits, zero skips.
All seven published blobs match. PR CodeQL was still running at I8 recovery;
its terminal success remains mandatory before the phase final merge gate.
Subsequent pre-publish check: all 17 I7 exact-head workflows completed/success.
Recovery: main unchanged, eleven ahead / zero behind, draft PR #23; I6 all
17 workflows terminal-success. Canonical and all 15 owner test modules re-read.
RED: the new six orchestration regressions failed importing the absent module.

Versioned data/r19/asset_audits.json binds every existing R6/R7/R14/R15 test
module to its exact Git blob and requires actual minimum counts 52/21/24/74.
The new single entry point discovers and executes the 15 unchanged owner modules,
then records actual per-phase test counts. Empty/missing phase/module coverage,
source drift, zero/truncated execution, skips and first actual failures cannot
report PASS. It stops with the component and failing case; no result is inferred
from merely importing, listing tests or existing historical completion docs.

Existing audits use source inputs and synthetic private package stand-ins.
No commercial/normalized payload is required or committed. The receipt explicitly
excludes actual Unreal runtime and device execution. Existing private asset/audio
completion obligations are unchanged. No gameplay/asset owner implementation is
modified. FFmpeg/ffprobe remain required by the actual hosted source job.

Local six new negative/coverage checks and all 44 R19 Python regressions PASS.
The partial local cache cannot execute all owner source audits; actual hosted
171-test component execution is mandatory. Expected exact CI: 6/81 CTest,
44/372 Python/source plus explicit asset components 52+21+24+74=171, zero skips.
Next after terminal-success: R19-I8 generated package/manifest/hash integrity.


## R19-I8 — Repeated clean metadata generation and corruption integrity

State: VERIFYING; I9 NOT_STARTED until actual component CI succeeds.
Recovery: main unchanged, twelve ahead / zero behind, draft PR #23; I7 native,
source and actual 171 asset components passed. Canonical docs and each owner
package generator re-read. RED: missing package integrity module import failed.

Versioned package_integrity.json pins the four existing owner generator blobs
and all 12 staged output names. Entry point creates two fresh temporary metadata
packages per owner (R6 3, R7 5, R14 2, R15 2 files), compares every byte SHA-256,
checks exact complete file coverage and retains digests in an actual receipt.
R14/R15 regenerated catalog/header bytes must match committed owner outputs.
Each of the twelve files is deliberately corrupted, must fail its own checksum,
is restored and must verify again. Empty/missing/extra files, unsafe paths,
symlinks and clean-generation drift cannot report integrity success.

All staging/negative mutations are temporary local output; no repository source,
vendor or committed expectations are rewritten. Receipt keeps no temp paths or
timestamps and does not certify private meshes/textures/audio, cook, UE or device.
R15 package here is the source catalog/header, not a private PCM package. Existing
R15 synthetic PCM corruption/loop/provenance owner tests remain in I7 coverage.

Six local integrity regressions and all 50 targeted R19 Python checks PASS.
Local partial source cache cannot regenerate the complete owners; actual hosted
four-owner, eight-clean-generation, twelve-corruption execution is mandatory.
Expected exact CI: 6/81 CTest, 50/378 Python/source, 171 asset audits and all
four package receipts. Next after terminal-success: R19-I9 Unreal smoke contract.
