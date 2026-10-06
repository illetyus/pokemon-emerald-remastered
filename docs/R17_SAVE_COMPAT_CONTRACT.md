# R17 Save Compatibility Contract

Date: 2026-10-06  
Phase: R17-P2 — Source-format contract audit  
Status: corrected source-backed implementation contract
Pinned Vanilla+ authority: `illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`

This document freezes the save-format and failure-semantics contract that R17
must implement and test. Production source is authoritative. Existing remaster
tests are evidence only and must be corrected when they disagree with the
pinned source.

P2 closure corrected the original audit's use of stale source comments as
compiled sizes/offsets. The production Makefile selects AGBCC by default
(`MODERN=0`); its layout, not a host compiler's ABI, determines save bytes.
See section 15 for the actual AGBCC measurements and provenance. The existing
remaster `0xF44` / `0x3DC8` block constants match pinned Vanilla+ and must not
be shrunk to the stock Emerald sizes.

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

## 3. Authoritative logical block sizes and format distinction

Stock Emerald and pinned Vanilla+ use distinct compiled gameplay layouts:

| Block | Stock Emerald bytes | Pinned Vanilla+ bytes | Main-sector IDs |
| --- | ---: | ---: | --- |
| `SaveBlock2` | `0xF2C` | `0xF44` | 0 |
| `SaveBlock1` | `0x3D88` | `0x3DC8` | 1-4 |
| `PokemonStorage` | `0x83D0` | `0x83D0` | 5-13 |

Vanilla+ appends a `0x18`-byte Follower at SaveBlock2 `0xF2C` and widens
`ObjectEvent.graphicsId`. The compiled ObjectEvent stride is `0x28`, versus
stock `0x24`; 16 records shift the following SaveBlock1 fields by `0x40`.
The `sizeof` and offset comments in the pinned headers were not updated for
these extensions. `src/save.c` chunks actual `sizeof`, not those comments.

The corresponding logical checksum/payload sizes are therefore:

| Sector ID | Stock Emerald logical bytes | Pinned Vanilla+ logical bytes |
| ---: | ---: | ---: |
| 0 | `0xF2C` | `0xF44` |
| 1-3 | `0xF80` each | `0xF80` each |
| 4 | `0xF08` | `0xF48` |
| 5-12 | `0xF80` each | `0xF80` each |
| 13 | `0x7D0` | `0x7D0` |

Compiled PokemonStorage offsets are `currentBox=0`, `boxes=0x4`,
`boxNames=0x8344`, `boxWallpapers=0x83C2`. The `boxes` comment saying `0x1`
does not include the actual alignment padding.

R17 must explicitly select or unambiguously identify the source format before
applying format-specific checksums or domain offsets. The common signature
does not identify the format. Zero-filled tails can make both checksum spans
validate, so checksum probing alone is not an unconditional discriminator.
An ambiguous generic import requires explicit format/provenance selection;
it must not silently reinterpret stock flags/vars using Vanilla+ offsets.
The existing Vanilla+-specific path retains its declared pinned layout.

I1 owns format-aware sector validation. I2 owns domain mapping between the
selected source layout and the established runtime representation. I5 owns
the public ambiguous/unsupported result surface. Format support must not be
claimed solely because the raw block prefixes decode.

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

Only the selected format's authoritative logical size for that sector ID participates. Padding
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
wraparound is treated as the forward transition. Other pairs use ordinary
unsigned ordering, matching the production comparison rather than a general
half-range serial-number comparison. Equal counters preserve the existing
remaster tie-break: physical slot 0 wins. This is a deterministic policy for
synthetic equal-counter slots; the normal alternating writer does not produce
two complete slots with the same counter.

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

Offsets shared by both compiled formats are:

- SaveBlock2 encryption key: `0xAC`;
- SaveBlock2 local time offset: `0x98`;
- SaveBlock2 last berry update: `0xA0`;
- SaveBlock1 registered item: `0x496`;
- SaveBlock1 PC items: `0x498`;

Offsets after the saved ObjectEvent array are format-specific:

| SaveBlock1 field | Stock Emerald offset | Pinned Vanilla+ offset |
| --- | ---: | ---: |
| flags | `0x1270` | `0x12B0` |
| vars | `0x139C` | `0x13DC` |
| field named `unused_3598` | `0x3598` | `0x35D8` |

Field names and source offset comments are not a substitute for `offsetof`.

## 7. Encryption and Vanilla+ VP5 metadata

The Emerald encryption key remains the 32-bit value at SaveBlock2 `0xAC`.
R17 must preserve the existing domain-specific encryption/decryption semantics;
save compatibility is not permission to globally transform unrelated bytes.

