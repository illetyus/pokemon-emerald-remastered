#!/usr/bin/env python3
"""Inventory Vanilla+/pokeemerald event scripts for the portable R2 runtime.

Task 1 intentionally stops at source inventory and dependency closure.  Semantic
IR emission is added in Task 2.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from script_ir import (
    CommandSpec,
    ScriptClosure,
    ScriptConversionError,
    SourceCommand,
    SpecialSpec,
)


CLASSIFICATIONS = {
    "CORE",
    "WORLD",
    "PRESENTATION_YIELD",
    "DOMAIN_ADAPTER",
    "SPECIAL_ADAPTER",
    "DEFERRED",
}


# Explicit R2 semantic categories.  Macros that exist in authoritative
# event.inc but are not required by the first acceptance slice are inventoried
# as DEFERRED rather than silently treated as supported.
CORE_COMMANDS = {
    "nop",
    "nop1",
    "end",
    "return",
    "call",
    "goto",
    "goto_if",
    "call_if",
    "setvar",
    "addvar",
    "subvar",
    "copyvar",
    "setorcopyvar",
    "compare",
    "compare_var_to_value",
    "compare_var_to_var",
    "setflag",
    "clearflag",
    "checkflag",
    "checkplayergender",
    "goto_if_unset",
    "goto_if_set",
    "call_if_unset",
    "call_if_set",
    "goto_if_lt",
    "goto_if_eq",
    "goto_if_gt",
    "goto_if_le",
    "goto_if_ge",
    "goto_if_ne",
    "call_if_lt",
    "call_if_eq",
    "call_if_gt",
    "call_if_le",
    "call_if_ge",
    "call_if_ne",
}

WORLD_COMMANDS = {
    "map_script",
    "map_script_2",
    "warp",
    "warpsilent",
    "warpdoor",
    "warphole",
    "warpteleport",
    "warpwhitefade",
    "setwarp",
    "setdynamicwarp",
    "setdivewarp",
    "setholewarp",
    "setescapewarp",
    "applymovement",
    "applymovementat",
    "waitmovement",
    "waitmovementat",
    "removeobject",
    "removeobjectat",
    "addobject",
    "addobjectat",
    "setobjectxy",
    "setobjectxyperm",
    "copyobjectxytoperm",
    "showobjectat",
    "hideobjectat",
    "faceplayer",
    "turnobject",
    "setobjectmovementtype",
    "setmetatile",
    "setweather",
    "resetweather",
    "doweather",
    "setflashlevel",
    "setmaplayoutindex",
    "setrespawn",
}

PRESENTATION_COMMANDS = {
    "waitstate",
    "delay",
    "playse",
    "waitse",
    "playfanfare",
    "waitfanfare",
    "playbgm",
    "savebgm",
    "fadedefaultbgm",
    "fadenewbgm",
    "fadeoutbgm",
    "fadeinbgm",
    "waitmessage",
    "message",
    "messageinstant",
    "messageautoscroll",
    "closemessage",
    "msgbox",
    "lockall",
    "lock",
    "releaseall",
    "release",
    "waitbuttonpress",
    "yesnobox",
    "multichoice",
    "multichoicedefault",
    "multichoicegrid",
    "showmonpic",
    "hidemonpic",
    "fadescreen",
    "fadescreenspeed",
    "opendoor",
    "closedoor",
    "waitdooranim",
    "setdooropen",
    "setdoorclosed",
    "pokenavcall",
}

DOMAIN_COMMANDS = {
    "getpartysize",
    "additem",
    "removeitem",
    "checkitemspace",
    "checkitem",
    "checkitemtype",
    "addpcitem",
    "checkpcitem",
    "givemon",
    "giveegg",
    "setmonmove",
    "checkpartymove",
    "bufferspeciesname",
    "bufferleadmonspeciesname",
    "bufferpartymonnick",
    "bufferitemname",
    "buffermovename",
    "addmoney",
    "removemoney",
    "checkmoney",
    "giveitem",
    "trainerbattle",
    "dotrainerbattle",
    "setwildbattle",
    "dowildbattle",
}

SPECIAL_COMMANDS = {"special", "specialvar"}


def _classification_for(command: str, macro_names: set[str]) -> str | None:
    if command in CORE_COMMANDS:
        return "CORE"
    if command in WORLD_COMMANDS:
        return "WORLD"
    if command in PRESENTATION_COMMANDS:
        return "PRESENTATION_YIELD"
    if command in DOMAIN_COMMANDS:
        return "DOMAIN_ADAPTER"
    if command in SPECIAL_COMMANDS:
        return "SPECIAL_ADAPTER"

    # It is important that this fallback only applies to a macro that exists in
    # the authoritative event macro file.  A typo or new reachable command is
    # therefore an error, while known-but-not-yet-needed Vanilla commands are
    # tracked explicitly as deferred.
    if command in macro_names:
        return "DEFERRED"
    return None


def _macro_names(source_root: Path) -> set[str]:
    path = source_root / "asm/macros/event.inc"
    if not path.is_file():
        return set()

    result: set[str] = set()
    pattern = re.compile(r"^\s*\.macro\s+([A-Za-z_][A-Za-z0-9_]*)\b")
    for raw in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(raw)
        if match:
            result.add(match.group(1))
    return result


def build_command_inventory(source_root: Path) -> dict[str, CommandSpec]:
    path = source_root / "data/script_cmd_table.inc"
    if not path.is_file():
        raise FileNotFoundError(path)

    macro_names = _macro_names(source_root)
    pattern = re.compile(
        r"^\s*\.4byte\s+(ScrCmd_[A-Za-z0-9_]+)\s*@\s*0x([0-9A-Fa-f]+)\s*$"
    )

    inventory: dict[str, CommandSpec] = {}
    seen_opcodes: set[int] = set()

    for raw in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(raw)
        if not match:
            continue

        handler = match.group(1)
        opcode = int(match.group(2), 16)
        command = handler.removeprefix("ScrCmd_")
        classification = _classification_for(command, macro_names) or "DEFERRED"

        key = command
        if key in inventory:
            key = f"{command}@0x{opcode:02X}"

        inventory[key] = CommandSpec(
            name=command,
            handler=handler,
            opcode=opcode,
            classification=classification,
        )
        seen_opcodes.add(opcode)

    expected = set(range(0xE8))
    if seen_opcodes != expected:
        missing = sorted(expected - seen_opcodes)
        extra = sorted(seen_opcodes - expected)
        raise ScriptConversionError(
            f"{path}: opcode table must cover 0x00..0xE7; "
            f"missing={missing}, extra={extra}"
        )

    return inventory


def build_special_inventory(source_root: Path) -> dict[str, SpecialSpec]:
    path = source_root / "data/specials.inc"
    if not path.is_file():
        raise FileNotFoundError(path)

    result: dict[str, SpecialSpec] = {}
    pattern = re.compile(r"^\s*def_special\s+([A-Za-z_][A-Za-z0-9_]*)\b")

    for raw in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(raw)
        if not match:
            continue
        name = match.group(1)
        if name in result:
            raise ScriptConversionError(f"{path}: duplicate special {name}")
        result[name] = SpecialSpec(name=name, special_id=len(result))

    return result


def _strip_comment(raw: str) -> str:
    return raw.split("@", 1)[0].strip()


def _parse_args(text: str) -> tuple[str, ...]:
    if not text.strip():
        return ()
    return tuple(part.strip() for part in text.split(",") if part.strip())


def _index_script_sources(
    source_root: Path,
    roots: list[Path],
) -> tuple[
    dict[str, tuple[Path, int, list[SourceCommand]]],
    dict[Path, list[str]],
]:
    candidates: set[Path] = set()

    for path in (source_root / "data/maps").glob("**/scripts.inc"):
        if path.is_file():
            candidates.add(path)
    for path in (source_root / "data/scripts").glob("**/*.inc"):
        if path.is_file():
            candidates.add(path)
    for relative in roots:
        path = source_root / relative
        if path.is_file():
            candidates.add(path)

    labels: dict[str, tuple[Path, int, list[SourceCommand]]] = {}
    labels_by_file: dict[Path, list[str]] = {}

    label_pattern = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)(?:::|:)$")
    command_pattern = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\b(.*)$")

    for absolute in sorted(candidates):
        relative = absolute.relative_to(source_root)
        current_label: str | None = None

        for line_no, raw in enumerate(
            absolute.read_text(encoding="utf-8").splitlines(),
            start=1,
        ):
            line = _strip_comment(raw)
            if not line:
                continue

            label_match = label_pattern.match(line)
            if label_match:
                current_label = label_match.group(1)
                if current_label in labels:
                    previous = labels[current_label][0]
                    raise ScriptConversionError(
                        f"{relative}:{line_no}: duplicate label {current_label}; "
                        f"first defined in {previous}"
                    )
                labels[current_label] = (relative, line_no, [])
                labels_by_file.setdefault(relative, []).append(current_label)
                continue

            if current_label is None or line.startswith("."):
                continue

            command_match = command_pattern.match(line)
            if not command_match:
                continue

            command = command_match.group(1)
            args = _parse_args(command_match.group(2).strip())
            labels[current_label][2].append(
                SourceCommand(
                    name=command,
                    args=args,
                    path=relative,
                    line=line_no,
                    label=current_label,
                )
            )

    return labels, labels_by_file


def _script_targets(command: SourceCommand) -> tuple[str, ...]:
    name = command.name
    args = command.args

    if name in {"call", "goto"} and args:
        return (args[0],)

    if name in {"goto_if", "call_if", "map_script", "map_script_2"} and args:
        return (args[-1],)

    if name.startswith("goto_if_") or name.startswith("call_if_"):
        if args:
            return (args[-1],)

    return ()


def _default_entry_labels(
    roots: list[Path],
    labels_by_file: dict[Path, list[str]],
) -> list[str]:
    result: list[str] = []

    for root in roots:
        for label in labels_by_file.get(root, []):
            # Movement programs share scripts.inc files but are a distinct IR
            # domain handled in Task 2.  Text blocks contain only assembler
            # data.  Event/map script labels remain entry points for inventory
            # so object interactions are not missed.
            if "_Movement_" in label or "_Text_" in label:
                continue
            result.append(label)

    return result


def collect_script_dependency_closure(
    source_root: Path,
    roots: list[Path],
    entry_labels: list[str] | None = None,
) -> ScriptClosure:
    source_root = Path(source_root)
    roots = [Path(path) for path in roots]

    labels, labels_by_file = _index_script_sources(source_root, roots)
    macro_names = _macro_names(source_root)
    opcode_commands = {
        spec.name for spec in build_command_inventory(source_root).values()
    }

    entries = (
        list(entry_labels)
        if entry_labels is not None
        else _default_entry_labels(roots, labels_by_file)
    )

    closure = ScriptClosure()
    pending = list(entries)
    visited: set[str] = set()

    while pending:
        label = pending.pop()
        if label in visited:
            continue
        if label not in labels:
            raise ScriptConversionError(f"unresolved script label: {label}")

        visited.add(label)
        path, _label_line, commands = labels[label]
        closure.labels.add(label)
        closure.source_files.add(path)

        for command in commands:
            classification = _classification_for(command.name, macro_names)
            if classification is None and command.name in opcode_commands:
                classification = "DEFERRED"

            if classification is None:
                raise ScriptConversionError(
                    f"{command.path}:{command.line}: {command.label}: "
                    f"unclassified reachable command {command.name}"
                )

            if classification not in CLASSIFICATIONS:
                raise AssertionError(classification)

            closure.commands[command.name] = classification
            closure.source_commands.append(command)

            for target in _script_targets(command):
                # Numeric/std-function operands are not script labels.
                if re.fullmatch(r"(?:0x[0-9A-Fa-f]+|\d+)", target):
                    continue
                if target in labels and target not in visited:
                    pending.append(target)

    return closure


def _resolve_root_paths(source_root: Path, root_names: list[str]) -> list[Path]:
    result: list[Path] = []
    for name in root_names:
        raw = Path(name)
        direct = source_root / raw
        if direct.is_file():
            result.append(raw)
            continue

        map_script = Path("data/maps") / name / "scripts.inc"
        if (source_root / map_script).is_file():
            result.append(map_script)
            continue

        raise ScriptConversionError(f"unable to resolve script root {name}")
    return result


def _report(
    source_root: Path,
    roots: list[Path],
) -> dict:
    command_inventory = build_command_inventory(source_root)
    specials = build_special_inventory(source_root)
    closure = collect_script_dependency_closure(source_root, roots)

    return {
        "schema_version": 1,
        "opcode_count": len({spec.opcode for spec in command_inventory.values()}),
        "special_count": len(specials),
        "roots": [str(path).replace("\\", "/") for path in roots],
        "reachable_labels": sorted(closure.labels),
        "source_files": sorted(
            str(path).replace("\\", "/") for path in closure.source_files
        ),
        "reachable_commands": {
            name: closure.commands[name] for name in sorted(closure.commands)
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--roots", nargs="+", required=True)
    parser.add_argument("--inventory-only", action="store_true")
    args = parser.parse_args()

    roots = _resolve_root_paths(args.source_root, args.roots)
    report = _report(args.source_root, roots)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    output = args.output_dir / "script_compatibility.json"
    output.write_text(
        json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
