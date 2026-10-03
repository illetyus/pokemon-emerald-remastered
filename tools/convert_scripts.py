#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from script_ir import CommandSpec, ScriptClosure, ScriptConversionError, SpecialSpec


TABLE_RE = re.compile(r"^\s*\.4byte\s+(ScrCmd_[A-Za-z0-9_]+).*@\s*0x([0-9A-Fa-f]+)\s*$")
MACRO_RE = re.compile(r"^\s*\.macro\s+([A-Za-z_][A-Za-z0-9_]*)\b")
LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*):{1,2}\s*$")
SPECIAL_RE = re.compile(r"^\s*def_special\s+([A-Za-z_][A-Za-z0-9_]*)\b")
TOKEN_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")


CORE = {
    "nop", "nop1", "end", "return", "call", "goto", "goto_if", "call_if",
    "setvar", "addvar", "subvar", "copyvar", "setorcopyvar", "compare",
    "compare_var_to_value", "compare_var_to_var", "setflag", "clearflag", "checkflag",
    "getplayerxy", "getpartysize", "checkplayergender", "random", "setweather",
    "resetweather", "setmaplayoutindex", "setflashlevel", "incrementgamestat",
}
WORLD = {
    "warp", "warpsilent", "warpdoor", "warphole", "warpteleport", "setwarp",
    "setdynamicwarp", "setdivewarp", "setholewarp", "applymovement", "applymovementat",
    "waitmovement", "waitmovementat", "removeobject", "removeobjectat", "addobject",
    "addobjectat", "setobjectxy", "showobjectat", "hideobjectat", "faceplayer", "turnobject",
    "setobjectxyperm", "copyobjectxytoperm", "setobjectmovementtype", "setmetatile",
    "setobjectsubpriority", "resetobjectsubpriority", "opendoor", "closedoor", "setdooropen",
    "setdoorclosed", "setrespawn", "setescapewarp", "setstepcallback", "setfollower",
    "destroyfollower", "facefollower", "checkfollower", "isfollowervisible",
    "followerintopokeball", "updatefollowerpokemongraphic",
}
PRESENTATION = {
    "waitstate", "delay", "playse", "waitse", "playfanfare", "waitfanfare", "playbgm",
    "savebgm", "fadedefaultbgm", "fadenewbgm", "fadeoutbgm", "fadeinbgm", "waitmessage",
    "message", "messageinstant", "messageautoscroll", "closemessage", "lockall", "lock",
    "releaseall", "release", "waitbuttonpress", "yesnobox", "multichoice", "multichoicedefault",
    "multichoicegrid", "drawbox", "erasebox", "drawboxtext", "showmonpic", "hidemonpic",
    "fadescreen", "fadescreenspeed", "fadescreenswapbuffers", "waitdooranim", "pokenavcall",
    "showmoneybox", "hidemoneybox", "updatemoneybox", "showcoinsbox", "hidecoinsbox",
    "updatecoinsbox", "playmoncry", "waitmoncry", "dofieldeffect", "waitfieldeffect",
}
DOMAIN = {
    "additem", "removeitem", "checkitemspace", "checkitem", "checkitemtype", "addpcitem",
    "checkpcitem", "adddecoration", "removedecoration", "checkdecor", "checkdecorspace",
    "givemon", "giveegg", "setmonmove", "checkpartymove", "pokemart", "pokemartdecoration",
    "pokemartdecoration2", "addmoney", "removemoney", "checkmoney", "checkcoins", "addcoins",
    "removecoins", "setwildbattle", "dowildbattle", "trainerbattle", "dotrainerbattle",
    "checktrainerflag", "settrainerflag", "cleartrainerflag", "setberrytree",
    "bufferleadmonspeciesname", "bufferspeciesname", "bufferpartymonnick", "bufferitemname",
    "bufferdecorationname", "buffermovename", "buffernumberstring", "bufferstdstring",
    "bufferstring", "bufferboxname", "buffertrainerclassname", "buffertrainername",
    "buffercontestname", "bufferitemnameplural",
}
SPECIAL_ADAPTER = {"special", "specialvar", "callnative", "gotonative"}


def _canonical_handler(handler: str) -> str:
    return handler.removeprefix("ScrCmd_")