Pinned production VP5 item-management metadata is stored through
`gSaveBlock1Ptr->unused_3598`. In the compiled Vanilla+ layout this existing
SaveBlock1 unused region starts at `0x35D8`, despite its historical field name:

- magic `0x35504956`;
- version 1;
- packed footprint exactly 21 bytes.

That metadata does not resize SaveBlock1. Reset/migration logic must touch only
the 21-byte metadata footprint and must not clear the full `unused_3598[0x180]`
region.

Current remaster `emerald_qol.c` uses literal `0x3598`, which is `0x40` before
the production Vanilla+ metadata field. This is a confirmed R17-I4 gap, not a
reason to move production metadata or enlarge a SaveBlock. Legacy remaster
metadata at that offset must be audited before any migration: do not assume
those bytes are unused in the selected format or silently clear/copy them.
Stock Emerald has no production VP5 metadata; stock import must not manufacture
it inside an unrelated gameplay field.

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

- compiled stock/Vanilla+ block constants and format-specific checksum lengths;
- explicit format selection and checksum-ambiguous input policy;
- two valid rotating slots, newest selected;
- counter wraparound;
- newest-slot checksum damage -> older valid slot recovery;
- mixed-counter slot rejected;
- missing sector / bad signature / bad checksum;
- import -> encode -> decode round trip;
- sectors 28-31 unchanged by normal store;
- encryption-key-dependent existing domain data remains stable;
- production VP5 metadata at compiled Vanilla+ `0x35D8` survives round trip
  without resizing blocks; adjacent bytes remain untouched;
- legacy remaster metadata boundary/migration is explicit and non-destructive;
- platform MISSING vs ERROR distinction;
- wrong-size existing file rejected without overwrite;
- failed write restores/retains pre-write metadata;
- representative world/progression, flags/vars, party/storage and bag data;
- real or legally redistributable provenance-pinned save fixtures where
  available, plus deterministic synthetic fixtures for edge cases.

Fixtures must not include ROM images, extracted commercial assets, credentials
or secrets.

## 13. R17 ownership map

- R17-I1: preserve correct pinned block sizes; add format-aware section/checksum
  validation, independent fixtures, slot choice, counter coherence and rotation.
- R17-I2: full-domain import reconstruction and stock/Vanilla+ offset mapping.
- R17-I3: export, untouched-byte/special-sector preservation and transactional
  write behavior.
- R17-I4: migration boundary, compiled VP5 offset `0x35D8`, and audit of the
  current remaster's incorrect literal `0x3598`.
- R17-I5: corrupt/unsupported/platform-I/O handling.
- R17-T1: compatibility fixture matrix.

R17-I1 starts with independent fixtures for both declared formats and the
explicit counter-coherence policy. Existing tests pinning `0xF44` / `0x3DC8`
correctly protect the pinned Vanilla+ layout. They must not be changed to stock
sizes under the name of a Vanilla+ compatibility fix.

## 14. P2 exit criteria

R17-P2 is complete when this contract is versioned on the live R17 branch and
no newer repo/source drift invalidates the evidence above.

The earlier versioned P2 contract failed this criterion because it confused
stale comments with compiled layout. Its closure requires the source-backed
correction and the production/stock distinction above to be versioned before
I1 resumes.

Implementation starts at R17-I1. No behavior change is part of P2.

## 15. P2 closure measurement evidence

Measured on 2026-10-06, without building a ROM or importing commercial assets:

- Vanilla+ production source:
  [`70db90c9077aed1272e746fc2537d9f12b95a91c`](https://github.com/illetyus/pokezumrut-vanillaplus/tree/70db90c9077aed1272e746fc2537d9f12b95a91c).
  Primary files: `Makefile`, `include/global.h`, `include/global.fieldmap.h`,
  `include/pokemon_storage_system.h`, `src/save.c`, `src/vanillaplus_items.c`.
- Stock Emerald comparison source:
  [`pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881`](https://github.com/pret/pokeemerald/tree/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881).
- Compiler source:
  [`pret/agbcc@da598c1d918402c42c0c0d7128ba14567f3175e9`](https://github.com/pret/agbcc/tree/da598c1d918402c42c0c0d7128ba14567f3175e9).
  Built only `gcc/agbcc` with `make -j1`. Assembly identifies itself as
  `gcc 2.9-arm-000512 for Thumb/elf`.

The layout probe emits an array of `sizeof` and `offsetof` constant expressions
from the unchanged source headers. AGBCC compiles the preprocessed translation
unit with `-mthumb-interwork -O2`; constants are read from emitted `.word`s.
Preprocessing uses `MODERN=0`, AGBCC's `ginclude`, source `include`/`gflib`,
and minimal `string.h`/`limits.h` declaration shims. The stock probe uses an
empty generated `map_groups.h`: no generated map IDs participate in these save
types. No shim changes the source structs. A generic host/modern compiler's
layout is explicitly not accepted as production AGBCC evidence.

| Expression | Pinned Vanilla+ | Stock Emerald |
| --- | ---: | ---: |
| `sizeof(void *)` | `0x4` | `0x4` |
| `sizeof(struct Follower)` | `0x18` | absent |
| `sizeof(struct ObjectEvent)` | `0x28` | `0x24` |
| `sizeof(struct SaveBlock2)` | `0xF44` | `0xF2C` |
| `sizeof(struct SaveBlock1)` | `0x3DC8` | `0x3D88` |
| `sizeof(struct PokemonStorage)` | `0x83D0` | `0x83D0` |
| `offsetof(SaveBlock2, follower)` | `0xF2C` | absent |
| `offsetof(SaveBlock2, encryptionKey)` | `0xAC` | `0xAC` |
| `offsetof(SaveBlock1, flags)` | `0x12B0` | `0x1270` |
| `offsetof(SaveBlock1, vars)` | `0x13DC` | `0x139C` |
| `offsetof(SaveBlock1, unused_3598)` | `0x35D8` | `0x3598` |
| `offsetof(PokemonStorage, boxes)` | `0x4` | `0x4` |

The production packed `VanillaPlusItemMetadata` declaration, copied unchanged
into a layout-only probe with its source constants, emits `sizeof=21`.
Normal save chunk sizes follow from these measured struct sizes and the
production `SAVEBLOCK_CHUNK` macro. Checksum/signature/slot semantics remain
source-backed as specified above.

This remeasurement agrees with the earlier production-layout and real-save
evidence in `docs/R1_SAVE_RTC.md`. That historical real-save report is secondary
evidence; no new real-save runtime validation was performed in this closure.

## 16. R17-I1 checkpoint and verification boundary

`remaster_emerald_save_validate` performs read-only sector/slot validation with
an explicit `RemasterEmeraldSaveFormat`. Stock and pinned Vanilla+ use their
own section 0/4 checksum lengths. No autodetection or heuristic layout choice
is performed: the matrix includes a zero-tail image that validates under both
formats, demonstrating why callers must provide format/provenance.

The accepted candidate must contain all 14 IDs with valid signatures/checksums
and a coherent counter. Selection preserves unsigned ordering, exact MAX/zero
wrap semantics, ID-driven ordering, and logical ID 0's physical rotation.
A damaged/incomplete/mixed-counter slot falls back only to an independently
valid slot. The established gameplay decoder delegates validation using the
pinned Vanilla+ format. Stock validation does not yet reconstruct stock
gameplay state; that is R17-I2.

Versioned synthetic fixture source: `tests/emerald_save_validation_test.c`,
CTest name `r17_save_sector_validation`. Fixture sizes/footer offsets are
literal source-backed values; fixture construction uses its own checksum,
not production constants or the production checksum function.

Local verification on the live I1 parent:

- Before implementation, 100 checks ran against the existing decoder:
  42 mixed-counter recovery/rejection checks failed; process exit was 1.
- After implementation, 284 checks pass in both strict C99 and C++17 builds.
- AddressSanitizer/UndefinedBehaviorSanitizer: 284 checks pass with leak
  detection disabled because LeakSanitizer cannot run under this executor's
  ptrace environment. This does not establish a leak-sanitizer result.
- Existing native `emerald_save_test` and `emerald_save_platform_test` pass.

The matrix covers both formats, every rotation in either slot, counter
ordering/wrap/ties, all physical mixed-counter positions, checksum tail/padding
boundaries, ID bounds/duplicates, bad signatures/checksums, incomplete and
cross-format slots, explicit ambiguous-format selection, read-only probing,
and exclusion of special-sector footers from main-slot selection.

I1 verification requires the exact committed HEAD's targeted test and complete
relevant PR CI to reach terminal-success. No later subphase starts on pending
or failed CI. Failure -> a gap-specific I1 closure; success -> R17-I2.
No full R17 acceptance or new real-save runtime result is claimed here.
Domain reconstruction, export transactionality/preservation, VP5 migration,
and platform OK/MISSING/ERROR semantics remain in I2/I3/I4/I5 respectively.
