#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter
from pathlib import Path

PATTERNS = {
    "hardware_register": re.compile(r"\bREG_[A-Z0-9_]+\b"),
    "vram": re.compile(r"\bVRAM\b|\bBG_VRAM\b|\bOBJ_VRAM\b"),
    "palette": re.compile(r"\bPLTT\b|\bPLTT_SIZE\b"),
    "oam": re.compile(r"\bOAM\b|\bOamData\b"),
    "dma": re.compile(r"\bDMA[0-3]?\b|\bDma(?:Copy|Fill|Stop)"),
    "interrupt": re.compile(r"\bINTR_CHECK\b|\bIntrMain\b|\bSetIntrFunc\b"),
    "gba_bios": re.compile(r"\b(?:CpuSet|CpuFastSet|SoftReset|RegisterRamReset)\b"),
    "m4a_audio": re.compile(r"\bm4a[A-Za-z0-9_]*\b|\bMPlay[A-Za-z0-9_]*\b"),
}

def scan(root: Path) -> dict:
    files = []
    totals = Counter()
    for base in ("src", "include", "gflib"):
        folder = root / base
        if not folder.exists():
            continue
        for path in sorted(folder.rglob("*")):
            if path.suffix not in {".c", ".h", ".inc"} or not path.is_file():
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            hits = {}
            for name, pattern in PATTERNS.items():
                count = len(pattern.findall(text))
                if count:
                    hits[name] = count
                    totals[name] += count
            if hits:
                files.append({
                    "path": str(path.relative_to(root)).replace("\\", "/"),
                    "hits": hits,
                    "total": sum(hits.values()),
                })
    files.sort(key=lambda item: (-item["total"], item["path"]))
    return {
        "schema_version": 1,
        "files_with_platform_coupling": len(files),
        "totals": dict(sorted(totals.items())),
        "files": files,
    }

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("vanillaplus_root", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = scan(args.vanillaplus_root.resolve())
    encoded = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
