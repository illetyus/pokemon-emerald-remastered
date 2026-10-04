# Vanilla+ Phase 7 — Speed and Flow QoL

Date: 2026-10-02
Branch: `phase7-speed-qol`
Version owner: `VP016 / T016 / V+016`

## Goal

Reduce repeated waiting in ordinary play without changing battle calculations, turn order, damage timing, RNG rules, save layouts, story flags, or HM eligibility.

## Implemented scope

### 1. Fast text is actually fast

The existing `HIZLI` option remains the same saved value and UI choice. Its printer delay stays non-zero and compatible with the vanilla text state machine.

When `HIZLI` is selected, the printer may render up to four printable glyphs in one frame. A render barrier immediately stops the burst on control-state transitions such as waits, pauses, scrolling, callbacks, or end-of-text.

This avoids the unsafe shortcut of setting text speed to zero, which the engine interprets as a special immediate-draw path.

### 2. Menu/list responsiveness

The global held-key repeat timing changes from 40/5 frames to 24/3 frames (start/continue). This makes long lists, Bag, party and other repeat-input menus respond sooner while leaving single presses unchanged. Screens that intentionally override repeat timing keep their own values.

### 3. Shared HM / field-move presentation

The common field-move Pokémon banner is shortened rather than modifying each HM's badge checks or move logic.

- Outdoor banner expansion and contraction advance roughly twice as fast.
- The Pokémon sprite enters and exits at 40 px/frame instead of 20.
- Center hold time is 4 frames instead of 8.
- Indoor banner background movement is doubled.
- Indoor tile reveal/removal processes four columns per update so acceleration does not skip visual data.

Surf, Fly, Waterfall, Dive, Cut, Rock Smash, Strength, Flash and other users of the common show-mon effect benefit where they invoke it. Their underlying game-state logic remains unchanged.

### 4. Pokémon Center

Only the Pokémon Center variant of the shared glowing-Poké Ball effect is accelerated. Hall of Fame keeps vanilla timings.

Center timing changes:

- Ball placement interval: 25 -> 10 frames.
- Post-placement wait: 32 -> 12 frames.
- Flash interval: 8 -> 4 frames.
- Post-flash wait: 30 -> 12 frames.
- Follower return-to-ball delay: 21 -> 8 frames.
- Nurse bow delay: 4 -> 2 movement-delay units.

The healing fanfare is not truncated. The sequence still waits for the normal fanfare completion, avoiding clipped or overlapping audio.

## Deliberately unchanged

- Battle engine timing and controller delays.
- Damage/HP-bar timing.
- Battle animation timing.
- RNG and encounter logic.
- SaveBlock and PokémonStorage layouts.
- Story/event flags.
- Generic palette fades whose frame staging is used for VRAM safety.
- Battle-engine controller/timing constants.
- Hall of Fame heal presentation.

## Verification

`tools/vanillaplus_phase07_verify.py` checks the Phase 7 source invariants and synchronized VP016/T016/V+016 markers.

`.github/workflows/phase07-branch.yml` runs all completed phase verifiers and builds both release and test ROMs.
