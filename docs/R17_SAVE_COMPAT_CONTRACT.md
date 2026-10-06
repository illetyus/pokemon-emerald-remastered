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

Before I4, remaster `emerald_qol.c` used literal `0x3598`, `0x40` before
the production Vanilla+ metadata field. I4 corrects ordinary QoL access to
`0x35D8`; the old address is reserved for an explicit provenance-gated recovery
operation. It overlaps production Mystery Gift data and is never automatically
copied or cleared. Stock Emerald has no production VP5 metadata: its item
extensions live only in a runtime sidecar, never in a serialized source block.
Section 19 defines recovery, unsupported-version and reset boundaries.

## 8. Special-sector preservation

Normal save/export must preserve physical sectors 28-31 byte-for-byte from an
existing valid 128 KiB image unless the caller explicitly performs the matching
Hall of Fame, Trainer Hill or Recorded Battle operation.

A normal save must never turn a transient read failure into a blank 128 KiB
image, because doing so would also destroy those special sectors.

## 9. Platform I/O result contract

I5 adds a preferred typed `save_read_result` callback with:

- FOUND / OK;
- MISSING;
- ERROR.

The legacy boolean callback remains source-compatible after recompilation.
Its positive result means FOUND; its ambiguous failure means ERROR and cannot
authorize EMPTY or initial creation. The typed callback takes precedence.
The I5 implementation/result matrix is in section 20.

Required behavior:

- load + MISSING -> `REMASTER_EMERALD_SAVE_EMPTY`;
- load + ERROR -> error/corrupt path, never EMPTY;
- load + FOUND with size other than 128 KiB -> unsupported/corrupt, never EMPTY;
- store + MISSING -> a new 128 KiB image may be initialized deliberately;
- store + ERROR -> abort without write;
- store + FOUND wrong size -> abort without write;
- absence of a usable read path must not silently authorize overwrite of an
  existing slot.

An existing filesystem image with no independently valid normal slot is
CORRUPT even if the raw flash validator calls signatureless slots EMPTY.
This is explicit filesystem safety policy, not a change to the pinned raw
flash reader. New creation requires confirmed MISSING and a deliberate initial
EMPTY checkpoint. A loaded game whose file disappeared must not silently
initialize a replacement.

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
  old remaster's incorrect literal `0x3598` (section 19).
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


## 17. R17-I2 domain import checkpoint

`remaster_emerald_save_decode_format` imports an explicitly selected stock or
pinned Vanilla+ image. All logical bytes remain in their original source
layout; no migration, metadata synthesis or persistent block expansion occurs.
`RemasterEmeraldSave.source_is_stock` is runtime provenance appended outside
its raw block arrays. It is never serialized. Zero-initialized existing callers
and the established `remaster_emerald_save_decode` retain Vanilla+ behavior.
Flags/vars, NPC templates, saved opaque ObjectEvents and encounter persistent
fields now select the measured source layout. Other owned readers use shared
party/storage, bag, world/warps, encryption-key and RTC offsets. Raw PC item,
box-name/wallpaper, follower and otherwise unmodeled block bytes are retained.

The layout-only AGBCC probe used section 15's unchanged pinned sources/compiler
and emitted these additional constants:

| Expression | Pinned Vanilla+ | Stock Emerald |
| --- | ---: | ---: |
| `offsetof(SaveBlock1, playerPartyCount)` | `0x234` | `0x234` |
| `sizeof(playerPartyCount)` | **1** | **1** |
| `offsetof(SaveBlock1, objectEventTemplates)` | `0xCB0` | `0xC70` |
| `sizeof(ObjectEventTemplate)` | `0x18` | `0x18` |
| template `graphicsId` offset / size | `0x2` / 2 | `0x1` / 1 |
| template `kind` offset | `0x1` | `0x2` |
| `offsetof(SaveBlock1, outbreakPokemonSpecies)` | `0x2BD0` | `0x2B90` |
| `offsetof(SaveBlock1, outbreakPokemonProbability)` | `0x2BE1` | `0x2BA1` |
| `offsetof(SaveBlock1, roamer)` | `0x321C` | `0x31DC` |
| `sizeof(Roamer)` | `0x1C` | `0x1C` |