def classify_command(name: str) -> str | None:
    if name in CORE or name.startswith(("compare_", "goto_if_", "call_if_")):
        return "CORE"
    if name in WORLD:
        return "WORLD"
    if name in PRESENTATION:
        return "PRESENTATION_YIELD"
    if name in DOMAIN:
        return "DOMAIN_ADAPTER"
    if name in SPECIAL_ADAPTER:
        return "SPECIAL_ADAPTER"
    if name.startswith(("msgbox", "giveitem", "comparehiddenvar")):
        return "PRESENTATION_YIELD" if name.startswith("msgbox") else "DOMAIN_ADAPTER"
    return None


def _event_macro_names(source_root: Path) -> set[str]:
    path = source_root / "asm/macros/event.inc"
    names: set[str] = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        match = MACRO_RE.match(line)
        if match:
            names.add(match.group(1))
    return names


def build_command_inventory(source_root: Path) -> dict[str, CommandSpec]:
    table_path = source_root / "data/script_cmd_table.inc"
    slots: list[tuple[int, str]] = []
    for line in table_path.read_text(encoding="utf-8").splitlines():
        match = TABLE_RE.match(line)
        if not match:
            continue
        opcode = int(match.group(2), 16)
        if opcode <= 0xE7:
            slots.append((opcode, match.group(1)))
    slots.sort()
    if [op for op, _ in slots] != list(range(0xE8)):
        raise ScriptConversionError(f"{table_path}: opcode table must cover 0x00..0xE7 exactly")

    result: dict[str, CommandSpec] = {}
    for opcode, handler in slots:
        name = _canonical_handler(handler)
        classification = classify_command(name) or "DEFERRED"
        key = name if name not in result else f"{name}@0x{opcode:02X}"
        result[key] = CommandSpec(name=name, opcode=opcode, handler=handler, classification=classification)
    return result


def build_special_inventory(source_root: Path) -> dict[str, SpecialSpec]:
    path = source_root / "data/specials.inc"
    result: dict[str, SpecialSpec] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = SPECIAL_RE.match(line)
        if match:
            name = match.group(1)
            result[name] = SpecialSpec(name=name, index=len(result))
    return result


def _candidate_script_files(source_root: Path) -> list[Path]:
    files: list[Path] = []
    for base in (source_root / "data/maps", source_root / "data/scripts"):
        if base.is_dir():
            files.extend(sorted(base.rglob("*.inc")))
    return files


def _index_labels(files: list[Path]) -> tuple[dict[str, Path], dict[Path, set[str]]]:
    label_to_file: dict[str, Path] = {}
    file_labels: dict[Path, set[str]] = {}
    for path in files:
        labels: set[str] = set()
        for line in path.read_text(encoding="utf-8").splitlines():
            match = LABEL_RE.match(line)
            if match:
                label = match.group(1)
                labels.add(label)
                label_to_file.setdefault(label, path)
        file_labels[path] = labels
    return label_to_file, file_labels


def collect_script_dependency_closure(source_root: Path, roots: list[Path]) -> ScriptClosure:
    candidates = _candidate_script_files(source_root)
    label_to_file, file_labels = _index_labels(candidates)
    event_macros = _event_macro_names(source_root)

    root_paths = [Path(p) for p in roots]
    for path in root_paths:
        if not path.is_file():
            raise ScriptConversionError(f"missing script root: {path}")

    queue = list(root_paths)
    seen: set[Path] = set()
    commands: set[str] = set()
    specials: set[str] = set()

    while queue:
        path = queue.pop(0)
        if path in seen:
            continue
        seen.add(path)
        current_label = "<top-level>"
        for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            clean = raw.split("@", 1)[0].strip()
            if not clean:
                continue
            label_match = LABEL_RE.match(clean)
            if label_match:
                current_label = label_match.group(1)
                continue
            if clean.startswith((".", "#")):
                continue
            first = clean.split(None, 1)[0]
            if first in {"map_script", "map_script_2"}:
                classification = "STRUCTURAL"
            elif first in event_macros:
                classification = classify_command(first)
                if classification is None:
                    rel = path.relative_to(source_root).as_posix()
                    raise ScriptConversionError(
                        f"{rel}: line {line_no}: {current_label}: unclassified command {first}"
                    )
            else:
                classification = None

            if classification is not None:
                if classification != "STRUCTURAL":
                    commands.add(first)
                if first in {"special", "specialvar"}:
                    args = clean.split(None, 1)[1] if " " in clean else ""
                    tokens = [t for t in TOKEN_RE.findall(args) if t not in {"VAR_RESULT"}]
                    if tokens:
                        specials.add(tokens[-1])

            for token in TOKEN_RE.findall(clean):
                target = label_to_file.get(token)
                if target is not None and target not in seen and target not in queue:
                    queue.append(target)

    labels: set[str] = set()
    for path in seen:
        labels.update(file_labels.get(path, set()))
    return ScriptClosure(files=sorted(seen), labels=labels, commands=commands, specials=specials)


