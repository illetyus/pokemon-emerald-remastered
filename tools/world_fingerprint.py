#!/usr/bin/env python3
"""Canonical JSON hashing helpers for deterministic R3 world packages."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any


IGNORED_PACKAGE_FILES = {"audit.json"}
VOLATILE_PROVENANCE_KEYS = {
    "generated_at",
    "package_sha256",
    "file_sha256",
}


def canonical_json_bytes(value: Any) -> bytes:
    return (
        json.dumps(
            value,
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    ).encode("utf-8")


def fingerprint_json(value: Any) -> str:
    return hashlib.sha256(canonical_json_bytes(value)).hexdigest()


def fingerprint_document_content(value: Any) -> str:
    if isinstance(value, dict):
        normalized = {
            key: item
            for key, item in value.items()
            if key != "fingerprint_sha256"
        }
    else:
        normalized = value
    return fingerprint_json(normalized)


def _normalized_document(relative: str, value: Any) -> Any:
    if relative == "provenance.json" and isinstance(value, dict):
        return {
            key: item
            for key, item in value.items()
            if key not in VOLATILE_PROVENANCE_KEYS
        }
    return value


def fingerprint_world_package(root: Path) -> dict[str, Any]:
    root = Path(root)
    file_sha256: dict[str, str] = {}

    for path in sorted(root.rglob("*.json")):
        relative = path.relative_to(root).as_posix()
        if relative in IGNORED_PACKAGE_FILES:
            continue
        value = json.loads(path.read_text(encoding="utf-8"))
        normalized = _normalized_document(relative, value)
        file_sha256[relative] = fingerprint_json(normalized)

    package_payload = [
        {"file": relative, "sha256": file_sha256[relative]}
        for relative in sorted(file_sha256)
    ]
    package_sha256 = fingerprint_json(package_payload)

    return {
        "package_sha256": package_sha256,
        "file_sha256": file_sha256,
    }


__all__ = [
    "canonical_json_bytes",
    "fingerprint_document_content",
    "fingerprint_json",
    "fingerprint_world_package",
]
