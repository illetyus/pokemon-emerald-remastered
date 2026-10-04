# Phase 8 — RTC Settings and Safe Time Realignment Design

Date: 2026-10-01
Repository: `illetyus/pokezumrut-vanillaplus`
Status: Approved design

## 1. Goal

Phase 8 makes Emerald's real-time clock behavior recoverable and emulator-friendly without changing the save layout or turning clock correction into a time-skip mechanic.

The player will be able to open `RTC AYARLARI` from the in-game Options menu, inspect the current local game time, correct hour/minute, and optionally correct the internal game-day counter through an advanced screen.

The correction path must preserve normal berry growth, daily events, Shoal Cave tides, Mirage/lottery/TV timing, and all other vanilla time-based systems after the new reference time is established.

## 2. Non-negotiable compatibility contracts

Phase 8 must preserve all existing Vanilla+ compatibility guarantees:

- `GAME_CODE` remains `BPEE`.
- Existing 128 KiB save files continue to load without conversion.
- `SaveBlock1`, `SaveBlock2`, `PokemonStorage`, party Pokémon, item IDs, move IDs, species IDs, flags, and vars do not shift.
- No new persistent save field is added for RTC settings.
- Event tickets, Mystery Gift, legendary areas, story state, and existing daily-event state remain intact.
- Existing learned moves and all Phase 0–4 behavior remain unchanged.
- The hidden vanilla RTC recovery path remains present and functional.

Phase 8 reuses the existing persisted time state already used by Emerald:

- `gSaveBlock2Ptr->localTimeOffset`
- `gSaveBlock2Ptr->lastBerryTreeUpdate`
- `VAR_DAYS`
- existing RTC hardware/emulator source read through `rtc.c`

## 3. Existing architecture being reused

Emerald already contains the required building blocks:

- `src/rtc.c` reads RTC state and calculates local time as RTC time minus `localTimeOffset`.
- `src/clock.c` runs per-day and per-minute time-based events.
- `src/reset_rtc_screen.c` already contains a full RTC correction UI and input framework.
- `src/event_data.c` contains the vanilla one-shot hidden RTC-reset enable/disable mechanism.
- `src/title_screen.c` contains the hidden title-screen RTC recovery entry.
- `src/option_menu.c` contains the normal Options UI.
- `src/start_menu.c` provides a distinct in-game path into Options.

The implementation should adapt these systems instead of introducing a second clock implementation.

## 4. User-facing entry point

### 4.1 In-game only

`RTC AYARLARI` is shown only when Options is opened from the in-game Start Menu.

The title-screen Options menu remains unchanged and must not expose RTC controls before a save has been loaded.

Do not infer this state from fragile callback comparisons. Add an explicit non-persistent Options entry mode, for example:

- normal/title Options entry
- in-game Options entry

`StartMenuOptionCallback` should use the in-game entry function. The existing title/main-menu path should use the normal entry function.

No state for this distinction is written to the save file.

### 4.2 Options layout

Normal/title Options remains:

1. Text Speed
2. Battle Scene
3. Battle Style
4. Sound
5. Button Mode
6. Frame
7. Cancel

In-game Options becomes:

1. Text Speed
2. Battle Scene
3. Battle Style
4. Sound
5. Button Mode
6. Frame
7. `RTC AYARLARI  >`
8. Cancel

The Options window, Y positions, cursor/highlight range, and navigation bounds must be adjusted for the eighth row only in in-game mode.

Selecting `RTC AYARLARI` must save/apply any pending normal Option values exactly as if the player were leaving Options normally, then transition to the RTC screen.

Returning from RTC settings must return to the field safely through the existing in-game return path. It does not need to reopen Options automatically.

## 5. RTC Settings UX

The RTC screen reuses the existing `reset_rtc_screen.c` visual/input infrastructure but exposes a new player-facing mode separate from the vanilla hidden recovery mode.

### 5.1 RTC settings home

Display:

- current local game time in 24-hour format
- RTC availability/status

Menu:

- `SAATİ AYARLA`
- `GELİŞMİŞ`
- `GERİ`

### 5.2 Standard correction

`SAATİ AYARLA` edits only:

- hour: 00–23
- minute: 00–59

Seconds are normalized to `00` when the correction is applied so the resulting reference is deterministic.

The game-day counter is unchanged.

### 5.3 Advanced correction

`GELİŞMİŞ` edits:

- game-day counter
- hour
- minute

The day range should remain compatible with the existing `struct Time`/RTC-reset UI range and existing save representation. Do not introduce calendar date/month/year storage.

Seconds are normalized to `00` on apply.

This is explicitly an advanced repair tool, not a normal gameplay progression control.