def _split_args(text: str) -> list[str]:
    return [part.strip() for part in text.split(",") if part.strip()]


def _source_record(source_root: Path, path: Path, line_no: int) -> dict:
    return {"file": path.relative_to(source_root).as_posix(), "line": line_no}


def _lower_script_command(
    source_root: Path,
    path: Path,
    line_no: int,
    command: str,
    args: list[str],
    known_labels: set[str],
) -> list[dict]:
    source = _source_record(source_root, path, line_no)

    def target_record(op: str, target: str, **extra) -> dict:
        if target not in known_labels:
            raise ScriptConversionError(
                f"{source['file']}: line {line_no}: unresolved script target {target}"
            )
        record = {"op": op, "target_script_id": target, "target_pc": 0, "source": source}
        record.update(extra)
        return record

    if command == "call":
        return [target_record("CALL", args[0])]
    if command == "goto":
        return [target_record("GOTO", args[0])]
    if command == "goto_if_eq":
        return [
            {"op": "COMPARE_VAR_VALUE", "a": args[0], "b": args[1], "source": source},
            target_record("GOTO_IF", args[2], condition="EQUAL"),
        ]
    if command == "call_if_lt":
        return [
            {"op": "COMPARE_VAR_VALUE", "a": args[0], "b": args[1], "source": source},
            target_record("CALL_IF", args[2], condition="LESS"),
        ]
    if command == "call_if_unset":
        return [
            {"op": "CHECK_FLAG", "flag": args[0], "source": source},
            target_record("CALL_IF", args[1], condition="UNSET"),
        ]
    if command == "call_if_set":
        return [
            {"op": "CHECK_FLAG", "flag": args[0], "source": source},
            target_record("CALL_IF", args[1], condition="SET"),
        ]
    condition_names = {
        "lt": "LESS", "eq": "EQUAL", "gt": "GREATER",
        "le": "LESS_EQUAL", "ge": "GREATER_EQUAL", "ne": "NOT_EQUAL",
    }
    match = re.fullmatch(r"(goto|call)_if_(lt|eq|gt|le|ge|ne)", command)
    if match:
        op, cond = match.groups()
        return [
            {"op": "COMPARE_VAR_VALUE", "a": args[0], "b": args[1], "source": source},
            target_record(f"{op.upper()}_IF", args[2], condition=condition_names[cond]),
        ]
    if command == "setvar":
        return [{"op": "SET_VAR", "a": args[0], "b": args[1], "source": source}]
    if command == "special":
        return [{"op": "SPECIAL", "special_id": args[0], "source": source}]
    if command == "specialvar":
        return [{"op": "SPECIAL_VAR", "output": args[0], "special_id": args[1], "source": source}]
    if command in {"end", "return"}:
        return [{"op": command.upper(), "source": source}]

    record = {"op": command.upper(), "source": source}
    if args:
        record["args"] = args
    return [record]


