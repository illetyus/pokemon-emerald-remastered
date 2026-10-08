# R8-I1 — Common input router evidence

State: **VERIFYING**; exact published-head CI is required before VERIFIED_COMPLETE.
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

