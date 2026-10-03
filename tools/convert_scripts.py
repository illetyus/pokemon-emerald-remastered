#!/usr/bin/env python3
"""Inventory Vanilla+/pokeemerald event scripts for the portable R2 runtime.

Task 1 intentionally stops at source inventory and dependency closure.  Semantic
IR emission is added in Task 2.
"""

from __future__ import annotations

import argparse
import ast
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
    "followerintopokeball",
    "updatefollowerpokemongraphic",
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
    for path in (source_root / "data/text").glob("**/*.inc"):
        if path.is_file():
            candidates.add(path)
    event_scripts = source_root / "data/event_scripts.s"
    if event_scripts.is_file():
        candidates.add(event_scripts)
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



_CONDITION_SUFFIX = {
    "lt": "LESS",
    "eq": "EQUAL",
    "gt": "GREATER",
    "le": "LESS_EQUAL",
    "ge": "GREATER_EQUAL",
    "ne": "NOT_EQUAL",
}


def _script_source_sections(
    source_root: Path,
    roots: list[Path],
) -> dict[str, tuple[Path, int, list[tuple[int, str]]]]:
    candidates: set[Path] = set()
    for path in (source_root / "data/maps").glob("**/scripts.inc"):
        if path.is_file():
            candidates.add(path)
    for path in (source_root / "data/scripts").glob("**/*.inc"):
        if path.is_file():
            candidates.add(path)
    for path in (source_root / "data/text").glob("**/*.inc"):
        if path.is_file():
            candidates.add(path)
    event_scripts = source_root / "data/event_scripts.s"
    if event_scripts.is_file():
        candidates.add(event_scripts)
    for relative in roots:
        path = source_root / relative
        if path.is_file():
            candidates.add(path)

    sections: dict[str, tuple[Path, int, list[tuple[int, str]]]] = {}
    label_pattern = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)(?:::|:)$")

    current: str | None
    for absolute in sorted(candidates):
        relative = absolute.relative_to(source_root)
        current = None
        for line_no, raw in enumerate(
            absolute.read_text(encoding="utf-8").splitlines(),
            start=1,
        ):
            stripped = raw.strip()
            label_match = label_pattern.match(stripped)
            if label_match:
                current = label_match.group(1)
                if current in sections:
                    previous = sections[current][0]
                    raise ScriptConversionError(
                        f"{relative}:{line_no}: duplicate label {current}; "
                        f"first defined in {previous}"
                    )
                sections[current] = (relative, line_no, [])
                continue
            if current is not None:
                sections[current][2].append((line_no, raw))
    return sections


def _symbolic_target_or_error(
    target: str,
    labels: set[str],
    command: SourceCommand,
) -> str:
    if target not in labels:
        raise ScriptConversionError(
            f"{command.path}:{command.line}: {command.label}: "
            f"unresolved script label {target}"
        )
    return target


def _compare_instruction(lhs: str, rhs: str) -> dict:
    if rhs.startswith("VAR_"):
        return {"op": "COMPARE_VAR_VAR", "lhs_var": lhs, "rhs_var": rhs}
    return {"op": "COMPARE_VAR_VALUE", "var": lhs, "value": rhs}