def convert_script_closure(source_root: Path, roots: list[Path]) -> dict:
    closure = collect_script_dependency_closure(source_root, roots)
    known_labels = set(closure.labels)
    event_macros = _event_macro_names(source_root)

    sections: list[tuple[Path, str, int, list[tuple[int, str]]]] = []
    for path in closure.files:
        current_label: str | None = None
        current_line = 0
        body: list[tuple[int, str]] = []
        for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            clean = raw.split("@", 1)[0].strip()
            label_match = LABEL_RE.match(clean)
            if label_match:
                if current_label is not None:
                    sections.append((path, current_label, current_line, body))
                current_label = label_match.group(1)
                current_line = line_no
                body = []
                continue
            if current_label is not None:
                body.append((line_no, clean))
        if current_label is not None:
            sections.append((path, current_label, current_line, body))

    table_kind_by_label: dict[str, str] = {}
    map_scripts: list[dict] = []
    for path, label, _, body in sections:
        for line_no, clean in body:
            if not clean:
                continue
            if clean.startswith("map_script "):
                args = _split_args(clean[len("map_script "):])
                if len(args) != 2:
                    raise ScriptConversionError(
                        f"{path.relative_to(source_root)}: line {line_no}: malformed map_script"
                    )
                kind, target = args
                if target not in known_labels:
                    raise ScriptConversionError(
                        f"{path.relative_to(source_root)}: line {line_no}: unresolved script target {target}"
                    )
                if kind.endswith("_TABLE"):
                    table_kind_by_label[target] = kind
                else:
                    map_scripts.append({"kind": kind, "script_id": target})

    scripts: list[dict] = []
    movements: list[dict] = []
    texts: list[dict] = []
    map_script_tables: list[dict] = []

    for path, label, label_line, body in sections:
        meaningful = [(n, x) for n, x in body if x]
        first = meaningful[0][1] if meaningful else ""
        if label in table_kind_by_label:
            entries = []
            for line_no, clean in meaningful:
                if clean.startswith("map_script_2 "):
                    args = _split_args(clean[len("map_script_2 "):])
                    if len(args) != 3:
                        raise ScriptConversionError(
                            f"{path.relative_to(source_root)}: line {line_no}: malformed map_script_2"
                        )
                    var, value, target = args
                    if target not in known_labels:
                        raise ScriptConversionError(
                            f"{path.relative_to(source_root)}: line {line_no}: unresolved script target {target}"
                        )
                    entries.append({"var": var, "value": value, "script_id": target})
            map_script_tables.append({
                "kind": table_kind_by_label[label],
                "table_id": label,
                "entries": entries,
            })
            continue
        if "Movement" in label:
            steps = [clean for _, clean in meaningful if not clean.startswith(".")]
            movements.append({"movement_id": label, "steps": steps})
            continue
        if first.startswith(".string"):
            texts.append({
                "text_id": label,
                "source": _source_record(source_root, path, label_line),
            })
            continue
        if label.endswith("MapScripts"):
            continue

        instructions: list[dict] = []
        for line_no, clean in meaningful:
            if clean.startswith(".") or clean.startswith("map_script"):
                continue
            command = clean.split(None, 1)[0]
            if command not in event_macros:
                continue
            args_text = clean[len(command):].strip()
            args = _split_args(args_text)
            instructions.extend(
                _lower_script_command(
                    source_root, path, line_no, command, args, known_labels
                )
            )
        if instructions:
            scripts.append({
                "script_id": label,
                "instructions": instructions,
                "source": _source_record(source_root, path, label_line),
            })

    specials = build_special_inventory(source_root)
    used_specials = sorted(closure.specials)
    return {
        "schema_version": 1,
        "scripts": scripts,
        "movements": movements,
        "texts": texts,
        "map_scripts": map_scripts,
        "map_script_tables": map_script_tables,
        "specials": [
            {"special_id": name, "index": specials[name].index if name in specials else None}
            for name in used_specials
        ],
    }


def _report(source_root: Path, roots: list[Path]) -> dict:
    inventory = build_command_inventory(source_root)
    specials = build_special_inventory(source_root)
    closure = collect_script_dependency_closure(source_root, roots)
    return {
        "opcode_min": min(spec.opcode for spec in inventory.values()),
        "opcode_max": max(spec.opcode for spec in inventory.values()),
        "opcode_slot_count": len(inventory),
        "files": [p.relative_to(source_root).as_posix() for p in closure.files],
        "labels": sorted(closure.labels),
        "commands": sorted(closure.commands),
        "specials": [
            {"name": name, "index": specials[name].index if name in specials else None}
            for name in sorted(closure.specials)
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--roots", nargs="+", required=True)
    parser.add_argument("--inventory-only", action="store_true")
    args = parser.parse_args()
    root_paths = [args.source_root / "data/maps" / name / "scripts.inc" for name in args.roots]
    report = _report(args.source_root, root_paths)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    out = args.output_dir / "script-compatibility.json"
    out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
