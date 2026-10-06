# R17 Save Compatibility Contract

Date: 2026-10-06  
Phase: R17-P2 — Source-format contract audit  
Status: accepted implementation contract  
Pinned Vanilla+ authority: `illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`

This document freezes the save-format and failure-semantics contract that R17
must implement and test. Production source is authoritative. Existing remaster
tests are evidence only and must be corrected when they disagree with the
pinned source.

## 1. Compatibility objective

R17 must read and write normal 128 KiB Pokemon Emerald / pinned Vanilla+ save
images without changing the authoritative gameplay layout. A valid input save
must survive import -> remaster -> export with the expected alternating-slot,
sector-rotation, checksum, counter and special-sector semantics intact.

R17 must not enlarge or shift `SaveBlock1`, `SaveBlock2` or
`PokemonStorage` to make remaster-only state fit. Existing accepted Vanilla+
metadata may use already-unused bytes only where its offset and size are proven
compatible.

## 2. Physical save geometry

The complete save image is exactly:

- 128 KiB total;
- 32 physical sectors;
- 4096 bytes per sector;
- 3968 bytes (`0xF80`) of logical sector payload capacity.

Physical sector ownership is:

- sectors 0-13: normal save slot 1;
- sectors 14-27: normal save slot 2;
- sectors 28-29: Hall of Fame;
- sector 30: Trainer Hill;
- sector 31: Recorded Battle.

A normal game-save write updates one 14-sector main slot. It must not erase or
rewrite sectors 28-31. Special-sector writes remain separate operations.

## 3. Authoritative logical block sizes

Pinned Vanilla+ production source fixes the gameplay blocks at:

| Block | Authoritative bytes | Main-sector IDs |
| --- | ---: | --- |
| `SaveBlock2` | `0xF2C` | 0 |
| `SaveBlock1` | `0x3D88` | 1-4 |
| `PokemonStorage` | `0x83D0` | 5-13 |

The corresponding logical checksum/payload sizes are therefore:

| Sector ID | Logical bytes |
| ---: | ---: |
| 0 | `0xF2C` |
| 1 | `0xF80` |
| 2 | `0xF80` |
| 3 | `0xF80` |
| 4 | `0xF08` |
| 5-12 | `0xF80` each |
| 13 | `0x7D0` |

The current remaster constants `0xF44` for SaveBlock2 and `0x3DC8` for
SaveBlock1 are not authoritative. They make sector 0 and sector 4 checksum spans
too large and are R17-I1 blockers. Existing tests that pin those values must be
updated rather than treated as authority.

## 4. Sector footer and checksum

For each normal main-save sector:

- sector ID is stored at byte offset 4084 (`0xFF4`);
- checksum is stored at 4086 (`0xFF6`);
- signature is stored at 4088 (`0xFF8`);
- save counter is stored at 4092 (`0xFFC`);
- normal signature is `0x08012025`.

The checksum is the Emerald 16-bit fold of a 32-bit additive sum:

1. interpret the logical payload as little-endian 32-bit words;
2. sum exactly `logical_size / 4` words in 32-bit arithmetic;
3. return `(sum >> 16) + sum` truncated to 16 bits.

Only the authoritative logical size for that sector ID participates. Padding
between the logical payload and footer is not part of the checksum.

## 5. Slot validity, selection and rotation

A normal valid slot contains all logical sector IDs 0-13, each with the normal
signature and checksum valid for that ID's authoritative logical size.

The production writer:

- increments the save counter for a new normal save;
- alternates slot by counter parity;
- increments the rotation modulo 14;
- writes the same new counter to every sector in the new slot;
- maps physical sector as `(logical_id + rotation) % 14` inside the selected
  slot.

The physical position of logical sector ID 0 is therefore the persisted rotation
/ last-written-sector reference used by the remaster model.

When both slots are valid, the newer counter wins. The `UINT32_MAX -> 0`
wraparound is treated as the forward transition.

### Counter-coherence hardening

Pinned Emerald/Vanilla+ validation records the counter observed from valid
sectors but does not explicitly reject a synthetic slot whose individually valid
sectors carry mixed counters. Such a slot cannot be emitted by the authoritative
normal writer.

R17 must therefore require all 14 sectors of an accepted slot to have one
coherent counter. This is a corruption-hardening rule for impossible writer
output, not a claim that the original reader performed this explicit check.

If a newest slot is invalid and an older complete slot is valid, load the older
slot and surface degraded/recovery status rather than reconstructing a hybrid
slot.

## 6. Reconstruction boundaries

Logical domain mapping remains:

- ID 0 -> SaveBlock2;
- IDs 1-4 -> consecutive SaveBlock1 chunks;
- IDs 5-13 -> consecutive PokemonStorage chunks.

R17 must reconstruct a selected slot only from sectors belonging to that one
validated slot. It must not combine logical IDs from different slots or
different counters.