def _normalize_script_command(
    command: SourceCommand,
    labels: set[str],
    specials: dict[str, SpecialSpec],
) -> list[dict]:
    name = command.name
    args = command.args

    if name in {"map_script", "map_script_2"}:
        return []

    if name == "call" and len(args) == 1:
        return [{
            "op": "CALL",
            "target_script_id": _symbolic_target_or_error(
                args[0], labels, command
            ),
        }]

    if name == "goto" and len(args) == 1:
        return [{
            "op": "GOTO",
            "target_script_id": _symbolic_target_or_error(
                args[0], labels, command
            ),
        }]

    if name in {"goto_if", "call_if"} and len(args) == 2:
        return [{
            "op": "GOTO_IF" if name == "goto_if" else "CALL_IF",
            "condition": args[0],
            "target_script_id": _symbolic_target_or_error(
                args[1], labels, command
            ),
        }]

    if name in {
        "goto_if_unset",
        "goto_if_set",
        "call_if_unset",
        "call_if_set",
    } and len(args) == 2:
        is_call = name.startswith("call_")
        is_set = name.endswith("_set")
        return [
            {"op": "CHECK_FLAG", "flag": args[0]},
            {
                "op": "CALL_IF" if is_call else "GOTO_IF",
                "condition": "TRUE" if is_set else "FALSE",
                "target_script_id": _symbolic_target_or_error(
                    args[1], labels, command
                ),
            },
        ]

    conditional = re.fullmatch(r"(goto|call)_if_(lt|eq|gt|le|ge|ne)", name)
    if conditional:
        branch_op = "GOTO_IF" if conditional.group(1) == "goto" else "CALL_IF"
        condition = _CONDITION_SUFFIX[conditional.group(2)]
        if len(args) == 3:
            return [
                _compare_instruction(args[0], args[1]),
                {
                    "op": branch_op,
                    "condition": condition,
                    "target_script_id": _symbolic_target_or_error(
                        args[2], labels, command
                    ),
                },
            ]
        if len(args) == 1:
            return [{
                "op": branch_op,
                "condition": condition,
                "target_script_id": _symbolic_target_or_error(
                    args[0], labels, command
                ),
            }]

    if name == "compare" and len(args) == 2:
        return [_compare_instruction(args[0], args[1])]

    if name == "compare_var_to_value" and len(args) == 2:
        return [{"op": "COMPARE_VAR_VALUE", "var": args[0], "value": args[1]}]

    if name == "compare_var_to_var" and len(args) == 2:
        return [{
            "op": "COMPARE_VAR_VAR",
            "lhs_var": args[0],
            "rhs_var": args[1],
        }]

    if name == "special" and len(args) == 1:
        special_name = args[0]
        if special_name not in specials:
            raise ScriptConversionError(
                f"{command.path}:{command.line}: {command.label}: "
                f"unknown special {special_name}"
            )
        return [{
            "op": "SPECIAL",
            "special_id": special_name,
            "special_index": specials[special_name].special_id,
        }]

    if name == "specialvar" and len(args) == 2:
        special_name = args[1]
        if special_name not in specials:
            raise ScriptConversionError(
                f"{command.path}:{command.line}: {command.label}: "
                f"unknown special {special_name}"
            )
        return [{
            "op": "SPECIAL_VAR",
            "output_var": args[0],
            "special_id": special_name,
            "special_index": specials[special_name].special_id,
        }]

    field_shapes: dict[str, tuple[str, ...]] = {
        "setvar": ("var", "value"),
        "setflag": ("flag",),
        "clearflag": ("flag",),
        "checkflag": ("flag",),
        "applymovement": ("local_id", "movement_id"),
        "waitmovement": ("local_id",),
        "msgbox": ("text_id", "mode"),
    }
    if name in field_shapes:
        fields = field_shapes[name]
        item: dict[str, object] = {"op": name.upper()}
        for index, field in enumerate(fields):
            if index < len(args):
                item[field] = args[index]
        return [item]

    # Task 2 preserves known commands that do not yet have a typed execution
    # shape as semantic command names plus symbolic operands.  Later R2 tasks
    # replace these generic payloads as the VM/host contracts are introduced.
    return [{"op": name.upper(), "args": list(args)}]


def _extract_text_strings(lines: list[tuple[int, str]]) -> list[str]:
    result: list[str] = []
    pattern = re.compile(r'^\\s*\\.string\\s+(.+?)\\s*$')
    for _line_no, raw in lines:
        match = pattern.match(raw)
        if not match:
            continue
        token = match.group(1)
        try:
            value = ast.literal_eval(token)
        except (SyntaxError, ValueError):
            value = token.strip().strip('"')
        result.append(str(value))
    return result


