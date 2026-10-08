# R8-I1 — Common input router evidence

I1 state: **VERIFIED_COMPLETE** at `1d45a91d64adc60ff250da1f2e122ec385d15ae4`.
Workflow 37817943261: targeted 1/1 R8 CTest, full 68/68 CTest, 310/310 Python
and Unreal source guard PASS. All 16 exact-head push/PR workflows reached success,
including CodeQL. I2 below is also VERIFIED_COMPLETE.
Base: `d9771a8da6611c9078dffb4aefeb4b62b1bac68c`.
Contract: [R8_INPUT_CONTRACT.md](R8_INPUT_CONTRACT.md).
Branch: `r8-input-infrastructure`.

The portable C++17 `RemasterInputRouter.h` owns only input state and dispatch
classification. It has no gameplay/save pointers, UE/Android dependency,
RNG or gameplay mutator. The production controller has not yet been rewired;
touch, physical adapters, actual owner integration, lifecycle callbacks and
R9 common-held repeat remain I2–I6.

## Actual regression-first verification

`tests/r8_input_router_test.cpp` was created first. Strict GNU C++17 compilation
failed (RED) because `RemasterInputRouter.h` did not exist. After the router
implementation, strict compilation and execution passed **191/191 native checks**.

The same scenario runner also passed AddressSanitizer and UndefinedBehaviorSanitizer
with leak detection disabled. LeakSanitizer itself cannot run under this
environment's ptrace instrumentation; no leak-sanitizer success is claimed.

Meaningful cases cover independent ten-action owner expectations and equivalent
keyboard/gamepad/touch/Enhanced traces; duplicate/multi-control hold suppression;
release ownership/mismatch; pinned digital priority without synthetic release
movement; 64-record capacity and atomic rejection; malformed enums; stale release
against a fresh same-control press; owner/focus epoch reset; independent suspension
reasons; Menu/load recovery with unusable world; script/dialogue/I/O precedence;
explicit unattached field/battle failure; axis dead zone/ties/nonfinite/out-of-range
values; epoch exhaustion without wrap.

The CMake target `r8_input_router_test` and CTest `r8_common_input_router`
compile this actual header. The new R8 workflow runs targeted native R8,
full portable tests including R4/R9, all Python regressions with mandatory
FFmpeg, and existing Unreal authority source guards. It never commits source.

## Publication gate and remaining scope

Local schema/action/limit, authority and workflow boundaries passed.
The local snapshot does not contain a complete configured CMake build; hosted
targeted/full results must be read from the exact published commit, not inferred
from local native checks. No real UE/UHT or Android success is implied.
After exact-head CI terminal-success, next is **R8-I2 — Touch source wiring**.



## R8-I2 — Touch layout/source wiring

State: **VERIFIED_COMPLETE** at `c18d820c4e34caa3b35a94c82595e1cc161f10f1`.
Workflow 37846220404 passed targeted 2/2 CTest, full 69/69 CTest, 310/310 Python
and Unreal source guards. All 16 exact-head workflows, including CodeQL, succeeded.
Base: `1d45a91d64adc60ff250da1f2e122ec385d15ae4`.

The shared portable `RemasterTouchInput.h` implements all ten schema rectangles,
bounded finger captures, half-open hit regions, pixel/safe-inset normalization,
press/release/cancel, slide-out cancellation without retargeting, and monotonic
epoch synchronization. Invalid/overlapping authoring overrides retain the prior
validated layout. Touch configuration lives in a presentation InputConfig asset;
it creates no Emerald save fields.

The new UE InputSubsystem owns the same portable Router and Touch adapters.
The production controller binds native touch press/move/release and forwards
normalized events only through that endpoint. Existing keyboard/world handlers
remain unchanged. No direct touch-to-core mutation or Blueprint gameplay shortcut
was added. The endpoint is blocked until a native dispatch owner is attached;
production UI/world/battle focus/owner attachment remains I4, physical mappings
I3, lifecycle delegates I5 and common-held R9 repeat I6.

`tests/r8_touch_input_test.cpp` was written first and strict compilation was RED
because the touch header did not exist. A second targeted RED caught stale source
epoch rollback destroying a current capture; Sync now rejects an older epoch.
Strict compilation and **88/88 native checks** then passed, including all ten
independent control centers, shared-router dispatch/release, duplicates, slide,
finger bounds, malformed coordinates, safe-area extents, half-open edges, stale
queued presses and atomic layout rejection. ASan/UBSan passed with leak detection
disabled for the previously documented ptrace environment limitation.

