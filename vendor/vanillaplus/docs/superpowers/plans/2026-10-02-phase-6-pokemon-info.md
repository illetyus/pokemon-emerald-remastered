# Vanilla+ Phase 6 — Pokémon Info & Management Plan

## Goal

Complete Phase 6 without changing Pokémon/save binary formats.

## Scope

1. Summary Skills page advanced view
   - SELECT toggles normal stats <-> IV/EV view.
   - Show all six IV and EV values.
   - Show Hidden Power type and total EV.
   - Preserve existing Nature up/down stat colors in normal view.
2. Battle Moves page
   - Preserve existing Type / Power / Accuracy / PP / description implementation.
3. Party-menu Move Relearner shortcut
   - Available from the normal field party action menu.
   - Preserve one Heart Scale cost.
   - Consume the Heart Scale only after a move is actually learned.
   - Reject Eggs / no relearnable moves / missing Heart Scale cleanly.
   - Return to the open field menu rather than a script.
4. Party-menu nickname shortcut
   - Use the existing naming screen.
   - Preserve vanilla Name Rater ownership rules: player OT ID and OT name must match.
   - Reject Eggs and traded/foreign Pokémon.
   - Return to the open field menu.
5. Versioning
   - Phase 6 owns VP008 / T008 / V+008.
   - Older phase verifiers become version-agnostic.

## Safety invariants

- Do not modify SaveBlock layouts.
- Do not modify BoxPokemon/Pokemon serialized data layouts.
- Read IV/EV from existing MON_DATA fields only.
- Existing Summary normal behavior, move reordering, HM replacement, Phase 5 held-item menu, and RTC remain intact.
- Build both release and VANILLAPLUS_TEST ROMs.

## TDD sequence

1. Add Phase 6 verifier and confirm it fails against Phase 5 source.
2. Implement summary advanced-view data extraction and rendering.
3. Implement direct Move Relearner wrapper and party-menu callback.
4. Implement nickname shortcut and ownership checks.
5. Bump VP008/T008/V+008 and update version ownership contract.
6. Run Phase 0-6 and Phase 8 verifiers.
7. Build release and test ROMs and upload artifacts.
8. Field-test before merging to main.

## Implementation checkpoint

Generated Phase 6 sources are committed on the branch. The branch CI is rerun from committed sources to verify the transformation is idempotent and both VP008/T008 builds remain green before field testing.

## Field-test polish checkpoint

Build 011 addresses the final field-test findings: Phase 6 warning messages remain visible until A/B dismissal, and the IV/EV display uses fixed IV/EV columns with aligned values.