### 5.4 Confirmation

Before applying any correction, show a confirmation screen communicating both points:

- the displayed game time will change
- daily/time-based events will not be advanced by the correction itself

The correction is applied only after explicit confirmation.

Canceling at any point returns without modifying RTC/time reference state.

## 6. RTC source policy

The physical/emulated RTC remains the time source.

The normal Phase 8 correction path must NOT:

- call `RtcReset()`
- write a new hardware RTC date/time
- emulate time using play time or frame counters
- create a software RTC fallback in save data

Instead, correction recalculates `localTimeOffset` so that the current RTC source maps to the player-selected local game time.

Conceptually:

`new localTimeOffset = current raw RTC time - selected local game time`

The existing `RtcCalcLocalTimeOffset(...)` path should be reused or wrapped rather than duplicating RTC arithmetic.

This keeps time progressing while the game is closed whenever the emulator/device provides RTC support.

## 7. Safe time-event realignment

Clock correction is a reference-time repair operation, not elapsed-time simulation.

After a correction is confirmed:

1. Recalculate `localTimeOffset` for the selected local time.
2. Recalculate/read `gLocalTime` from the new offset.
3. Set `gSaveBlock2Ptr->lastBerryTreeUpdate = gLocalTime`.
4. Set `VAR_DAYS = gLocalTime.days`.
5. Do not call per-day update handlers as part of the correction.
6. Do not call `BerryTreeTimeUpdate` for the skipped/rewound interval.
7. Do not clear daily flags.
8. Save normally.

This rule applies whether the corrected clock moves forward or backward.

### 7.1 Consequences by design

A correction must NOT immediately:

- grow berries
- reset daily gifts
- reroll Mirage Island state
- reroll lottery
- advance TV shows
- advance Pokerus timers
- run missed-day weather updates
- create multiple days of accumulated daily processing

Existing daily flags remain exactly as they were before correction.

Once realigned, normal future RTC progression resumes through the existing `DoTimeBasedEvents()` path.

### 7.2 Backward corrections

Backward correction must not strand `lastBerryTreeUpdate` or `VAR_DAYS` in the future relative to local time.

The realignment in section 7 is mandatory for backward corrections specifically to avoid vanilla guards suppressing updates until the old future timestamp is reached again.

## 8. Shoal Cave and time-of-day consumers

Shoal Cave and other consumers that depend directly on current local hour should immediately observe the newly selected local time after correction.

No Shoal Cave-specific persistent state should be introduced.

The correction must not fake elapsed days; it only changes the current local-time reference. Subsequent tide behavior therefore follows the normal existing hour checks.

## 9. RTC unavailable/error behavior

Before allowing edits, Phase 8 checks the existing RTC initialization/error state.

If no usable RTC source is available:

- display a clear Turkish message that RTC is unavailable
- advise enabling RTC support in the emulator/device
- do not modify `localTimeOffset`
- do not modify `lastBerryTreeUpdate`
- do not modify `VAR_DAYS`
- do not save a fabricated time source

Warnings that still provide a usable RTC value may be displayed as warnings, but invalid/probe-failure states must block correction.

Do not silently fall back to the existing dummy `2000-01-01` RTC value for a player-facing correction.

## 10. Vanilla hidden RTC recovery compatibility

The existing title-screen hidden RTC recovery path remains available.

Its legacy behavior may continue to use the vanilla reset flow, including `RtcReset()`, because it is retained as an emergency/recovery mechanism and is separate from the new normal Options flow.

Phase 8 must not remove:

- `EnableResetRTC`
- `DisableResetRTC`
- `CanResetRTC`
- the title-screen button-combination entry
- the existing hidden recovery screen path

The new in-game RTC mode must be distinguishable from the legacy hidden reset mode so their save/exit behavior cannot accidentally cross over.

## 11. Save behavior

Applying a confirmed RTC correction performs a normal save after all reference values have been realigned.

If saving fails:

- show the normal save-failed feedback
- do not claim the correction was safely persisted
- the player must be able to exit/retry without corrupting the save

No save migration is required for old saves.

A pre-Phase-8 save opened by a Phase-8 ROM should immediately use the new menu and existing stored time values.

## 12. Expected implementation areas

Expected files include, but are not limited to:

- `src/option_menu.c`
- `include/option_menu.h`
- `src/start_menu.c`
- `src/reset_rtc_screen.c`
- `include/reset_rtc_screen.h`
- `src/rtc.c`
- `include/rtc.h`
- `src/clock.c` only if a narrowly scoped helper is required
- `src/strings.c`
- `include/strings.h`
- `tools/vanillaplus_phase08_verify.py`
- `tools/phase_version_contract_test.py`
- `.github/workflows/build.yml`
- `Makefile`

