# R17-T1 — Compatibility fixture matrix

Date: 2026-10-06
State: VERIFIED_COMPLETE at `f0db6e28a06ebd104f9e184c475a0ec359ca31f6`; all nine PR runs terminal-success.
Implementation baseline: `ac50b51674d5707caa0a2ac1d237d687a0af6d9e`.
Contract: [R17_SAVE_COMPAT_CONTRACT.md](R17_SAVE_COMPAT_CONTRACT.md).

## Versioned artifacts

`tests/fixtures/r17/matrix.json` pins source provenance, named recipes, input
SHA-256 values, expected objectives, seven existing fixture-source Git blob
hashes/CTest owners and one private real-save evidence fixture.
`tools/r17_save_fixture_matrix.py` constructs source-layout bytes independently
of the production encoder and compares native results against its byte oracle.
`tests/r17_save_fixture_probe.cpp` calls the actual portable platform load/store,
world/party/bag/RTC/objective APIs and the shared native file reader. No gameplay,
save implementation, vendor or Unreal adapter changes belong to this checkpoint.

The probe opens input read-only and uses a test-only memory write adapter.
The driver checks the full persisted image and all logical payloads, writes the
export to its own temporary directory, then reopens it through a new native
process. This is native file/disk-reopen evidence; it does not claim Unreal's
real filesystem/Android runtime behavior or new ROM/emulator acceptance.

## Public recipe matrix

The CTest entry `r17_save_fixture_matrix` runs **37 named cases**:

| Recipes | Count | Coverage |
| --- | ---: | --- |
| Six progression profiles, each stock/Vanilla+ | 12 | Pre-starter, rival, Birch, Petalburg, Roxanne, game-clear; objectives 0/1/2/3/4/0; party 0/1/3/6; zero/nonzero encryption keys; five bag pockets |
| Nine error/transaction profiles, each stock/Vanilla+ | 18 | Damaged-newest backup, mixed counters, missing section, both corrupt, bad ID/signature, present erased image, short file, failed write |
| MAX/zero counter boundary, each stock/Vanilla+ | 2 | Source wrap exception, sector rotation 13 to 0, ordinary next counter/slot |
| Production/future/legacy VP5; stock VP5-shaped bytes | 4 | Supported version, future-version platform rejection, no automatic legacy relocation, stock bytes opaque |
| Redacted user VP019 semantic projection | 1 | Actual world/RTC/progression/party/bag semantics with objective 15; artificial names/identity/encryption/opaque/storage data |

Each recipe's generated input must match its pinned SHA-256. Accepted saves are
checked against independent ID/checksum/signature/counter selection and whole
source-block reconstruction. The entire export must match an independent
source-format sector writer, including padding/footer, target slot, rotation,
counter, untouched main-slot bytes and special sectors 28–31. World, all party
records, all five bag pockets, RTC, objective and all three payloads must match
after reimport. Failed writes must retain the wrapper and complete disk image.

The new recipes supplement the versioned exhaustive I1–I5/native fixtures:

| Requirement | Existing CTest fixture |
| --- | --- |
| Geometry/checksum spans, ambiguity, all rotations, slot/counter ordering | `r17_save_sector_validation` |
| Flags/vars, full party/storage/bag/NPC/RTC/encounter domain import | `r17_save_domain_import` |
| Whole-image export, sequential writes, source format and transactionality | `r17_save_export_transactionality` |
| Explicit legacy migration, conflicts/idempotence, VP5 neighbors/stock sidecar | `r17_save_metadata_boundary` |
| Every corrupted section, unsupported versions, MISSING/ERROR, stale checkpoint | `r17_save_safe_platform_io` |
| Native atomic write/close/replace failures | `r17_atomic_file_save` |
| Native size/error/directory/Unicode path reads | `r17_native_file_save_read` |

The runner checks those CTest names and exact fixture-source blobs. They execute
in the normal full CTest suite; the runner does not pretend to execute or replace
them. No extra CI implementation transport or workflow self-commit is added.

## Private real-save fixture

The user supplied `pokemon-emerald-vp019-r1-roundtrip.srm` on 2026-10-06.
Its declared provenance is VP019; shared signatures alone do not establish a
ROM version. The private 131072-byte input is pinned by:

`4cf20c4310a49fe4a7e0782f89deaa8e4efa45b2f46a9f443941d506fcd2a633`

Both main slots independently validate. Slot 0 has counter 72, rotation 2;
slot 1 has counter 71, rotation 1. The selected source checkpoint is 72/0/2.
The ordinary next export is 73/1/3, with output SHA-256:

`db9bf9a76ace9158aed5ad68469de6c3e08dd7f848c67dc3ea6b6f4d1b05b001`

The actual native state is player (19,20), map (0,1), party count 6, money
62560, coins 33, registered item 259, current box 1 and derived objective 15.
All six party checksums validate. The real fixture's full logical payloads,
encrypted bag interpretation, progression/RTC and objective survive export and
disk reopen. The whole-image byte oracle verifies the unchanged other main
slot and special sectors. Input bytes remain unchanged.

The manifest stores payload/special-sector/bag hashes and a name-free semantic
summary. It contains no raw real-save payload, trainer/Pokémon names, OT identity,
personalities, private path, ROM or extracted commercial assets. Its public
semantic projection is reconstructed with artificial values for redacted fields;
it is explicitly a derived synthetic recipe, not the original real image.
Public CI runs that projection and the other recipes; it does not claim access
to the private original. The original is replayed locally with a hash check.

Only this supplied real Vanilla+ sample was replayed in T1. Stock coverage is
source-backed synthetic coverage; no real stock sample or new emulator/game
reopen was provided/executed here. The historical R1 emulator evidence remains
historical and is not substituted for the new native run.

## Reproduce

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure -R r17_save_fixture_matrix
python3 tools/r17_save_fixture_matrix.py --probe build/r17_save_fixture_probe \
    --real-save /private/path/pokemon-emerald-vp019-r1-roundtrip.srm
```

The private option accepts the manifest's exact real-save hash. Keep that input
outside the repository; the driver creates/removes its own temporary exports.
It never overwrites the input or uploads private files. No private path is
needed for the normal CI test, and no private replay is silently marked passed
when the option is omitted.

## Actual local verification

- Strict C99 core + strict C++17 probe: 37/37 recipes pass; private real import,
  store/export, disk reopen, domain preservation and byte oracle pass.
- C/C++ ASan/UBSan: the same 37 recipes and private real replay pass with
  `UBSAN_OPTIONS=halt_on_error=1`; leak detection disabled under ptrace.
- Seven deliberately altered probe results (world, party, bag, RTC, objective,
  logical payload and special-sector byte) are all rejected by the independent
  oracle. These are test-only negative inputs, not production mutations.
- No implementation fix was needed to make these T1 cases pass. Exact-HEAD CI
  passed 61/61, including the new CTest entry; Windows read 32/32 and atomic
  write 24/24 passed. All nine workflow runs are terminal-success.

T1, V1 and G1 are VERIFIED_COMPLETE at the same acceptance HEAD. Completion
and exact CI evidence are in [R17_SAVE_COMPAT_COMPLETION.md](R17_SAVE_COMPAT_COMPLETION.md).
D1 verification, the independent final gate and merge/main verification remain
separate checkpoints.
