#!/usr/bin/env python3
"""Versioned save recipes and private real-save replay; no ROM/assets required.

The byte oracle uses pinned source geometry, not the production encoder or its
constants. Synthetic images live only in a temporary directory. Private input
and payload bytes are never included in the public evidence report.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "tests/fixtures/r17/matrix.json"
SIZE = 131072
POCKETS = ((0x560, 30), (0x650, 16), (0x690, 64), (0x790, 46), (0x5D8, 30))
# Pinned src/pokemon.c GetSubstruct: canonical type -> physical substruct.
ORDER = ((0,1,2,3),(0,1,3,2),(0,2,1,3),(0,3,1,2),(0,2,3,1),(0,3,2,1),
         (1,0,2,3),(1,0,3,2),(2,0,1,3),(3,0,1,2),(2,0,3,1),(3,0,2,1),
         (1,2,0,3),(1,3,0,2),(2,1,0,3),(3,1,0,2),(2,3,0,1),(3,2,0,1),
         (1,2,3,0),(1,3,2,0),(2,1,3,0),(3,1,2,0),(2,3,1,0),(3,2,1,0))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def checksum(data):
    total = sum(struct.unpack("<" + "I" * (len(data) // 4), data)) & 0xFFFFFFFF
    return ((total >> 16) + total) & 0xFFFF


def chunks(blocks):
    return [blocks[0]] + [blocks[1][i:i + 3968] for i in range(0, len(blocks[1]), 3968)] \
        + [blocks[2][i:i + 3968] for i in range(0, len(blocks[2]), 3968)]


def write_slot(image, slot, counter, rotation, blocks):
    for sid, data in enumerate(chunks(blocks)):
        sector = bytearray(4096)
        sector[:len(data)] = data
        struct.pack_into("<HHII", sector, 4084, sid, checksum(data), 0x08012025, counter)
        at = (slot * 14 + (sid + rotation) % 14) * 4096
        image[at:at + 4096] = sector


def block_sizes(fmt):
    return (0xF2C, 0x3D88, 0x83D0) if fmt == "stock" else (0xF44, 0x3DC8, 0x83D0)


def read_slots(image, fmt):
    sizes = block_sizes(fmt)
    spans = [sizes[0], 3968, 3968, 3968, sizes[1] - 3 * 3968] + [3968] * 8 + [0x7D0]
    slots = []
    for slot in range(2):
        sections, counters, rotation = {}, set(), None
        for physical in range(14):
            at = (slot * 14 + physical) * 4096
            sector = image[at:at + 4096]
            sid, check, sig, counter = struct.unpack_from("<HHII", sector, 4084)
            if sid >= 14 or sig != 0x08012025 or sid in sections \
                    or check != checksum(sector[:spans[sid]]):
                break
            sections[sid] = sector[:spans[sid]]
            counters.add(counter)
            if sid == 0:
                rotation = physical
        if len(sections) != 14 or len(counters) != 1:
            slots.append(None)
        else:
            blocks = [sections[0], b"".join(sections[i] for i in range(1, 5)),
                      b"".join(sections[i] for i in range(5, 14))]
            slots.append(dict(slot=slot, counter=counters.pop(), rotation=rotation, blocks=blocks))
    return slots


def select(image, fmt):
    if len(image) != SIZE:
        return 5, None
    slots = read_slots(image, fmt)
    if not any(slots):
        return 3, None
    if all(slots):
        a, b = slots
        # Exact source wrap exception; this is not a signed-distance ordering.
        if {a["counter"], b["counter"]} == {0, 0xFFFFFFFF}:
            selected = a if a["counter"] == 0 else b
        else:
            selected = b if b["counter"] > a["counter"] else a
        status = 1
    else:
        selected, status = next(s for s in slots if s), 2
    meta = selected["blocks"][1][0x35D8:0x35D8 + 21]
    if fmt == "vanillaplus" and u32(meta, 0) == 0x35504956 and meta[4] != 1:
        return 5, None
    return status, selected


def box_packet(species=21, held=95, experience=1000):
    # Artificial encrypted record; header/identity bytes are synthetic.
    sub = [bytearray(12) for _ in range(4)]
    struct.pack_into("<HHIBB", sub[0], 0, species, held, experience, 0, 70)
    struct.pack_into("<HHHHBBBB", sub[1], 0, 33, 85, 57, 0, 35, 15, 25, 0)
    sub[2][:6] = bytes(range(1, 7))
    raw = bytearray(80)
    struct.pack_into("<II", raw, 0, 17, 0xA1B2C3D4)
    raw[18:20] = bytes((2, 2))
    plain = bytearray(48)
    for typ, physical in enumerate(ORDER[17]):
        plain[physical * 12:(physical + 1) * 12] = sub[typ]
    struct.pack_into("<H", raw, 28, sum(struct.unpack("<24H", plain)) & 0xFFFF)
    for off in range(0, 48, 4):
        struct.pack_into("<I", raw, 32 + off, u32(plain, off) ^ 17 ^ 0xA1B2C3D4)
    return raw


def recipe(case, profiles):
    fmt, mode = case["format"], case.get("mode", "normal")
    profile = profiles[case["profile"]]
    sb2, sb1, storage = (bytearray(n) for n in block_sizes(fmt))
    key = profile["key"]
    struct.pack_into("<hhbbb", sb1, 0, -7, 22, 0, 16, -1)
    struct.pack_into("<hh", sb1, 8, 12, 15)
    struct.pack_into("<HBBBxH", sb1, 0x2C, 0x1234, 3, 2, 1, 441)
    count = profile["party_count"]
    sb1[0x234:0x238] = bytes((count, 0xA1, 0xB2, 0xC3))
    for i in range(count):
        at = 0x238 + i * 100
        mon = profile.get("party", [[1, 21, 95, 1000, 42, 73 + j, 100, 8] for j in range(count)])[i]
        sb1[at:at + 80] = box_packet(mon[1], mon[2], mon[3])
        struct.pack_into("<IBBHHHHHHH", sb1, at + 80, mon[7], mon[4], 255,
                         mon[5], mon[6], 81, 65, 91, 102, 77)
    struct.pack_into("<I", sb2, 0xAC, key)
    struct.pack_into("<IHH", sb1, 0x490, 543210 ^ key, 4321 ^ (key & 0xFFFF), 259)
    for pocket, (at, capacity) in enumerate(POCKETS):
        for i in range(capacity):
            populated = profile["bag"] and i in (0, capacity - 1)
            struct.pack_into("<HH", sb1, at + i * 4, (13, 4, 289, 133, 259)[pocket] if populated else 0,
                             ((i + 1) if populated else 0) ^ (key & 0xFFFF))
            if "bag_slots" in profile:
                item, quantity = profile["bag_slots"][pocket][i]
                struct.pack_into("<HH", sb1, at + i * 4, item, quantity ^ (key & 0xFFFF))
    flags, vars_at = (0x1270, 0x139C) if fmt == "stock" else (0x12B0, 0x13DC)
    for flag in profile["flags"]:
        sb1[flags + flag // 8] |= 1 << (flag % 8)
    for var, value in profile["vars"].items():
        struct.pack_into("<H", sb1, vars_at + (int(var) - 0x4000) * 2, value)
    sb1[-16:] = bytes([0xAA] * 16)
    sb1[0x3540:0x3590] = bytes((i * 19 + 7) & 255 for i in range(0x50))
    if fmt == "vanillaplus":
        sb2[0xF2C:] = bytes([0x5A] * 24)
    struct.pack_into("<hbbb", sb2, 0x98, -12, 2, 3, 4)
    struct.pack_into("<hbbb", sb2, 0xA0, 1234, 5, 6, 7)
    if "world" in profile:
        w = profile["world"]
        struct.pack_into("<hhbbb", sb1, 0, *w[:5])
        struct.pack_into("<hh", sb1, 8, *w[5:7])
        struct.pack_into("<HBBBxH", sb1, 0x2C, w[8], *w[9:12], w[7])
        struct.pack_into("<IHH", sb1, 0x490, w[13] ^ key, w[14] ^ (key & 65535), w[15])
        struct.pack_into("<hbbb", sb2, 0x98, *profile["rtc"][:4])
        struct.pack_into("<hbbb", sb2, 0xA0, *profile["rtc"][4:])
    storage[0:4] = bytes((13, 0x33, 0x44, 0x55))
    for at in (4, 4 + 419 * 80):
        storage[at:at + 80] = box_packet()
    storage[0x8344:] = bytes((i * 13 + 5) & 255 for i in range(0x83D0 - 0x8344))
    if mode in ("vp5", "future-vp5", "legacy-vp5", "stock-vp5-bytes"):
        at = 0x3598 if mode == "legacy-vp5" else 0x35D8
        sb1[at:at + 21] = struct.pack("<IB", 0x35504956, 2 if mode == "future-vp5" else 1) + bytes(16)
    image = bytearray([0xFF] * SIZE)
    counter = case.get("counter", 72)
    write_slot(image, 0, counter, case.get("rotation", 2), (sb2, sb1, storage))
    write_slot(image, 1, (counter - 1) & 0xFFFFFFFF, 1, (sb2, sb1, storage))
    image[28 * 4096:] = bytes((i * 17 + 31) & 255 for i in range(4 * 4096))
    for slot in ((0, 1) if mode in ("corrupt-both", "bad-id", "bad-signature") else (0,)):
        at = (slot * 14 + (case.get("rotation", 2) if slot == 0 else 1)) * 4096
        if mode in ("corrupt-both", "checksum-backup"):
            image[at + 4086] ^= 1
        elif mode == "mixed-counter":
            struct.pack_into("<I", image, at + 4092, 14)
        elif mode == "bad-id":
            struct.pack_into("<H", image, at + 4084, 15)
        elif mode in ("bad-signature", "missing-section"):
            struct.pack_into("<I", image, at + 4088, 0)
    if mode == "blank":
        image[:] = bytes([0xFF] * SIZE)
    return bytes(image[:-1] if mode == "short" else image)


def domain_view(blocks):
    sb2, sb1, storage = blocks
    key = u32(sb2, 0xAC)
    world = list(struct.unpack_from("<hhbbb", sb1, 0)) + list(struct.unpack_from("<hh", sb1, 8))
    world += [u16(sb1, 0x32), u16(sb1, 0x2C), *sb1[0x2E:0x31], sb1[0x234],
              u32(sb1, 0x490) ^ key, u16(sb1, 0x494) ^ (key & 65535), u16(sb1, 0x496)]
    party = []
    for i in range(min(sb1[0x234], 6)):
        raw = sb1[0x238 + i * 100:0x238 + (i + 1) * 100]
        personality, ot = struct.unpack_from("<II", raw)
        plain = b"".join(struct.pack("<I", u32(raw, p) ^ personality ^ ot) for p in range(32, 80, 4))
        growth = plain[ORDER[personality % 24][0] * 12:][:12]
        valid = int((sum(struct.unpack("<24H", plain)) & 65535) == u16(raw, 28))
        party.append([valid, u16(growth, 0), u16(growth, 2), u32(growth, 4), raw[84],
                      u16(raw, 86), u16(raw, 88), u32(raw, 80)])
    bag = []
    for at, capacity in POCKETS:
        bag.append([[u16(sb1, at + i * 4),
                     (u16(sb1, at + i * 4 + 2) ^ (key & 65535)) if u16(sb1, at + i * 4) else 0]
                    for i in range(capacity)])
    return dict(world=world, party=party, bag=bag, current_box=storage[0],
                rtc=[*struct.unpack_from("<hbbb", sb2, 0x98), *struct.unpack_from("<hbbb", sb2, 0xA0)])


def probe(executable, fmt, path, fail=False):
    result = subprocess.run([str(executable), fmt, str(path)] + (["fail-write"] if fail else []),
                            capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise AssertionError(f"native probe failed ({result.returncode}): {result.stderr[:1000]}")
    return json.loads(result.stdout)


def require(ok, name):
    if not ok:
        raise AssertionError(name)


def verify(data, fmt, actual, objective=None, fail=False):
    status, chosen = select(data, fmt)
    require(actual["status"] == status, "platform load status")
    if chosen is None:
        require(actual["writes"] == 0, "rejected image reached writer")
        return dict(status=status, input_sha256=sha(data), writes=0)
    before = actual["before"]
    require([bytes.fromhex(b) for b in before["blocks"]] == chosen["blocks"], "source logical block reconstruction")
    for field, expected in domain_view(chosen["blocks"]).items():
        require(before[field] == expected, "independent domain view: " + field)
    for field in ("counter", "slot", "rotation"):
        require(before[field] == chosen[field], "source checkpoint: " + field)
    require(before["stock"] == int(fmt == "stock"), "format provenance")
    if objective is not None:
        require(before["objective"] == objective, "derived objective")
    require(actual["writes"] == 1, "exactly one write attempt")
    if fail:
        require(not actual["store_ok"] and actual["wrapper_unchanged"], "failed write advanced wrapper")
        require(bytes.fromhex(actual["image"]) == data, "failed write changed persistent image")
        return dict(status=status, input_sha256=sha(data), writes=1, failed_write_preserved=True)
    require(actual["store_ok"], "valid source could not store")
    expected = bytearray(data)
    counter, rotation = (chosen["counter"] + 1) & 0xFFFFFFFF, (chosen["rotation"] + 1) % 14
    write_slot(expected, counter % 2, counter, rotation, chosen["blocks"])
    encoded = bytes.fromhex(actual["image"])
    require(encoded == expected, "independent byte-exact export: payload/footer/padding/untouched sectors")
    after = actual["after"]
    require(after["counter"] == counter and after["slot"] == counter % 2 and after["rotation"] == rotation,
            "export checkpoint")
    require(after["status"] == 1, "exported slots not valid")
    for field in ("blocks", "stock", "objective", "world", "party", "bag", "rtc", "current_box"):
        require(after[field] == before[field], "round-trip domain drift: " + field)
    # Public evidence excludes raw blocks, trainer identities and input paths.
    return dict(status=status, input_sha256=sha(data), output_sha256=sha(encoded),
                source_checkpoint={k: before[k] for k in ("counter", "slot", "rotation")},
                output_checkpoint={k: after[k] for k in ("counter", "slot", "rotation")},
                domain_sha256=[sha(b) for b in chosen["blocks"]],
                special_sectors_sha256=sha(data[28 * 4096:]), objective=before["objective"],
                world=before["world"], rtc=before["rtc"], party=before["party"],
                bag_sha256=sha(json.dumps(before["bag"], separators=(",", ":")).encode()),
                current_box=before["current_box"], writes=1,
                exact_export=True, domains_preserved=True, untouched_sectors_preserved=True)


def run_matrix(executable, matrix):
    require(matrix["schema_version"] == 1, "matrix schema")
    require(matrix["source"]["vanillaplus"]["commit"] == "70db90c9077aed1272e746fc2537d9f12b95a91c",
            "Vanilla+ source pin")
    require(matrix["source"]["stock"]["commit"] == "731ad5bfd6e6f265508d0efcca0ba42f9dcf5881",
            "stock source pin")
    cmake = (ROOT / "CMakeLists.txt").read_text()
    for entry in matrix["coverage"]:
        require("NAME " + entry["ctest"] + " " in cmake, "missing CTest coverage: " + entry["requirement"])
        # Git text blobs are LF even when a Windows checkout uses CRLF.
        raw = (ROOT / entry["fixture_source"]).read_bytes().replace(b"\r\n", b"\n")
        require(hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
                == entry["source_blob_sha"], "fixture source drift: " + entry["requirement"])
    ids = [c["id"] for c in matrix["synthetic"]]
    require(len(ids) == len(set(ids)), "duplicate fixture IDs")
    with tempfile.TemporaryDirectory(prefix="r17-fixtures-") as tmp:
        for case in matrix["synthetic"]:
            data = recipe(case, matrix["profiles"])
            require(sha(data) == case["sha256"], "fixture drift: " + case["id"])
            status, chosen = select(data, case["format"])
            require(status == case["expected"]["load_status"], "pinned status policy: " + case["id"])
            if chosen:
                require({k: chosen[k] for k in ("counter", "slot", "rotation")}
                        == case["expected"]["checkpoint"], "pinned selection: " + case["id"])
            path = Path(tmp) / (case["id"] + ".sav")
            path.write_bytes(data)
            actual = probe(executable, case["format"], path, case.get("mode") == "fail-write")
            profile = matrix["profiles"][case["profile"]]
            verify(data, case["format"], actual, profile["objective"], case.get("mode") == "fail-write")
            if "output_sha256" in case["expected"]:
                require(sha(bytes.fromhex(actual["image"])) == case["expected"]["output_sha256"],
                        "pinned export: " + case["id"])
    print(f"R17 versioned fixture matrix: {len(ids)} cases, 0 failures")


def run_real(executable, path, fixture):
    data = path.read_bytes()
    require(sha(data) == fixture["sha256"], "private real fixture hash does not match provenance")
    actual = probe(executable, fixture["format"], path)
    evidence = verify(data, fixture["format"], actual, fixture["expected"]["objective"])
    require(evidence == fixture["expected"], "private real replay differs from pinned evidence")
    with tempfile.TemporaryDirectory(prefix="r17-real-replay-") as tmp:
        output = Path(tmp) / "roundtrip.sav"
        output.write_bytes(bytes.fromhex(actual["image"]))
        require(sha(output.read_bytes()) == evidence["output_sha256"], "disk export differs")
        reopened = probe(executable, fixture["format"], output)
        verify(output.read_bytes(), fixture["format"], reopened, evidence["objective"])
        for field in ("blocks", "world", "party", "bag", "objective", "rtc", "current_box"):
            require(reopened["before"][field] == actual["before"][field], "persisted replay drift: " + field)
    require(path.read_bytes() == data, "private input was changed")
    print("R17 private real replay: import/export/disk-reopen/domain/preservation PASS")
    return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--matrix", type=Path, default=MATRIX)
    parser.add_argument("--real-save", type=Path, help="optional private user-owned VP019 fixture; never uploaded")
    args = parser.parse_args()
    matrix = json.loads(args.matrix.read_text())
    run_matrix(args.probe.resolve(), matrix)
    if args.real_save is not None:
        run_real(args.probe.resolve(), args.real_save, matrix["real"][0])


if __name__ == "__main__":
    main()
