#!/usr/bin/env python3
"""R3 full-Hoenn acceptance gate against the vendored Vanilla+ source tree."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from audit_generated_content import audit  # noqa: E402
from convert_world import convert_world  # noqa: E402


PINNED_SOURCE_REPOSITORY = "illetyus/pokezumrut-vanillaplus"
PINNED_SOURCE_COMMIT = "70db90c9077aed1272e746fc2537d9f12b95a91c"
PINNED_SOURCE_TREE = "58b83886d5f99915741a0929bd2d67a95a78740a"

EXPECTED_COUNTS = {
    "group_count": 34,
    "map_count": 518,
    "layout_count": 441,
    "map_script_file_count": 468,
    "encounter_group_count": 3,
    "map_encounter_count": 124,
}


class R3AcceptanceError(RuntimeError):
    """Raised when the R3 full-world acceptance contract is violated."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise R3AcceptanceError(message)


def _load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def _reset_dir(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def _verify_vendor_tree(repo_root: Path, source_root: Path) -> str:
    repo_root = repo_root.resolve()
    source_root = source_root.resolve()
    try:
        relative = source_root.relative_to(repo_root).as_posix()
    except ValueError as exc:
        raise R3AcceptanceError(
            f"source root must live inside repository: {source_root}"
        ) from exc

    actual = subprocess.check_output(
        ["git", "-C", str(repo_root), "rev-parse", f"HEAD:{relative}"],
        text=True,
    ).strip()

    _require(
        actual == PINNED_SOURCE_TREE,
        "vendored Vanilla+ tree mismatch: "
        f"expected {PINNED_SOURCE_TREE}, got {actual}",
    )
    return actual


def _file_hashes(root: Path) -> dict[str, str]:
    hashes: dict[str, str] = {}
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        relative = path.relative_to(root).as_posix()
        hashes[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
    return hashes


def _tree_digest(file_hashes: dict[str, str]) -> str:
    digest = hashlib.sha256()
    for relative, file_sha in sorted(file_hashes.items()):
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_sha.encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def _validate_counts(
    manifest: dict[str, Any],
    provenance: dict[str, Any],
    scripts: dict[str, Any],
    encounters: dict[str, Any],
) -> dict[str, int]:
    source_counts = provenance.get("source_counts", {})
    actual = {
        "group_count": source_counts.get("group_count"),
        "map_count": manifest.get("map_count"),
        "layout_count": manifest.get("layout_count"),
        "map_script_file_count": scripts.get("map_script_file_count"),
        "encounter_group_count": encounters.get("group_count"),
        "map_encounter_count": encounters.get("map_encounter_count"),
    }

    for key, expected in EXPECTED_COUNTS.items():
        _require(
            actual.get(key) == expected,
            f"{key}: expected {expected}, got {actual.get(key)!r}",
        )

    _require(
        source_counts.get("map_count") == EXPECTED_COUNTS["map_count"],
        "provenance source map_count mismatch",
    )
    _require(
        source_counts.get("layout_count") == EXPECTED_COUNTS["layout_count"],
        "provenance source layout_count mismatch",
    )
    _require(
        source_counts.get("map_script_file_count")
        == EXPECTED_COUNTS["map_script_file_count"],
        "provenance source map_script_file_count mismatch",
    )
    _require(
        scripts.get("map_count") == EXPECTED_COUNTS["map_count"],
        "script catalog map_count mismatch",
    )
    _require(
        manifest.get("encounter_group_count")
        == EXPECTED_COUNTS["encounter_group_count"],
        "manifest encounter_group_count mismatch",
    )
    _require(
        manifest.get("map_encounter_count")
        == EXPECTED_COUNTS["map_encounter_count"],
        "manifest map_encounter_count mismatch",
    )

    return actual


def run_probe(
    repo_root: Path,
    source_root: Path,
    work_root: Path,
    evidence_root: Path,
) -> dict[str, Any]:
    vendor_tree = _verify_vendor_tree(repo_root, source_root)

    output_a = work_root / "world-a"
    output_b = work_root / "world-b"
    _reset_dir(output_a)
    _reset_dir(output_b)
    _reset_dir(evidence_root)

    manifest_a = convert_world(
        source_root.resolve(),
        output_a,
        source_commit=PINNED_SOURCE_COMMIT,
        source_repository=PINNED_SOURCE_REPOSITORY,
    )
    manifest_b = convert_world(
        source_root.resolve(),
        output_b,
        source_commit=PINNED_SOURCE_COMMIT,
        source_repository=PINNED_SOURCE_REPOSITORY,
    )

    errors_a = audit(output_a)
    errors_b = audit(output_b)
    _write_json(evidence_root / "audit-a.json", {"errors": errors_a})
    _write_json(evidence_root / "audit-b.json", {"errors": errors_b})
    _require(not errors_a, f"world-a audit failed with {len(errors_a)} errors")
    _require(not errors_b, f"world-b audit failed with {len(errors_b)} errors")

    provenance_a = _load_json(output_a / "provenance.json")
    provenance_b = _load_json(output_b / "provenance.json")
    scripts_a = _load_json(output_a / "scripts" / "manifest.json")
    encounters_a = _load_json(output_a / "encounters.json")

    _require(
        provenance_a.get("source_repository") == PINNED_SOURCE_REPOSITORY,
        "provenance source_repository mismatch",
    )
    _require(
        provenance_a.get("source_commit") == PINNED_SOURCE_COMMIT,
        "provenance source_commit mismatch",
    )

    actual_counts = _validate_counts(
        manifest_a,
        provenance_a,
        scripts_a,
        encounters_a,
    )

    hashes_a = _file_hashes(output_a)
    hashes_b = _file_hashes(output_b)
    _require(
        hashes_a == hashes_b,
        "two full-world conversions are not byte-identical",
    )
    _require(
        provenance_a.get("package_sha256") == provenance_b.get("package_sha256"),
        "package fingerprints differ between repeated conversions",
    )
    _require(
        provenance_a.get("file_sha256") == provenance_b.get("file_sha256"),
        "per-file package fingerprints differ between repeated conversions",
    )

    summary = {
        "status": "PASS",
        "source_repository": PINNED_SOURCE_REPOSITORY,
        "source_commit": PINNED_SOURCE_COMMIT,
        "source_tree": vendor_tree,
        "counts": actual_counts,
        "audit_error_count": {
            "world_a": len(errors_a),
            "world_b": len(errors_b),
        },
        "deterministic": True,
        "generated_file_count": len(hashes_a),
        "raw_output_tree_sha256": _tree_digest(hashes_a),
        "package_sha256": provenance_a.get("package_sha256"),
    }
    _write_json(evidence_root / "summary.json", summary)

    for relative in (
        "manifest.json",
        "provenance.json",
        "layouts.json",
        "scripts/manifest.json",
        "encounters.json",
    ):
        src = output_a / relative
        dst = evidence_root / "world-a" / relative
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)

    return summary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path("vendor/vanillaplus"),
    )
    parser.add_argument(
        "--work-root",
        type=Path,
        default=Path("/tmp/r3-full-world"),
    )
    parser.add_argument(
        "--evidence-root",
        type=Path,
        default=Path("/tmp/r3-full-world-evidence"),
    )
    args = parser.parse_args()

    try:
        summary = run_probe(
            args.repo_root,
            args.source_root,
            args.work_root,
            args.evidence_root,
        )
    except Exception as exc:
        args.evidence_root.mkdir(parents=True, exist_ok=True)
        _write_json(
            args.evidence_root / "failure.json",
            {
                "status": "FAIL",
                "error_type": type(exc).__name__,
                "message": str(exc),
            },
        )
        raise

    print(json.dumps(summary, ensure_ascii=False, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