This closes two additional production import errors: party count previously
read padding as part of a u32, and outbreak/roamer readers used stock offsets
against Vanilla+ blocks. Party-count writes now modify only the source u8.
The existing encounter-special fixture now uses measured Vanilla+ offsets;
its encounter mechanics and expected outcomes are unchanged. Stock template
writes reject graphics IDs above 255 rather than truncating them. Opaque stock
ObjectEvent reads copy 0x24 source bytes and zero-pad the 0x28-byte output;
writes consume only 0x24 bytes, preserving the following record/template.

Versioned independent synthetic fixture: `tests/emerald_save_import_test.c`,
CTest `r17_save_domain_import`. It builds source bytes/checksums without the
production domain setters or layout constants. It exercises both formats,
both selected slots and every rotation; all persistent flags/vars, all six
party records, 420 storage records, five bag pockets, world/warps/currency,
RTC, NPC templates, objective derivation and progression mutations, raw-block
preservation, invalid API parameters, and outbreak/roamer reconstruction.
No new real-save/emulator validation is claimed.

Local verification (encounter catalog unavailable in this partial workspace):

- Before implementation, the final fixture against exact pre-I2 sources with
  `R17_BASELINE`/`R17_NO_ENCOUNTERS`: **189257 checks, 16103 failures**, exit 1.
- After implementation, with only `R17_NO_ENCOUNTERS`: **189257 checks,
  0 failures** in strict C99 and C++17 builds.
- C and C++ ASan/UBSan runs pass the same 189257 checks; LeakSanitizer is disabled
  because this execution environment uses ptrace, as recorded in section 16.
- Eleven existing native save/platform/sector/state/quest/quest-save/Pokemon-
  codec/party-storage/bag/object-state/RTC tests pass.
- CI's normal CMake target defines neither local-only switch. It must run the
  outbreak and roamer cases against the repository's generated catalog, as
  well as the complete relevant regression suite, on the exact committed HEAD.

At the I2 checkpoint, stock export was deliberately guarded until I3: the
existing Vanilla+ encoder returned failure for an imported stock wrapper
without mutating image/counter. Section 18 supersedes that temporary guard.
The legacy platform loader still selects Vanilla+; exposing explicit format
selection through its safe I/O surface remains in I5. VP5 readers/migration
remain an I4 gap. This checkpoint does not claim stock round-trip or full R17
completion. I2 advances to I3 only after exact-HEAD targeted/full CI is
terminal-success; a failing case requires a concrete I2 closure.


## 18. R17-I3 export and write transaction checkpoint

Normal export now uses the imported wrapper's explicit source layout: stock
section 0/4 spans are 0xF2C/0xF08; pinned Vanilla+ spans are 0xF44/0xF48.
There is no automatic stock-to-Vanilla+ conversion or VP5 synthesis. All raw
logical block bytes, including unmodeled fields, production VP5 at 0x35D8 and
its neighbors, remain source-layout bytes across reimport. Runtime provenance
is not serialized. The temporary I2 stock-export guard is removed.

Pinned `src/save.c` at section 15's source SHA defines the writer contract:
`WriteSaveSectorOrSlot` increments counter/rotation and restores its backups
on failure (lines 139-174); `HandleWriteSector` chooses slot by counter parity,
clears all 4096 scratch bytes, copies only the logical span and writes the
footer/checksum (lines 177-208). Export retains these writer semantics. The
selected write slot's out-of-domain padding is deliberately canonicalized to
zero; arbitrary old padding is not persistent gameplay data. The other main
slot and every byte of sectors 28-31 remain unchanged on a successful normal
write against an existing correctly sized image. Failed/wrong-size platform
reads still need the I5 safe read-result closure below.

`prepare_next_image` takes a const wrapper and produces scratch bytes plus a
small counter/rotation/slot plan. `save_store_platform` commits that plan only
after `save_write` returns success. The callback observes the original wrapper
while writing; failures retain all metadata and pending domain edits. Repeated
failed attempts prepare the same image/counter/rotation. The direct in-memory
`save_encode_next` commits its plan when image preparation succeeds, since that
API itself has no persistent callback. No large wrapper copy or new persistent
field is needed for the transaction.

