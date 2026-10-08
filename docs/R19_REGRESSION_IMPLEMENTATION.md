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

State: VERIFYING; local implementation/negative checks passed, exact-head CI required.
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
is 346 Python / 76 CTest; only actual CI logs can close this checkpoint.
Next after exact-head CI: R19-I3 — Story/progression replay slices.
