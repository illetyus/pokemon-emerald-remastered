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

State: VERIFYING; implementation/local checks passed, published-head CI required.
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