The existing Unreal writer also wrote directly to the committed path. It now
writes a unique same-directory temporary file, closes it through
`FFileHelper::SaveArrayToFile`, then uses the shared `RemasterAtomicSave`
protocol to replace the destination. POSIX uses `rename`; Windows native wide
paths use `MoveFileExW` with replacement and write-through flags. Neither path
deletes the committed destination before replacement. A failed prepare or
replace attempts temporary cleanup and returns failure, so core metadata does
not advance. The native filesystem test consumes this same protocol/replace
primitive. A Windows MSVC job exercises its wide-path implementation; Linux
runs it in the full portable suite. This is file transport only, not gameplay
ownership in Unreal. Read-result semantics remain unchanged for I5.

Independent versioned fixtures:

- `tests/emerald_save_export_test.c`, CTest `r17_save_export_transactionality`:
  both formats, every rotation, normal and UINT32_MAX/zero counters, three
  sequential exports per fixture, independently checked footer/checksum/span,
  whole logical blocks, other-slot/special-sector preservation, source padding
  policy, failure/retry/first-write behavior, and callback-visible commit point.
- `tests/r17_atomic_file_save_test.cpp`, CTest `r17_atomic_file_save`: native
  temporary-directory files, Unicode destination, injected short-write/close/
  replace failures, actual missing-source rename failure, cleanup protocol,
  successful replacement, and first creation. Cleanup is best effort in the
  production adapter; tests inject failure before cleanup and run with a
  writable temporary directory.
- The existing I2 import fixture now asserts stock export/reimport instead of
  its former temporary rejection guard. Domain expectations remain unchanged.

Local verification:

- Export regression before implementation: **25990 checks, 11458 failures**,
  exit 1; after implementation: **26006 checks, 0 failures**. More callbacks
  execute once stock writes are supported, explaining the check-count delta.
- Strict C99/C++17 and C/C++ ASan/UBSan export runs pass all 26006 checks.
  LeakSanitizer remains disabled under ptrace as in section 16.
- Native direct-write baseline: **20 checks, 8 failures**; final shared atomic
  protocol: **24 checks, 0 failures**, including actual POSIX replacement.
  Native C++ ASan/UBSan passes the same 24 checks.
- Existing save compatibility/platform tests and 284-check sector validation
  pass. Updated I2 local import mode passes **189258 checks**, excluding the
  generated encounter cases that full CI must run.
- Native dependencies match exact live core blobs; source/save ABI evidence
  and canonical docs were reread before implementation. No vendor edit, ROM,
  commercial extracted asset or workflow-generated source commit is involved.

I3 is VERIFYING until the exact committed HEAD's full relevant PR CI and
Windows native atomic transport job are terminal-success. Success -> R17-I4;
failure -> a concrete I3 closure. Neither this native test nor the source
adapter change is a real Unreal compile/cook/package, Android filesystem
smoke, or power-loss durability result. Those execution environments remain
in R18/runtime validation. I4 still owns VP5 migration and preference
boundaries; I5 still owns MISSING versus ERROR, unsupported/corrupt input and
explicit-format platform loading. Full R17 acceptance remains open.

## 19. R17-I4 migration and metadata checkpoint

### Source audit and exact footprint

Pinned `src/vanillaplus_items.c` lines 30-105 define the packed version-1
metadata and its lazy reset policy. Validity checks magic, version, count <= 4
and each of the five sort modes < 5. Lazy reset clears exactly 21 bytes and
seeds quick slot 0 from `registeredItem` without changing that source field.
The remaster retains this behavior at the measured production address.
Import/export themselves do not initialize or migrate metadata.

A fresh AGBCC (`MODERN=0`) offsetof/sizeof probe against pinned `global.h`
and the source layout types measured these Vanilla+ boundaries:

| SB1 field | Offset | Size |
| --- | ---: | ---: |
| Mystery Gift | `0x326C` | `0x36C` |
| Mystery Gift card metadata | `0x3580` | `0x24` |
| Mystery Gift questionnaire | `0x35A4` | `0x8` |
| Mystery Gift news metadata | `0x35AC` | `0x4` |
| Mystery Gift trainer IDs | `0x35B0` | `0x28` |
| `unused_3598` | `0x35D8` | `0x180` |

