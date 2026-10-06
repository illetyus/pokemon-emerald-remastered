# R16 — Vanilla+ QoL Completion Evidence

Branch: `r16-vanillaplus-qol`

Implementation acceptance head: `c11e52085c492a0adaf7e1de0a686cb5547ca10b`

Dedicated acceptance run:

- R16 Vanilla+ QoL #5 — run `37424844268` — SUCCESS.

## Status

**R16 COMPLETE at portable-core/source-host acceptance level.**

R16 applies only the approved Vanilla+ quality-of-life deltas that belong to
portable gameplay/domain authority. It does not copy GBA presentation timing or
UI implementation into the gameplay core.

## Source of truth

The pinned Vanilla+ source remains the specification. R16 audits the existing
Phase 3/4/5/6/8/9/10A and PC-management verifier contracts and classifies each
QoL behavior by its owning remaster subsystem.

The portable implementation is:

- `core/include/remaster/emerald_qol.h`
- `core/src/emerald_qol.c`

The Unreal preflight embeds the same C implementation through:

- `RemasterEmeraldQolEmbed.cpp`

There is no Unreal-owned duplicate QoL gameplay implementation.

## Accepted portable QoL behavior

R16 regression-pins the following gameplay/domain policies.

### Evolution / teaching

- Fire, Water, Thunder, Leaf, Moon and Sun Stones are reusable after a
  successful evolution.
- Other direct-use evolution items remain consumable.
- TM01-TM50 remain after successful teaching.
- HM01-HM08 remain reusable.
- HM moves may be replaced through the normal move-replacement flow.

### Move Relearner

- party shortcut eligibility rejects Eggs, a missing Heart Scale and Pokémon
  with no relearnable level-up moves;
- candidate moves are derived from the canonical species learnset up to the
  Pokémon's current level, excluding already-known moves;
- one Heart Scale is consumed only after a move is actually learned; cancelled
  or rejected attempts do not consume it.

### Field QoL

- Running is not gated by the old Running Shoes story flag.
- Field poison damage stops at 1 HP.
- Flash resolves to full visibility.
- Fishing uses the accepted 90-frame response window; rod-dependent mandatory
  reaction rounds remain Old Rod 1, Good Rod 1-3 and Super Rod 1-6, while the
  optional extra-round chance is exactly zero.
- Stationary bike switching is allowed.
- HM field access is derived from the exact Vanilla+ item + badge mapping;
  owning the badge alone or the HM alone is insufficient.

### Pokémon information / ownership policy

- total EV is calculated from the existing six stored EV values;
- Hidden Power type uses the Gen III six-IV parity mapping;
- nickname eligibility preserves player OT ID + OT name ownership and rejects
  Eggs;
- no IV/EV/Hidden-Power cache is added to persistent Pokémon data.

### PC / party management

- Bag sorting supports Name, Type, Quantity and Value for sortable pockets,
  with TM/HM and Berry canonical ordering preserved; PC item sorting uses the
  same modes over plaintext PC quantities;
- current-box sort supports Species, Level and Type modes;
- Eggs sort after ordinary Pokémon and empty slots sort last;
- current-box compaction preserves occupied order;
- box sort/compact moves complete 80-byte BoxPokemon records rather than
  reconstructing individual fields;
- quick deposit preserves the last-usable-party safeguard, mail safeguard and
  box-capacity failure;
- quick withdraw preserves party capacity and reconstructs party runtime stats
  from the canonical boxed Pokémon state;
- ordinary party held items can be moved/swapped while Mail remains excluded;
- PC Pokémon support direct GIVE from Bag and TAKE back to Bag for ordinary
  held items, with Egg, Mail, unholdable-item and Bag-capacity safeguards.

### Multi quick-item compatibility

R16 preserves the accepted Vanilla+ Phase 5 quick-item metadata format in the
existing SaveBlock1 reserve at `0x3598`. The runtime contract is exactly 21
bytes; initialization and updates must not overwrite the following reserve
bytes.

- no SaveBlock structure is enlarged;
- old single `registeredItem` state migrates lazily into slot 0;
- up to four quick-item IDs are retained;
- missing Bag items are pruned;
- quick slot 0 remains synchronized with the vanilla registered-item field.

This is deliberate compatibility with the existing Vanilla+ save-domain delta,
not a new remaster-owned save format.

## Already owned outside R16

R16 does not duplicate features that are already authoritative elsewhere:

- RTC correction / realignment remains R1-owned;
- main-story quest guidance remains R10-owned;
- the 386-species Phase 9 wild ecosystem remains R12-owned;
- pinned Vanilla+ map/script/data behavior already consumed by the R2/R3 world
  pipeline remains source truth rather than being hard-coded again in R16.

## Presentation-only QoL boundary

Vanilla+ speed/flow changes whose effect is purely presentation remain outside
the portable gameplay core. Examples include text burst rendering, key-repeat
timings, field-move banner animation speed and Pokémon Center presentation
timings.

Those belong to later UI/battle/audio/presentation phases. R16 intentionally
does not make battle outcomes, save state, encounter RNG or story state depend
on presentation timing.

## Regression evidence

The R16 workflow builds:

- `emerald_qol_test`;
- `unreal_cpp_embed_smoke`.

It executes:

- `r16_qol_behavior`;
- `r16_qol_source_audit`;
- `unreal_cpp_embed`;
- the pinned Vanilla+ source audit;
- the Unreal source-boundary validator.

Acceptance run:

- R16 Vanilla+ QoL #5 — `37424844268` — SUCCESS.

The source audit also guards the unchanged SaveBlock1/SaveBlock2/PokémonStorage
sizes and verifies that QoL ownership does not leak into the save structure.

## Exit checklist

- [x] approved portable QoL deltas are explicit;
- [x] dedicated behavior regression exists;
- [x] pinned Vanilla+ verifier contracts are audited;
- [x] HM item+badge policy is explicit;
- [x] evolution/TM/HM consumption policy is explicit;
- [x] field poison/Flash/fishing/running/bike policies are explicit;
- [x] Hidden Power/EV/nickname policy is explicit;
- [x] PC sort/compact/quick-transfer behavior is regression-pinned;
- [x] VP5 quick-item metadata remains save-layout compatible;
- [x] Unreal embeds the same portable QoL implementation;
- [x] presentation-only speed changes remain outside gameplay authority;
- [x] dedicated R16 CI is green.

Next phase: **R17 — Save compatibility / migration**.
