"""Shared types for Vanilla+ event-script conversion.

R2 keeps parsing/generation data deliberately small and serializable.  These
records contain identities and source provenance only; no GBA pointer is ever
treated as a host address.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


class ScriptConversionError(ValueError):
    """Raised when authoritative script source cannot be classified safely."""


@dataclass(frozen=True)
class CommandSpec:
    name: str
    handler: str
    opcode: int
    classification: str


@dataclass(frozen=True)
class SpecialSpec:
    name: str
    special_id: int


@dataclass(frozen=True)
class SourceCommand:
    name: str
    args: tuple[str, ...]
    path: Path
    line: int
    label: str


@dataclass
class ScriptClosure:
    labels: set[str] = field(default_factory=set)
    source_files: set[Path] = field(default_factory=set)
    commands: dict[str, str] = field(default_factory=dict)
    source_commands: list[SourceCommand] = field(default_factory=list)