Consequently the legacy remaster footprint `0x3598..0x35AC` overlaps the last
12 card-metadata bytes, all eight questionnaire bytes and the first news byte.
These are not reserved bytes. Stock's same numeric address is unused, but
stock has no authoritative VP5 extension; that does not authorize synthesis.

### Explicit legacy recovery

`remaster_emerald_qol_recover_legacy_item_metadata` is an explicit operation for
callers that have independently established old-remaster Vanilla+ provenance.
It is not called by decode, encode, platform load/store or ordinary QoL access.
Call it before ordinary lazy QoL initialization if recovery is intended.
The legacy magic by itself cannot prove provenance inside a gameplay field.

The result and mutation contract is:

| Result | Meaning / mutation |
| --- | --- |
| INVALID | Null wrapper or invalid runtime source provenance; no mutation |
| NOT_APPLICABLE | Stock source; no mutation or persisted VP5 synthesis |
| CURRENT | Valid production metadata wins; no mutation |
| UNSUPPORTED | Production VP5 magic with version other than 1; no mutation |
| NO_LEGACY | Legacy bytes fail pinned version-1 validity; no mutation |
| CONFLICT | Destination is not uniformly 21 bytes of 00 or FF; no mutation |
| RECOVERED | Copy exactly 21 valid legacy bytes into the blank production footprint |

Recovery does not clear legacy bytes, reset the full unused region, synchronize
`registeredItem`, resize any source block or rewrite unrelated data. Repeating
a successful recovery returns CURRENT. Already-overwritten Mystery Gift bytes
cannot be reconstructed by this operation; they are retained, not repaired.
No automatic stock-to-Vanilla+ or reverse layout conversion exists.

### Version and preference boundary

Unknown VP5 versions are deliberately opaque. This is explicit project
hardening relative to pinned `GetItemMetadata`, which would reset them.
Read APIs return neutral results, setters/registration reject and prune is a
no-op; sorting rejects before changing Bag bytes. Version-1 malformed metadata
and missing magic retain the source's exactly-21-byte lazy reset policy.
Export preserves an unknown version unchanged; I5 owns whole-image/platform
unsupported and corrupt-file reporting, rather than an implicit downgrade here.

For stock saves, a zero-initialized 21-byte `stock_item_metadata` runtime
sidecar provides the existing QoL API without placing a VP5 header in SB1.
Only an intentional primary-item registration/unregistration may change the
authoritative `registeredItem` field; Bag sorting changes actual item ordering
as before. Additional quick slots and sort preferences are session state and
are not serialized in stock Emerald exports. Decode starts a fresh sidecar
and derives the one persisted stock primary on first QoL access. Vanilla+
retains persistent VP5 sort modes and quick slots as accepted in QOL-021/024.

This wrapper extension changes the remaster runtime allocation size only.
SaveBlock2/SaveBlock1/PokemonStorage sizes, chunk spans and 128 KiB image size
are unchanged. Remaster graphics/performance settings stay in Unreal config
(`RemasterPerformanceSettings.h`, `DefaultGameUserSettings.ini`), outside the
gameplay blocks. No new preferences format, sidecar file, UI or platform I/O
semantics is introduced by I4.

### Regression and verification evidence

`r17_save_metadata_boundary` uses independently specified source offsets,
packed metadata and two-slot/checksum fixtures. It checks every SB1 byte
against allowed changes; SB2/storage, the other slot and special sectors are
also compared. Cases cover production precedence and functional round-trip,
source lazy initialization, malformed v1, all 255 unsupported byte versions,
stock runtime state/export boundaries, explicit recovery, idempotence,
occupied/mixed destinations at every footprint byte, invalid legacy fields,
invalid provenance and null inputs. Existing R16 tests now use the measured
production offset rather than asserting the old literal as authority.

- Pre-I4 code, with a test-only missing-recovery shim: **146062 checks,
  2657 failures**, RED, exit 1. The shim
  models the absent API without changing baseline implementation behavior.
