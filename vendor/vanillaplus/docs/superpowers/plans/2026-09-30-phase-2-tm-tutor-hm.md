# Vanilla+ Phase 2 — Reusable TM, Tutor and HM QoL

## Goal
Implement Phase 2 without changing the serialized save layout or Gen III move compatibility data.

## Requirements
- Technical Machines (TM01–TM50) are not consumed after a successful teach.
- Hidden Machines keep their existing reusable behavior.
- One-time overworld Move Tutors can teach repeatedly, including on saves where their old one-time flag is already set.
- HM moves may be replaced through the normal learn/replace-move flow; the dedicated Move Deleter remains valid but is no longer required.
- Tutor/TM learnability, moves, friendship behavior and battle mechanics remain unchanged.
- SaveBlock1, SaveBlock2 and PokemonStorage layouts are untouched.

## Implementation
1. Add an automated Phase 2 source verifier and make it fail against the Phase 1 source.
2. In `src/party_menu.c`, stop removing a TM from the bag in `Task_LearnedMove`.
3. In `data/scripts/move_tutors.inc`, remove one-time flag gates, one-use warning calls, and one-time flag writes while preserving normal yes/no and party-menu flow.
4. In `src/pokemon_summary_screen.c`, make `CanReplaceMove` permit replacing HM moves.
5. Run Phase 1 + Phase 2 source verification.
6. Build both release and test ROM configurations.
7. Remove the one-time updater and rerun verification/build from clean `main`.

## Automated checks
- No TM consumption call remains in `Task_LearnedMove`.
- Move Tutor scripts contain no one-time flag gate/write and no one-use warning call.
- `CanReplaceMove` no longer rejects HMs.
- Existing Phase 0–1 verifier still passes.
- Both release and test ROMs compile.

## Device smoke test
Using the test ROM/save rather than hours of normal progression:
- Teach one TM to two compatible Pokémon; TM remains in bag after both.
- Use the same overworld tutor twice.
- Replace an HM move while learning another move.
- Save, restart and Continue using the existing 128 KiB save.
