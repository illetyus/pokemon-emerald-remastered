# Phase 5 — Item Management Design

Date: 2026-10-01
Repository: `illetyus/pokezumrut-vanillaplus`
Status: Approved design

## Goal

Complete the item-management QoL layer without shifting the vanilla Emerald save layout or invalidating existing saves.

## Player-facing behavior

### Bag sorting
- Keep vanilla SELECT manual move behavior.
- START opens a sort menu where appropriate.
- Supported criteria: Name, Type, Quantity, Value, and Cancel.
- TM/HM and Berry pockets retain their canonical numbered ordering by default.
- Name sorting uses Turkish-aware collation for Ç/Ğ/İ/Ö/Ş/Ü rather than raw charmap byte order.
- Sorting is deterministic; ties fall back to item ID.

### Auto sort
- The last explicit sort criterion is remembered per pocket.
- Auto-sort can be enabled/disabled from the sort menu.
- Auto-sort happens at safe list refresh/open boundaries, not inside low-level AddBagItem/RemoveBagItem primitives.

### PC item storage
- START opens the same logical sort choices while browsing stored items.
- PC item sorting uses the same comparator core as the Bag.

### Held item management
- Party Item submenu becomes GIVE / TAKE / MOVE / CANCEL where legal.
- MOVE asks for a destination Pokémon.
- Empty destination: move source held item.
- Occupied destination: swap held items.
- Mail follows the existing mail path and is not silently moved through the generic held-item operation.

### Quick registered items
- Existing single `registeredItem` remains the vanilla-compatible primary registered item.
- Up to four quick items can be registered.
- With one valid quick item, SELECT preserves direct-use behavior.
- With multiple valid quick items, SELECT opens a compact quick-item chooser; choosing one invokes the existing registered-item field-use path.
- Removing an item from the bag prunes it from the quick list.

## Persistence and compatibility

- `sizeof(struct SaveBlock1)` and `sizeof(struct SaveBlock2)` must not change.
- Existing offsets for party, money, `registeredItem`, PC items, Bag pockets, flags, vars, and all following fields must not shift.
- Do not add a new persistent field to SaveBlock1/2.
- Phase 5 metadata is stored inside the already-reserved `SaveBlock1.unused_3598[0x180]` byte region through accessors; the field declaration itself remains unchanged.
- Metadata has magic/version bytes. An old save with zero/garbage in the reserved region initializes safe defaults lazily.
- Default sort is manual for normal pockets; TM/HM and Berries preserve vanilla canonical sorting.

## Architecture

Create `include/vanillaplus_items.h` and `src/vanillaplus_items.c` for comparator, persistence and quick-item policy. UI integration remains in the existing Bag, Player PC and Party Menu files.

The common sorter accepts an ItemSlot array plus a quantity accessor mode so Bag encrypted quantities and PC plaintext quantities are handled correctly. UI code never reimplements comparison rules.

## Regression contracts

- Old save still loads and saves.
- New Game still initializes correctly.
- Bag quantities remain intact after every sort mode.
- Manual SELECT move continues to work.
- TM/HM and Berry order remains canonical.
- Selling, depositing, withdrawing, tossing and giving items still refresh correctly.
- Registered bike swapping continues to update the primary quick item if relevant.
- Battle Pyramid bag is untouched.
- Union Room / Multi Partner / Battle Pyramid/Pike registered-item restrictions remain intact.
- Phase 0–4 and Phase 8 verifier contracts remain green.
- Release and test ROMs both build.