def convert_script_closure(
    source_root: Path,
    roots: list[Path],
    entry_labels: list[str] | None = None,
) -> dict:
    source_root = Path(source_root)
    roots = [Path(path) for path in roots]
    closure = collect_script_dependency_closure(
        source_root,
        roots,
        entry_labels=entry_labels,
    )
    sections = _script_source_sections(source_root, roots)
    labels = set(sections)
    specials = build_special_inventory(source_root)

    # Unlike the Task 1 inventory, semantic conversion requires every branch
    # target to resolve.  This prevents partial IR when a dependency file was
    # omitted or a label changed upstream.
    for command in closure.source_commands:
        for target in _script_targets(command):
            if re.fullmatch(r"(?:0x[0-9A-Fa-f]+|\d+)", target):
                continue
            _symbolic_target_or_error(target, labels, command)

    source_by_label: dict[str, list[SourceCommand]] = {}
    for command in closure.source_commands:
        source_by_label.setdefault(command.label, []).append(command)

    scripts: list[dict] = []
    map_scripts: list[dict] = []
    map_script_tables: list[dict] = []
    movement_ids: set[str] = set()
    text_ids: set[str] = set()

    for label in sorted(closure.labels):
        commands = source_by_label.get(label, [])
        if commands and all(
            command.name in {"map_script", "map_script_2"}
            for command in commands
        ):
            for command in commands:
                if command.name == "map_script" and len(command.args) >= 2:
                    map_scripts.append({
                        "owner_id": label,
                        "kind": command.args[0],
                        "script_id": _symbolic_target_or_error(
                            command.args[-1], labels, command
                        ),
                    })
                elif command.name == "map_script_2" and len(command.args) >= 3:
                    map_script_tables.append({
                        "owner_id": label,
                        "var": command.args[0],
                        "value": command.args[1],
                        "script_id": _symbolic_target_or_error(
                            command.args[-1], labels, command
                        ),
                    })
            continue

        instructions: list[dict] = []
        for command in commands:
            normalized = _normalize_script_command(command, labels, specials)
            instructions.extend(normalized)

            if command.name in {"applymovement", "applymovementat"}:
                if len(command.args) >= 2:
                    movement_ids.add(command.args[1])
            if command.name in {
                "msgbox",
                "message",
                "messageinstant",
                "messageautoscroll",
                "pokenavcall",
            } and command.args:
                text_ids.add(command.args[0])

        if instructions:
            source_path, source_line, _ = sections[label]
            scripts.append({
                "script_id": label,
                "source": {
                    "file": str(source_path).replace("\\", "/"),
                    "line": source_line,
                },
                "instructions": instructions,
            })

    movements: list[dict] = []
    for movement_id in sorted(movement_ids):
        if movement_id not in sections:
            raise ScriptConversionError(
                f"unresolved movement label {movement_id}"
            )
        source_path, source_line, lines = sections[movement_id]
        steps: list[dict] = []
        for _line_no, raw in lines:
            line = _strip_comment(raw)
            if not line or line.startswith("."):
                continue
            match = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\b(.*)$", line)
            if not match:
                continue
            steps.append({
                "op": match.group(1),
                "args": list(_parse_args(match.group(2).strip())),
            })
        movements.append({
            "movement_id": movement_id,
            "source": {
                "file": str(source_path).replace("\\", "/"),
                "line": source_line,
            },
            "steps": steps,
        })

    texts: list[dict] = []
    for text_id in sorted(text_ids):
        if text_id not in sections:
            raise ScriptConversionError(f"unresolved text label {text_id}")
        source_path, source_line, lines = sections[text_id]
        texts.append({
            "text_id": text_id,
            "source": {
                "file": str(source_path).replace("\\", "/"),
                "line": source_line,
            },
            "strings": _extract_text_strings(lines),
        })

    special_manifest = [
        {
            "special_id": spec.name,
            "special_index": spec.special_id,
        }
        for spec in sorted(
            specials.values(),
            key=lambda spec: spec.special_id,
        )
    ]

    return {
        "schema_version": 1,
        "specials": special_manifest,
        "scripts": sorted(scripts, key=lambda item: item["script_id"]),
        "movements": sorted(
            movements, key=lambda item: item["movement_id"]
        ),
        "texts": sorted(texts, key=lambda item: item["text_id"]),
        "map_scripts": sorted(
            map_scripts,
            key=lambda item: (
                item["owner_id"], item["kind"], item["script_id"]
            ),
        ),
        "map_script_tables": sorted(
            map_script_tables,
            key=lambda item: (
                item["owner_id"], item["var"], item["value"], item["script_id"]
            ),
        ),
    }


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
