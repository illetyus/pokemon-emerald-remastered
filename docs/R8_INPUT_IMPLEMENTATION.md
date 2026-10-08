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

State: **VERIFYING**; exact published-head hosted CI is required.
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