Local source checks verified native bindings, one shared router, explicit blocked/
unsupported unattached endpoints and no touch-core mutation. CMake adds
`r8_touch_source_capture`; hosted targeted/full CTest and full Python/source
results must be read from the exact commit. No UHT/Unreal compile, rendered touch
controls, cooked layout, automatic device safe-area query, UMG pointer propagation
or Android runtime behavior is certified. These actual runtime obligations remain
visible at the R18/real-runtime boundary.

After I2 exact-head CI terminal-success, next: **R8-I3 — Gamepad/keyboard mappings**.


## R8-I3 — Gamepad/keyboard mappings

State: **VERIFIED_COMPLETE** at `a39c97a3387e079d04543aaae175c4dad2996ed3`.
Workflow 37847802114 passed targeted 3/3 CTest, full 70/70 CTest, 313/313 Python
and Unreal source guards. Draft PR CodeQL is tracked separately from the subphase gate.
Base: `c18d820c4e34caa3b35a94c82595e1cc161f10f1`.

Regression files were created first; strict compilation was RED because the
physical adapter header did not exist. `RemasterPhysicalInput.h` now provides
25 schema-pinned native keyboard/gamepad bindings, captured digital press epochs
and independent Enhanced/gamepad analog adapters. Focus resets retain physical
press tombstones until release or neutral, preventing held sources from issuing
a fresh command after an owner switch. Axis changes release before pressing;
Triggered callbacks in the same direction do not generate world repetition.

Strict C++17 compilation and **218/218 native checks** passed. ASan/UBSan passed
with leak detection disabled for the documented instrumentation limitation.
**3/3 Python tests** compare compiled bindings with the frozen JSON contract,
check actual gamepad engine-key identities and execute edge/epoch/axis regressions.
The new CMake target is `r8_physical_input_test`, CTest `r8_physical_sources`.

Production native bindings now deliver both press and release through the shared
subsystem, including Map/Quest shoulder buttons. Enhanced actions require the
correct value type, an actual mapping-context entry and a unique action asset.
Invalid or absent actions retain their semantic native fallback. Enhanced buttons
use Started/Completed/Canceled; the 2D axis also accepts Triggered but deduplicates
held direction. The fallback gamepad stick samples its paired axes once per tick.
A stray standalone `+` in the previous controller source was removed; existing
source validators had not established real C++/Unreal compilation for that file.

No input path is attached to gameplay yet: I4 must install the native owner and
refresh UI/world/script/battle context before dispatch. I5 lifecycle reset and I6
R9 repeat remain open. Legacy unbound controller handlers will be reconciled with
owner routing in I4. The local partial snapshot cannot run the full R9 suite or
Unreal source guard (missing dependent repository files); hosted exact-head checks
remain required. No UE/UHT build, device bindings, cooked Enhanced assets or
Android execution is certified.

After I3 targeted/full hosted CI passes, next: **R8-I4 — UI/gameplay focus routing**.


## R8-I4 — UI/gameplay focus routing

State: **VERIFIED_COMPLETE** at `3f04104edb78ec8e6e92cf6e152aaf85b48fbdd9`.
Workflow 37848753892 passed targeted 4/4 CTest, full 71/71 CTest, 317/317 Python
and Unreal source guards. Draft PR aggregate/CodeQL remains a separate final gate.
Base: `a39c97a3387e079d04543aaae175c4dad2996ed3`.

The production controller now leases a single native context/dispatch owner,
without replacing another controller's owner. Each physical/touch/axis capture
refreshes context before capturing its epoch; delivery refreshes again and then
uses the shared compiled `RemasterInputDispatch.h` selector. A rejected owner
returns unsupported without another owner or Blueprint gameplay fallback.
Unbound old physical/Blueprint handlers were removed. World steps keep R9-first
consumption and the existing R4 authoritative StepPlayer/result/pawn bridge.

R9 exposes existing save/map/modal/I/O/script/battle and pending core-dialogue
facts. Screen, map identity and request ownership changes fence holds; I/O/load,
VM attachment, busy changes and view teardown explicitly reset the input boundary.
The UI boundary counter fails closed at uint64 exhaustion. Cursor revisions do
not reset the hold/repeat clock. The existing nonexistent GetCurrentMapNum call
was corrected to GetCurrentMapNumber, verified against the actual world header.

Field/battle endpoints are native single-cast boolean owners installed via
SetFieldOwner/SetBattleOwner, with reset on replacement or detachment (empty
delegate). Default endpoints remain unattached/unsupported. Actual core hosts
must validate their pending request/selection revision and call ResetInputs or
replace their owner when that request changes. These interfaces do not create a
script VM, playable battle host, resources or a new authoritative gameplay state.
R9 retains its existing core request/row/fingerprint checks and busy lifetimes;
visual completion cannot authorize battle or script progression.

