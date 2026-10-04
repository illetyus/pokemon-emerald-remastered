# Vanilla+ PC / Pokemon Management QoL — VP019

Date: 2026-10-02
Branch: `phase-pc-management-qol`
Version owner: `VP019 / T019 / V+019`

## Constraints

- Preserve old save compatibility.
- Do not change `struct PokemonStorage`, `SaveBlock1`, or `SaveBlock2` layout.
- Never reconstruct a stored Pokemon to sort or organize it; move complete `struct BoxPokemon` records intact.
- Existing party, box, held-item, summary, multi-move and release behavior must remain valid.

## Delivery slices

1. Current-box sorting:
   - Species: National Dex ascending.
   - Level: highest to lowest, National Dex tie-break.
   - Type: primary type, secondary type, National Dex tie-break.
   - Empty slots last.
   - Eggs after normal Pokemon and before empty slots.
2. Held-item visibility and lower-friction item management.
3. Faster Box <-> Party operations.
4. Box organization shortcuts.
5. Full regression and field-test ROM.

## First vertical slice

The box-title options menu gains three safe sort actions. Sorting is disabled while a Pokemon is being held and in Move Items mode. The implementation uses a stable in-place insertion sort over the current box's existing `BoxPokemon` records, then rebuilds only the on-screen icon layer.


## Implemented VP019 feature bundle

- Box-title `SIRALA` submenu:
  - Species / National Dex order.
  - Level descending.
  - Primary/secondary type order.
  - `TOPARLA` compacts occupied slots without otherwise reordering them.
- Normal PC information panel labels held items explicitly with `E:`.
- Normal Pokémon context menu can give an item from the Bag or send an existing non-mail held item directly to the Bag.
- START is a quick transfer shortcut:
  - occupied box slot -> Party in Move/Withdraw modes;
  - Party Pokémon -> current box in Move/Deposit modes.
- Quick deposit still enforces last-usable-party, mail and full-box safeguards.
- All organization operations move complete `BoxPokemon` records; no new persistent storage state was added.