- Final code: **146062 checks, 0 failures**, GREEN under strict C99 and C++17,
  plus C/C++ ASan/UBSan, each with the same check totals.
  LeakSanitizer is disabled in the ptrace workspace.
- Existing R16 behavior/item tests and the 26006-check I3 export/transaction
  regression pass. All 43 local core blobs matched the live baseline before
  edits; remaining core blobs must continue to match that baseline.

I4 remains VERIFYING until the atomic committed HEAD's complete relevant PR
CI is terminal-success. Success -> R17-I5; failure -> a concrete I4 closure.
Full R17 acceptance, real fixture acceptance and Unreal/Android execution are
not claimed by this metadata checkpoint.

## 20. R17-I5 corrupt/unsupported/platform checkpoint

### Read and public load results

The production platform installs the new typed reader. Its result is distinct
from the extended save-domain status; existing values EMPTY/OK/DEGRADED/CORRUPT
remain 0/1/2/3, and IO_ERROR/UNSUPPORTED are appended as 4/5.

| Input | Public platform-load result |
| --- | --- |
| Confirmed MISSING | EMPTY, zero initial checkpoint, requested source provenance |
| Open/read/close error, unknown callback result, absent platform/reader, legacy false | IO_ERROR |
| Found file with any size other than 128 KiB | UNSUPPORTED |
| Unspecified/ambiguous or unknown requested format | UNSUPPORTED, before read |
| Valid selected Vanilla+ slot with VP5 magic and version != 1 | UNSUPPORTED |
| Found blank/signatureless file, both slots invalid, checksum/ID/signature/counter failure with no valid backup | CORRUPT |
| Independently valid backup/current slot | OK or DEGRADED under the I1 source-backed policy |
| Null output/scratch or insufficient scratch capacity | CORRUPT, before read |

`remaster_emerald_save_load_platform_format` requires explicit stock or
Vanilla+ selection. The UNSPECIFIED enum represents missing caller provenance;
it never triggers checksum-based format guessing. The established older load
API remains declared Vanilla+, not a generic autodetector. The production
Vanilla+ subsystem now makes that selection explicitly. Failure clears
unusable output state and retains a distinct failure status; it never fabricates
a gameplay domain or authorizes overwrite as an EMPTY game.

Pinned `src/save.c` lines 515-637 classify flash slots with no signature as
EMPTY and select a complete valid backup. The raw validator retains those
semantics and the explicit I1 coherence hardening. The filesystem layer knows
the difference between a missing file and a present unimportable image, so it
does not translate the latter into permission to recreate it.

Version rejection is for the selected production VP5 extension only. Stock
does not interpret VP5-shaped bytes at the same numeric address. Low-level
raw decode/export can still preserve opaque future metadata for explicit
inspection/export as defined in I4; ordinary platform load/store rejects it
instead of silently resetting or downgrading it.

### Safe store and checkpoint consistency

A platform store performs all rejection checks before reaching save_write.

- Only deliberate EMPTY, OK or DEGRADED caller state is writable; raw export
  also rejects CORRUPT, IO_ERROR, UNSUPPORTED and unknown statuses.
- Only typed MISSING plus an EMPTY counter/slot/rotation of zero can prepare
  an initial 0xFF image. No reader, read error or legacy false can do so.
- Found images must have exact geometry, independently valid slot selection,
  supported source metadata and matching caller counter/slot/rotation.
- An EMPTY caller cannot overwrite a found image, and a previously loaded game
  cannot silently recreate a missing one. Pending future-version metadata and
  invalid runtime source provenance are rejected.
- A valid backup can write the ordinary next slot; data is never assembled
  across invalid slots. The backup slot and sectors 28-31 remain byte-exact.
- All rejected/failed writes retain the complete wrapper, including pending
  gameplay edits, stock runtime metadata, status, counter, slot and rotation.
  Only a successful write commits the I3 metadata plan.

Checkpoint comparison catches stale state observed at the read boundary. It
does not promise a cross-process file lock or atomic compare-and-swap against
a concurrent writer after that read. Scratch bytes may contain partial reads
or a prepared retry image; they are temporary, not committed save state.
Store continues to return success/failure without introducing gameplay repair,
new persistent fields or a new preferences format.

