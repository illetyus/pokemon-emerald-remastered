# R8-I1 — Common input router evidence

I1 state: **VERIFIED_COMPLETE** at `1d45a91d64adc60ff250da1f2e122ec385d15ae4`.
Workflow 37817943261: targeted 1/1 R8 CTest, full 68/68 CTest, 310/310 Python
and Unreal source guard PASS. All 16 exact-head push/PR workflows reached success,
including CodeQL. I2 below is VERIFYING its new exact-head CI.
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

State: **VERIFYING**; exact published-head CI remains required.
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
