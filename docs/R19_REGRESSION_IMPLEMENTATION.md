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

State: VERIFYING; R19-I4 NOT_STARTED until exact-head CI succeeds.
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