The native R9 integration regression was written first and strict compilation was
RED because the dispatch header did not exist. After implementation, **18/18
native focus/real-R9 checks** passed, including modal routing, cancellation,
stale queued delivery, unattached/rejecting owners, script/battle blockers,
corrupt-save recovery, malformed targets, save-state fingerprint preservation
and same-focus owner fences. The production dispatcher itself is compiled with
the accepted R9 model and actual core code; local core files were verified against
live Git blob hashes before reuse. The native integration and model passed
ASan/UBSan (leak detection excluded as documented). **7/7 local R8 Python tests**
passed; four guard actual source call/owner/context and world API boundaries.
The R9 source test now recognizes StepDirection's boolean response signature;
its UI-before-world assertions are unchanged. Full R4/R9, all Python and Unreal
source guard evidence remains the exact hosted CI gate.

Lifecycle callback/disconnect handling is I5; common-held R9 repeat is I6.
No actual UE/UHT compilation or working host/device execution is certified.
Next after targeted/full CI: **R8-I5 — Lifecycle reset/reconnect**.


## R8-I5 — Lifecycle reset/reconnect

State: **VERIFIED_COMPLETE** at `6046cba6508a3624a80dcf99f6def2563fb49802`.
Workflow 37849310238 passed targeted 5/5 CTest, full 72/72 CTest, 320/320 Python
and Unreal source guards. No real engine/device lifecycle result is implied.
Base: `3f04104edb78ec8e6e92cf6e152aaf85b48fbdd9`.

The shared `RemasterInputLifecycle.h` treats Background, Inactive and Paused as
independent reason bits. Only clearing all active reasons resumes input. Duplicate
or malformed reason changes do not mutate state. Its production-used source fence
clears common/touch/digital state, advances the router epoch, and requires neutral
before either analog source can rearm. No elapsed time or old holds are restored.

InputSubsystem registers/removes owned application background/foreground and
inactive/reactivated delegates, and the platform input-device connection delegate.
Every connect/disconnect conservatively fences all sources. The core ticker only
observes world pause/context; it never submits gameplay commands or completions.
Pause is also checked before every delivered source event, avoiding a world timer
that would stop observing while paused. Lifecycle transitions flush platform pressed
keys and request Enhanced mapping rebuild with held keys ignored until release.
Controller teardown removes only a mapping context that this controller installed;
previously installed contexts are retained. All delegate handles are removed.

The native lifecycle regression was written first; strict compilation was RED
because the lifecycle header did not exist. **20/20 native checks** then passed:
mixed-source suspension, independent reasons/partial resumes, duplicate transition,
stale queued delivery, cleared captures, fresh digital edge, neutral-only analog
resume, disconnect/reconnect and malformed reason rejection. ASan/UBSan passed
with the previously documented leak exclusion. **10/10 local R8 Python tests**
passed; three additional source guards cover delegate teardown, non-gameplay pause
observation/platform flush and owned mapping removal. CMake registers
`r8_lifecycle_source_fence` in targeted/full hosted runs.

The actual UE 5.8.3 compile, platform event order, Enhanced rebuild/pressed-key
flush behavior and Android/controller callbacks still require R18/runtime evidence.
This source checkpoint does not certify them. Common-held R9 repeat is still I6.
Next after targeted/full CI: **R8-I6 — Presentation timing QoL wiring**.


## R8-I6 — Presentation timing QoL wiring

State: **VERIFIED_COMPLETE** at `7194102a6dc6e991f8ef4ed2821b322acab580b2`.
Workflow 37849841175 passed targeted 6/6 CTest, full 73/73 CTest, 322/322 Python
and Unreal source guards. No actual engine/platform timing is certified.
Base: `6046cba6508a3624a80dcf99f6def2563fb49802`.

R9 PresentationTick now reads the shared router's semantic holds rather than
polling physical keyboard/D-pad keys. The compiled `RemasterInputRepeat.h` bridge
uses the existing R9 Repeat, retaining 60 Hz, first tick 24 and every 3 thereafter.
Only ModalUI/Dialogue Up/Down may repeat. Neither world/battle/recovery HUD nor
Confirm produces repeated commands. Multiple sources holding the same direction
retain a single countdown, including release of just one source.

Epoch/focus/settings changes reset the repeat countdown and drop old presentation
catch-up time. R9 MenuRepeat/FastText remain the existing GameUserSettings settings;
no Emerald metadata or gameplay timing was introduced. Counter values are bounded
by recycling the identical post-delay 3-tick interval, preserving long holds
without a multi-year unsigned overflow. Text ticking stops while input is blocked.

