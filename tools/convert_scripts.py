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
            if first in event_macros:
                classification = classify_command(first)
                if classification is None:
                    rel = path.relative_to(source_root).as_posix()
                    raise ScriptConversionError(
                        f"{rel}: line {line_no}: {current_label}: unclassified command {first}"
                    )
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
