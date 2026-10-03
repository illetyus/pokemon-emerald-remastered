from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class CommandSpec:
    name: str
    opcode: int
    handler: str
    classification: str


@dataclass(frozen=True)
class SpecialSpec:
    name: str
    index: int


@dataclass
class ScriptClosure:
    files: list[Path]
    labels: set[str]
    commands: set[str]
    specials: set[str]


class ScriptConversionError(ValueError):
    pass