### Production native transport and host boundary

`RemasterFileSaveRead.h` is the shared production/native-test transport.
It opens read-only, measures size without a file-sized allocation, copies only
if the file fits the supplied capacity, checks exact read/EOF and close results,
and initializes output size. Oversized found files publish actual size without
copying; the core can reject them as UNSUPPORTED before using scratch bytes.
Open ENOENT is additionally checked against filesystem status and the nearest
existing directory ancestor, because some CRTs conflate non-directory parents
with absence. Permission/storage errors are ERROR; directories are not saves.
The opened descriptor must be a regular file before any seek/size probe. This
rejects directories on filesystems where fopen/fseek can succeed and their
reported size exceeds capacity; pathname checks alone would also introduce a
replacement race. POSIX fstat and Windows _fstat64 check the actual descriptor.

Windows uses wide paths and `_wfopen_s`; other hosts use native UTF-8 paths.
The Unreal adapter resolves its Saved paths through IFileManager's external-app
read/write path conversions so stdio read and atomic rename address the same
physical files as the engine file helper. The I3 temporary-file protocol remains
the commit transport. The SaveSubsystem exposes IoError and Unsupported and
maps an unknown status to Corrupt, never Empty. Presentation still checks
HasUsableSave; no UI or gameplay result moves into the platform layer.

### Regression and verification evidence

`r17_save_safe_platform_io` constructs stock/Vanilla+ sector images with an
independent checksum/layout oracle. It covers typed/legacy precedence, explicit
source load, MISSING/ERROR/unknown results, wrong sizes, absent callbacks,
corruption of each of 14 sections in both slots (checksum/ID/signature/counter),
present blank files, stale checkpoint fields, all 255 unknown VP5 byte versions,
degraded backup save, transactional failures/retry, special-sector preservation,
new creation, invalid arguments/provenance and unusable raw-export statuses.

- Final fixture against pre-I5 code: **4118 checks, 2070 failures**, RED, exit 1.
  Test-only shims model the absent typed callback/explicit-format API.
- Final implementation: **3300 checks, 0 failures**, strict C99/C++17 and
  C/C++ ASan/UBSan. The baseline performs extra callback checks because it
  incorrectly reaches writers in rejected cases; GREEN removes those writes.
- Native `r17_native_file_save_read`: **32 checks, 0 failures**, including
  a Unicode destination, real missing files/directories/non-directory parents,
  exact/short/oversized/zero-length files, capacity canaries and argument errors.
  Permission/storage error classification is injected into the shared opener
  classifier; it is not claimed as a real device permission/storage failure.
  Native ASan/UBSan passes. LeakSanitizer is disabled under ptrace.
- Existing save/platform tests, 284-check I1 validation, 26006-check I3 export
  and 146062-check I4 metadata regressions pass. Existing first-save mocks now
  declare typed MISSING instead of using ambiguous legacy false as absence.

### R17-I5-Closure-1 — Reject directory size probes

Initial I5 commit `825e53f904f4f7211280a341cdd00cc512ca9184` passed Windows
native read 31/31 and atomic write 24/24. R0 Linux CI run `37529156535` passed
59/60 tests but failed the directory-read case: fopen/seek could expose a
directory length larger than capacity, bypassing fread and returning OK.
I5 therefore entered NEEDS_CLOSURE, not VERIFIED_COMPLETE.

This closure requires a regular-file descriptor before measuring/copying and
adds a zero-capacity directory probe regression. Local strict C++17 and native
ASan/UBSan pass 32/32. The existing Linux CI failure is the observed RED;
exact closure-HEAD Linux/Windows CI must be terminal-success before I5 closes.

I5 is VERIFYING until all exact committed-HEAD relevant PR CI is terminal-success,
including the Windows native read/atomic-write job. Success -> R17-T1;
failure -> a concrete I5 closure. T1 representative real/versioned fixture
acceptance and final phase gates remain open. Native stdio tests and source
path/status review are not real UE compilation or Android filesystem/runtime
validation; those remain R18/runtime work.
