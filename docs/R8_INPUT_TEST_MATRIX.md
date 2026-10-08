# R8 input source regression matrix

State: T1/V1 and Closure-1/G1 VERIFIED_COMPLETE; workflow 37851201049 passed
8/75/328. [Completion/final gates](R8_INPUT_COMPLETION.md). Contract: [R8_INPUT_CONTRACT.md](R8_INPUT_CONTRACT.md).
Source pin: `70db90c9077aed1272e746fc2537d9f12b95a91c`.
Branch: `r8-input-infrastructure`. No real engine/device certification.

## Compiled evidence

| Surface | Production-used implementation | Actual regression |
| --- | --- | --- |
| Event/focus/owner classification | RemasterInputRouter.h | 191 native checks |
| Ten touch regions/capture/normalization | RemasterTouchInput.h | 88 native checks |
| Keyboard/gamepad/Enhanced normalization | RemasterPhysicalInput.h | 218 native checks + compiled mapping JSON oracle |
| One-owner delivery and R9 consumption | RemasterInputDispatch.h + accepted R9 model/core | 18 native integration checks |
| Independent suspension/source fence | RemasterInputLifecycle.h | 20 native lifecycle checks |
| Shared held-state 24/3 presentation repeat | RemasterInputRepeat.h + existing R9 Repeat | 11,104 native checks |
| Partial Enhanced mapping/fallback coverage | RemasterEnhancedCoverage.h | 44 native checks |
| Versioned ordered traces | r8_input_fixture_probe.cpp, runner, input_matrix.json | 47 cases / 221 expected observations |

The fixture probe compiles actual portable implementations. Its command parser is
only test transport; it does not implement gameplay or certify UE owner attachment.
The versioned matrix covers four sources × ten actions; duplicate/release semantics;
queued focus handoff; touch/physical shared holds and slide cancellation; partial
suspension resumes/analog neutral/reconnect; actual touch-held repeat/settings;
unsupported field/battle, script/dialogue/battle priority; recovery HUD/menu scope;
changed/invalid/neutral analog sequences. Independent native probes add capacities,
malformed enums, safe areas, owner rejection without fallback, core fingerprint
preservation, epoch exhaustion and long repeat holds.

## Source and hosted gates

Eighteen Python tests currently validate compiled binding/fixture oracles and actual
production source boundaries, including context-before-capture, native owner
leasing, no Blueprint gameplay fallthrough, real world API identities, paired
lifecycle delegate teardown, platform flush, owned mapping removal, common-held
R9 repeat and settings isolation. The deliberately wrong fixture fails at its
first case/step/field; malformed fixture actions and duplicate identities fail.

The R8 workflow builds the native core and all R8 probes, runs targeted R8 CTest,
then full portable CTest (including R4/R9), all Python tests with mandatory FFmpeg,
and Unreal authority source guards. Exact commit/job logs, not local partial-tree
assumptions, supply the full hosted gate. CodeQL and all PR checks are required
before merge; main terminal-success is required before R19-P1.

## Explicit actual-runtime obligations

R18 must establish real UE 5.8.3/UHT compile/cook/package and cooked mapping/layout
readiness. Real runtime validation must establish visible/usable touch controls,
automatic device safe area/DPI, UMG pointer consumption without duplicate controller
delivery, keyboard/controller mappings, physical fresh-press/Enhanced flush semantics,
focus/background/pause/device callback order, timer/latency and actual field/battle
request-owner/resource attachment. None is certified by this source matrix.

Actual host endpoints must validate core pending request/selection revisions and
reset/replace their input owner on request changes. Unattached field/battle inputs
remain explicitly unsupported. These fixtures create no VM or playable battle host.
Stop at EXTERNAL_ENV_REQUIRED when the actual R18 environment is absent, or after
R18 main CI at REAL UNREAL RUNTIME VALIDATION. Do not automatically execute Android
smoke, BrowserStack, final device matrix or R21/R22.