The following established offsets remain authoritative where already used by
the remaster:

- SaveBlock2 encryption key: `0xAC`;
- SaveBlock2 local time offset: `0x98`;
- SaveBlock2 last berry update: `0xA0`;
- SaveBlock1 registered item: `0x496`;
- SaveBlock1 PC items: `0x498`;
- accepted Vanilla+ metadata area begins at SaveBlock1 `0x3598`.

## 7. Encryption and Vanilla+ VP5 metadata

The Emerald encryption key remains the 32-bit value at SaveBlock2 `0xAC`.
R17 must preserve the existing domain-specific encryption/decryption semantics;
save compatibility is not permission to globally transform unrelated bytes.

R16's accepted VP5 item-management metadata is stored inside Vanilla+'s existing
SaveBlock1 unused region at `0x3598`:

- magic `0x35504956`;
- version 1;
- packed footprint exactly 21 bytes.

That metadata does not resize SaveBlock1. Reset/migration logic must touch only
the 21-byte metadata footprint and must not clear the full `unused_3598[0x180]`
region.

## 8. Special-sector preservation

Normal save/export must preserve physical sectors 28-31 byte-for-byte from an
existing valid 128 KiB image unless the caller explicitly performs the matching
Hall of Fame, Trainer Hill or Recorded Battle operation.

A normal save must never turn a transient read failure into a blank 128 KiB
image, because doing so would also destroy those special sectors.

## 9. Platform I/O result contract

The current platform callback returns only boolean success/failure, so a missing
save and a genuine read error are indistinguishable. R17 must introduce or wrap
an explicit read result with at least:

- FOUND / OK;
- MISSING;
- ERROR.

Required behavior:

- load + MISSING -> `REMASTER_EMERALD_SAVE_EMPTY`;
- load + ERROR -> error/corrupt path, never EMPTY;
- load + FOUND with size other than 128 KiB -> unsupported/corrupt, never EMPTY;
- store + MISSING -> a new 128 KiB image may be initialized deliberately;
- store + ERROR -> abort without write;
- store + FOUND wrong size -> abort without write;
- absence of a usable read path must not silently authorize overwrite of an
  existing slot.

R17-I5 owns the public/platform error surface. R17-I3 owns preservation and
transactional write behavior.

## 10. Transactionality

A failed platform write must not leave the in-memory `RemasterEmeraldSave`
metadata pretending that the save succeeded.

In particular, `counter`, `last_written_sector`, `selected_slot` and
`status` must reflect the last committed persistent image after failure, or be
restored to their pre-write values.

Image preparation may occur in scratch memory, but the caller-visible metadata
commit point is successful persistent write completion.

## 11. Corruption and unsupported-input policy

R17 may safely reject inputs that cannot be produced by a valid normal
Emerald/Vanilla+ writer, including:

- wrong total image size;
- invalid/missing required sector IDs;
- bad signature;
- checksum mismatch;
- mixed counters inside one candidate slot;
- unsupported layout/version where authoritative interpretation is impossible.

No silent repair may fabricate gameplay state across slots. Recovery is limited
to selecting an independently valid backup slot.

## 12. Fixture and regression requirements

R17-T1 must pin at least these cases:

- authoritative block constants and exact section checksum lengths;
- two valid rotating slots, newest selected;
- counter wraparound;
- newest-slot checksum damage -> older valid slot recovery;
- mixed-counter slot rejected;
- missing sector / bad signature / bad checksum;
- import -> encode -> decode round trip;
- sectors 28-31 unchanged by normal store;
- encryption-key-dependent existing domain data remains stable;
- VP5 21-byte metadata survives round trip without resizing blocks;
- platform MISSING vs ERROR distinction;
- wrong-size existing file rejected without overwrite;
- failed write restores/retains pre-write metadata;
- representative world/progression, flags/vars, party/storage and bag data;
- real or legally redistributable provenance-pinned save fixtures where
  available, plus deterministic synthetic fixtures for edge cases.

Fixtures must not include ROM images, extracted commercial assets, credentials
or secrets.

## 13. R17 ownership map

- R17-I1: authoritative block sizes, sector/checksum validation, slot choice,
  counter coherence and rotation.
- R17-I2: full-domain import reconstruction gaps.
- R17-I3: export, untouched-byte/special-sector preservation and transactional
  write behavior.
- R17-I4: migration boundary and VP5 compatibility.
- R17-I5: corrupt/unsupported/platform-I/O handling.
- R17-T1: compatibility fixture matrix.

R17-I1 must start by correcting the authoritative block/checksum geometry and
the tests that currently pin the wrong remaster values. No later R17 slice may
paper over those constants with ad-hoc checksum exceptions.

## 14. P2 exit criteria

R17-P2 is complete when this contract is versioned on the live R17 branch and
no newer repo/source drift invalidates the evidence above.

Implementation starts at R17-I1. No behavior change is part of P2.