The regression was written first and strict compilation was RED because the
bridge header did not exist. **11,104/11,104 native checks** passed, including
four-source 24/3 traces, settings disable/reenable, epoch fences, semantic source
handover, world/battle/Confirm non-repetition, dialogue scope and 10,000-tick
cadence. ASan/UBSan passed with documented leak exclusion. **12/12 local R8 Python
tests** passed; two new guards verify actual R9 timer integration, old-time discard,
absence of physical polling and existing settings ownership. CMake registers
`r8_common_held_ui_repeat` in targeted/full hosted runs.

T1/V1/G1 still must reconcile the complete matrix and all acceptance requirements.
Actual UE/UHT, platform timer/callback order, touch consumption, controller mapping
and Android latency/device behavior remain the R18/runtime obligations.
Next after targeted/full CI: **R8-T1 — Input contract regressions**.


## R8-T1 — Versioned input fixture matrix

State: **VERIFIED_COMPLETE** at `d1730592c20decbf872d8d8a3dda72eeec6ac8c5`.
Workflow 37850579912 passed targeted 7/7 CTest, full 74/74 CTest, 326/326 Python
and Unreal source guards. V1 passed; G1 found the Closure-1 mapping gap below.
Base: `7194102a6dc6e991f8ef4ed2821b322acab580b2`.

The versioned [input_matrix.json](../tests/fixtures/r8/input_matrix.json) contains
47 cases and 221 expected observations. The actual compiled fixture probe uses
the production portable Router, Digital/Touch/Analog adapters, Lifecycle fence
and R9 repeat bridge. The runner reports the first divergent case/step/field;
fixtures pin input provenance and expected behavior, not a second gameplay model.
[Coverage and limitations](R8_INPUT_TEST_MATRIX.md) map the complete native/source
matrix to the canonical acceptance requirements.

The unittest was written first and import was RED because the replay runner did
not exist. Strict native compilation, all **47/47 cases / 221/221 observations**,
and **16/16 local R8 Python tests** passed. A deliberately altered expectation
proves first-divergence reporting; malformed action and duplicate case identity
are rejected. ASan/UBSan replay passed with the documented leak exclusion.
CMake adds `r8_versioned_input_fixture_matrix`; hosted targeted/full checks remain
required. There are no extracted/private save, ROM, asset or device payloads.

After targeted/full exact-head CI, next: **R8-V1 — Targeted CI reconciliation**.


## R8-G1 → R8-Closure-1 — Partial Enhanced fallback coverage

Initial G1: **NEEDS_CLOSURE** at `d1730592`; Closure-1 is **VERIFIED_COMPLETE**
at `19f8716d9db199f7c7ae42f0ed214a6224503f74`. Workflow 37851201049 passed
8/8 targeted CTest, 75/75 full CTest, 328/328 Python and Unreal source guards.
V1/G1 re-verification passed. D1 publication is VERIFYING; F1 remains required.
Acceptance review found that a valid keyboard-only Enhanced action suppressed
all native equivalents, including unmapped gamepad controls. The existing action-
level tests had not covered partial mapping assets. This violated I3 equivalence.

The new production-used `RemasterEnhancedCoverage.h` validates each mapped key
atomically and tracks coverage per actual default physical key. All ten native
action groups are visited; only covered native keys are omitted. The native paired
stick is disabled only with complete Enhanced stick coverage. Partial single-axis
stick assets, reserved other-action keys, positional Touch keys, empty/invalid
mapping assets and duplicate action assets retain safe native fallbacks.

The regression was written first and strict compilation was RED because the
coverage planner header did not exist. **44/44 native coverage checks** passed,
including keyboard-only Move retaining gamepad, mapped Enter retaining Space and
FaceBottom, atomic wrong-action rejection, complete vs partial stick coverage and
mapped D-pad retaining keyboard without duplicate native keys. ASan/UBSan passed
with documented leak exclusion. **18/18 local R8 Python tests** passed, including
two actual controller coverage guards. The R9 fallback source assertion now checks
all-action/per-key coverage rather than the obsolete all-or-nothing boolean flags;
its UI-first world routing guard is unchanged.

No gameplay/persistence/timing or lifecycle rule changed. Exact hosted R4/R9/full
regressions remain required before re-running V1/G1. Actual Enhanced modifiers,
cooked assets and UE/device callback behavior remain explicit runtime obligations.
Next after closure CI: **R8-V1 → R8-G1 re-verification**.


G1 acceptance and completion ledger: [R8_INPUT_COMPLETION.md](R8_INPUT_COMPLETION.md).
Next: D1 exact-head CI, then independent F1 reconciliation before PR/main gates.