No unrelated UI, save, clock, or event refactor belongs in Phase 8.

## 13. Build/version contract

Phase 8 owns the next Vanilla+ version marker:

- release: `ZUMRUT VP006`
- test: `ZUMRUT T006`
- player marker: `OYUNCU V+006`

Earlier phase verifiers remain version-agnostic with respect to Phase 8. The Phase 8 verifier owns exact `006` synchronization.

## 14. Automated verification

Add `tools/vanillaplus_phase08_verify.py` and run it in the normal build workflow after Phase 0–4 verifiers.

At minimum it must verify:

- in-game Options exposes RTC settings
- title/main-menu Options does not expose RTC settings
- no new save-layout field is introduced for Phase 8
- normal RTC correction does not call `RtcReset()`
- normal RTC correction recalculates/uses `localTimeOffset`
- correction realigns `lastBerryTreeUpdate`
- correction realigns `VAR_DAYS`
- correction does not clear daily flags
- correction does not directly invoke missed-day/per-day progression
- RTC-invalid path blocks changes
- vanilla hidden reset path remains present
- Phase 0–4 verifier commands remain in CI
- Phase 8 verifier is added to CI
- `006` build markers are synchronized
- `GAME_CODE` remains `BPEE`

The existing version-contract test should be extended so Phase 8 owns `006` without making older phase tests brittle.

## 15. Build/regression gate

Before Phase 8 is considered implementation-complete:

1. `tools/phase_version_contract_test.py` passes.
2. Phase 0 verifier passes.
3. Phase 1 verifier passes.
4. Phase 2 verifier passes.
5. Phase 3 verifier passes.
6. Phase 4 verifier passes.
7. Phase 8 verifier passes.
8. Release ROM builds successfully.
9. Test ROM builds successfully.
10. Both artifacts are uploaded and hashed by CI.

## 16. Manual smoke-test matrix

Use a copy of a known-good old save.

### 16.1 Basic compatibility

- Continue old save.
- Verify party, PC, bag, money, story location, flags, and current progression are intact.
- Save, close emulator, relaunch, Continue.

### 16.2 Menu visibility

- Title-screen Options: RTC row absent.
- In-game Start → Options: RTC row present.
- Normal Options changes still save/apply correctly.

### 16.3 Standard clock changes

Test at minimum:

- `14:00 -> 18:00`
- `18:00 -> 10:00`

For each:

- apply correction
- return to field
- reopen RTC settings and confirm displayed time
- save/reboot/continue
- confirm time continues progressing from the corrected reference

### 16.4 Advanced day changes

Test:

- day counter forward
- day counter backward

Confirm no freeze/lockout in future time-based processing.

### 16.5 Berry safety

- plant or use a berry tree with known state
- record its state
- perform forward correction
- verify no immediate artificial growth
- perform backward correction
- verify no regression corruption or long-term lockout
- allow real RTC time to pass and verify normal growth resumes

### 16.6 Daily-event safety

For at least one daily reward/event:

- claim today's reward
- correct time forward/backward
- verify the same day's reward is not immediately claimable again solely because of RTC correction
- verify the next legitimate day transition restores normal behavior

### 16.7 Shoal Cave

- test one high-tide hour
- correct to a low-tide hour
- verify cave state follows the corrected current local hour
- reverse the test

### 16.8 RTC unavailable

On an emulator/configuration with RTC disabled or invalid:

- open RTC settings
- verify a clear error is shown
- verify no correction can be applied
- verify save/time references remain unchanged

### 16.9 Legacy hidden recovery

If practical in the test environment, verify the existing hidden title-screen RTC recovery entry still reaches its original flow.

## 17. Acceptance criteria

Phase 8 is accepted when all of the following are true:

- old saves remain byte-layout compatible
- RTC settings are accessible from in-game Options only
- normal correction changes local game time without resetting hardware/emulator RTC
- forward and backward correction safely realign berry/day reference timestamps
- correction does not grant elapsed-time progression or duplicate daily rewards by itself
- time-based gameplay resumes normally after correction
- RTC-unavailable behavior fails safely
- legacy hidden RTC recovery remains intact
- all Phase 0–4 and Phase 8 automated verification passes
- release and test ROM builds pass
- manual RTC/berry/daily/Shoal Cave smoke tests show no regression

## 18. Non-goals

Phase 8 does not add:

- a real Gregorian calendar UI
- timezone selection
- daylight-saving-time logic
- network time synchronization
- software-clock fallback for RTC-less emulators
- arbitrary fast-forward/time-skip gameplay controls
- new daily events
- new berry mechanics
- new save fields

Those would be separate features with separate compatibility review.
