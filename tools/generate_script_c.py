#!/usr/bin/env python3
"""Generate portable C fixtures from R2 semantic script IR."""

from __future__ import annotations

import ast
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


class ScriptCGenerationError(ValueError):
    """Raised when semantic IR cannot be lowered safely to native C."""


_ALLOWED_BINOPS = {
    ast.Add: lambda a, b: a + b,
    ast.Sub: lambda a, b: a - b,
    ast.BitOr: lambda a, b: a | b,
    ast.BitAnd: lambda a, b: a & b,
    ast.BitXor: lambda a, b: a ^ b,
    ast.LShift: lambda a, b: a << b,
    ast.RShift: lambda a, b: a >> b,
    ast.Mult: lambda a, b: a * b,
    ast.FloorDiv: lambda a, b: a // b,
}
_ALLOWED_UNARY = {
    ast.UAdd: lambda a: a,
    ast.USub: lambda a: -a,
    ast.Invert: lambda a: ~a,
}


@dataclass
class NumericResolver:
    source_root: Path
    globals: dict[str, str]
    locals: dict[Path, dict[str, str]]

    def resolve(self, token: Any, source_path: Path | str | None = None) -> int:
        if isinstance(token, bool):
            return int(token)
        if isinstance(token, int):
            return token
        if token is None:
            raise ScriptCGenerationError("missing numeric operand")

        text = str(token).strip()
        if not text:
            raise ScriptCGenerationError("empty numeric operand")

        text = re.sub(r"(?<=\b0[xX][0-9A-Fa-f]+)[uUlL]+\b", "", text)
        text = re.sub(r"(?<=\b\d)[uUlL]+\b", "", text)

        try:
            return int(text, 0)
        except ValueError:
            pass

        local_map: dict[str, str] = {}
        if source_path is not None:
            rel = Path(source_path)
            local_map = self.locals.get(rel, {})

        return self._eval_expr(text, local_map, set())

    def _eval_expr(
        self,
        expression: str,
        local_map: dict[str, str],
        stack: set[str],
    ) -> int:
        try:
            node = ast.parse(expression, mode="eval").body
        except SyntaxError as exc:
            raise ScriptCGenerationError(
                f"unsupported numeric expression: {expression}"
            ) from exc
        return self._eval_node(node, local_map, stack)

    def _eval_node(
        self,
        node: ast.AST,
        local_map: dict[str, str],
        stack: set[str],
    ) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return int(node.value)

        if isinstance(node, ast.Name):
            name = node.id
            if name in stack:
                raise ScriptCGenerationError(f"recursive numeric symbol: {name}")
            if name in local_map:
                expr = local_map[name]
            elif name in self.globals:
                expr = self.globals[name]
            else:
                raise ScriptCGenerationError(f"unresolved numeric symbol: {name}")
            return self._eval_expr(expr, local_map, stack | {name})

        if isinstance(node, ast.BinOp) and type(node.op) in _ALLOWED_BINOPS:
            lhs = self._eval_node(node.left, local_map, stack)
            rhs = self._eval_node(node.right, local_map, stack)
            return _ALLOWED_BINOPS[type(node.op)](lhs, rhs)

        if isinstance(node, ast.UnaryOp) and type(node.op) in _ALLOWED_UNARY:
            value = self._eval_node(node.operand, local_map, stack)
            return _ALLOWED_UNARY[type(node.op)](value)

        raise ScriptCGenerationError(
            f"unsupported numeric expression node: {ast.dump(node)}"
        )


def _strip_c_comment(value: str) -> str:
    value = value.split("//", 1)[0]
    value = value.split("/*", 1)[0]
    return value.strip()


def build_numeric_resolver(source_root: Path) -> NumericResolver:
    source_root = Path(source_root)
    globals_: dict[str, str] = {}
    locals_: dict[Path, dict[str, str]] = {}

    define_pattern = re.compile(
        r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.+?)\s*$"
    )
    set_pattern = re.compile(
        r"^\s*\.set\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*(.+?)\s*$"
    )

    include = source_root / "include"
    if include.is_dir():
        for path in sorted(include.rglob("*.h")):
            for raw in path.read_text(encoding="utf-8").splitlines():
                match = define_pattern.match(raw)
                if not match:
                    continue
                name, expr = match.groups()
                expr = _strip_c_comment(expr)
                if expr and "(" not in name:
                    globals_.setdefault(name, expr)

    for path in sorted(source_root.rglob("*.inc")):
        rel = path.relative_to(source_root)
        table: dict[str, str] = {}
        for raw in path.read_text(encoding="utf-8").splitlines():
            match = set_pattern.match(raw)
            if not match:
                continue
            name, expr = match.groups()
            table[name] = _strip_c_comment(expr)
        if table:
            locals_[rel] = table

    return NumericResolver(source_root, globals_, locals_)


def _cident(value: str) -> str:
    result = re.sub(r"[^A-Za-z0-9_]", "_", value)
    if not result or result[0].isdigit():
        result = "_" + result
    return result


def _cstr(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def _u(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:04X}u"


def _resolve(
    resolver: NumericResolver,
    value: Any,
    source: Path | None,
    *,
    field: str,
    script_id: str,
) -> int:
    try:
        return resolver.resolve(value, source)
    except ScriptCGenerationError as exc:
        raise ScriptCGenerationError(
            f"{script_id}: cannot resolve {field}={value!r}: {exc}"
        ) from exc


_CONDITIONS = {
    "LESS": "REMASTER_EMERALD_CONDITION_LESS",
    "EQUAL": "REMASTER_EMERALD_CONDITION_EQUAL",
    "GREATER": "REMASTER_EMERALD_CONDITION_GREATER",
    "LESS_EQUAL": "REMASTER_EMERALD_CONDITION_LESS_EQUAL",
    "GREATER_EQUAL": "REMASTER_EMERALD_CONDITION_GREATER_EQUAL",
    "NOT_EQUAL": "REMASTER_EMERALD_CONDITION_NOT_EQUAL",
    "FALSE": "REMASTER_EMERALD_CONDITION_EQUAL",
    "TRUE": "REMASTER_EMERALD_CONDITION_NOT_EQUAL",
}

_OPCODE_NAMES = {
    name: f"REMASTER_EMERALD_SCRIPT_{name}"
    for name in [
        "NOP", "END", "RETURN", "GOTO", "CALL", "GOTO_IF", "CALL_IF",
        "SET_VAR", "COPY_VAR", "SET_OR_COPY_VAR", "COMPARE_VAR_VALUE",
        "COMPARE_VAR_VAR", "ADD_VAR", "SUB_VAR", "SET_FLAG", "CLEAR_FLAG",
        "CHECK_FLAG", "SET_WEATHER", "SET_MAP_LAYOUT", "WAIT_STATE",
        "ADD_OBJECT", "REMOVE_OBJECT", "SHOW_OBJECT", "HIDE_OBJECT",
        "SET_OBJECT_XY", "SET_OBJECT_XY_PERM", "SET_OBJECT_MOVEMENT_TYPE",
        "TURN_OBJECT", "FACE_PLAYER", "APPLY_MOVEMENT", "WAIT_MOVEMENT",
        "MESSAGE", "CLOSE_MESSAGE", "WAIT_MESSAGE", "CHOICE", "DELAY",
        "PLAY_SOUND", "WAIT_SOUND", "PLAY_FANFARE", "WAIT_FANFARE",
        "PLAY_BGM", "FADE", "OPEN_DOOR", "CLOSE_DOOR", "WAIT_DOOR", "WARP",
        "ADD_MONEY", "REMOVE_MONEY", "CHECK_MONEY", "DOMAIN_ITEM_ADD",
        "DOMAIN_ITEM_REMOVE", "DOMAIN_ITEM_CHECK", "DOMAIN_ITEM_SPACE",
        "DOMAIN_GIVE_MON", "DOMAIN_HEAL_PARTY", "DOMAIN_PARTY_SIZE",
        "SPECIAL", "SPECIAL_VAR", "CHECK_PLAYER_GENDER", "LOCK", "LOCK_ALL",
        "RELEASE", "RELEASE_ALL", "SET_METATILE", "SAVE_BGM",
        "FADE_DEFAULT_BGM", "INCREMENT_GAME_STAT",
        "BUFFER_LEAD_MON_SPECIES_NAME", "SET_FOLLOWER",
        "FOLLOWER_INTO_POKEBALL", "UPDATE_FOLLOWER_POKEMON_GRAPHIC",
    ]
}


def _instruction_fields(
    ins: dict[str, Any],
    resolver: NumericResolver,
    source: Path | None,
    script_id: str,
    program_indices: dict[str, int],
) -> list[str]:
    op = str(ins.get("op", ""))
    if op not in _OPCODE_NAMES:
        raise ScriptCGenerationError(f"{script_id}: unknown IR opcode {op}")

    fields = [f".opcode = {_OPCODE_NAMES[op]}"]

    def number(key: str) -> int:
        return _resolve(
            resolver, ins.get(key), source, field=key, script_id=script_id
        )

    if op in {"SET_VAR", "ADD_VAR", "SUB_VAR"}:
        fields += [f".a = {_u(number('var'))}", f".b = {_u(number('value'))}"]
    elif op == "COPY_VAR":
        fields += [
            f".a = {_u(number('dest_var'))}",
            f".b = {_u(number('source_var'))}",
        ]
    elif op == "SET_OR_COPY_VAR":
        fields += [
            f".a = {_u(number('var'))}",
            f".b = {_u(number('value'))}",
        ]
    elif op == "COMPARE_VAR_VALUE":
        fields += [f".a = {_u(number('var'))}", f".b = {_u(number('value'))}"]
    elif op == "COMPARE_VAR_VAR":
        fields += [
            f".a = {_u(number('lhs_var'))}",
            f".b = {_u(number('rhs_var'))}",
        ]
    elif op in {"SET_FLAG", "CLEAR_FLAG", "CHECK_FLAG"}:
        fields += [f".a = {_u(number('flag'))}"]
    elif op == "CHECK_PLAYER_GENDER":
        fields += [f".a = {_u(number('result_var'))}"]
    elif op in {"CALL", "GOTO", "CALL_IF", "GOTO_IF"}:
        target = str(ins.get("target_script_id", ""))
        if target not in program_indices:
            raise ScriptCGenerationError(
                f"{script_id}: unresolved target script {target}"
            )
        if op in {"CALL_IF", "GOTO_IF"}:
            cond = str(ins.get("condition", ""))
            if cond not in _CONDITIONS:
                raise ScriptCGenerationError(
                    f"{script_id}: unknown condition {cond}"
                )
            fields.append(f".condition = {_CONDITIONS[cond]}")
        fields += [
            ".target = 0u",
            f".target_program = {program_indices[target]}u",
            ".target_program_valid = 1u",
        ]
    elif op in {"ADD_OBJECT", "REMOVE_OBJECT", "SHOW_OBJECT", "HIDE_OBJECT"}:
        fields.append(f".a = {_u(number('local_id'))}")
        if "map_id" in ins:
            fields.append(f".map_id = {_u(number('map_id'))}")
    elif op in {"SET_OBJECT_XY", "SET_OBJECT_XY_PERM"}:
        fields += [
            f".a = {_u(number('local_id'))}",
            f".x = {number('x')}",
            f".y = {number('y')}",
        ]
    elif op == "SET_OBJECT_MOVEMENT_TYPE":
        fields += [
            f".a = {_u(number('local_id'))}",
            f".b = {_u(number('movement_type'))}",
        ]
    elif op == "TURN_OBJECT":
        fields += [
            f".a = {_u(number('local_id'))}",
            f".b = {_u(number('direction'))}",
        ]
    elif op in {"APPLY_MOVEMENT", "WAIT_MOVEMENT"}:
        fields.append(f".a = {_u(number('local_id'))}")
        if "map_id" in ins:
            fields.append(f".map_id = {_u(number('map_id'))}")
        if "movement_id" in ins:
            fields.append(f".resource_id = {_cstr(str(ins['movement_id']))}")
    elif op == "MESSAGE":
        if "text_id" in ins:
            fields.append(f".resource_id = {_cstr(str(ins['text_id']))}")
        if "mode" in ins:
            fields.append(f".b = {_u(number('mode'))}")
    elif op in {"DELAY"}:
        key = "frames"
        fields.append(f".a = {_u(number(key))}")
    elif op in {"PLAY_SOUND"}:
        fields.append(f".a = {_u(number('sound'))}")
    elif op in {"PLAY_FANFARE", "SAVE_BGM"}:
        fields.append(f".a = {_u(number('song'))}")
    elif op == "PLAY_BGM":
        fields += [f".a = {_u(number('song'))}", f".b = {_u(number('save'))}"]
    elif op == "FADE":
        fields.append(f".a = {_u(number('mode'))}")
    elif op in {"OPEN_DOOR", "CLOSE_DOOR"}:
        fields += [
            f".a = {_u(number('x_operand'))}",
            f".b = {_u(number('y_operand'))}",
        ]
    elif op == "WARP":
        fields.append(f".map_id = {_u(number('map_id'))}")
        if "x_operand" in ins:
            fields.append(f".a = {_u(number('x_operand'))}")
        if "y_operand" in ins:
            fields.append(f".b = {_u(number('y_operand'))}")
        if "warp_id" in ins:
            fields.append(f".target = {number('warp_id') & 0xFFFFFFFF}u")
    elif op in {"ADD_MONEY", "REMOVE_MONEY", "CHECK_MONEY"}:
        fields.append(f".value_u32 = {number('amount') & 0xFFFFFFFF}u")
    elif op in {
        "DOMAIN_ITEM_ADD", "DOMAIN_ITEM_REMOVE",
        "DOMAIN_ITEM_CHECK", "DOMAIN_ITEM_SPACE"
    }:
        fields += [
            f".a = {_u(number('item'))}",
            f".b = {_u(number('quantity'))}",
        ]
    elif op == "DOMAIN_GIVE_MON":
        fields += [
            f".a = {_u(number('species'))}",
            f".b = {_u(number('level'))}",
        ]
    elif op in {"SPECIAL", "SPECIAL_VAR"}:
        fields += [
            f".resource_id = {_cstr(str(ins.get('special_id', '')))}",
            f".value_u32 = {int(ins.get('special_index', 0))}u",
        ]
        if op == "SPECIAL_VAR":
            fields.append(f".a = {_u(number('output_var'))}")
    elif op == "SET_WEATHER":
        fields.append(f".a = {_u(number('weather'))}")
    elif op == "SET_MAP_LAYOUT":
        fields.append(f".a = {_u(number('layout'))}")
    elif op == "SET_METATILE":
        fields += [
            f".a = {_u(number('x_operand'))}",
            f".b = {_u(number('y_operand'))}",
            f".value_u32 = {number('metatile') & 0xFFFFFFFF}u",
            f".target = {number('impassable') & 0xFFFFFFFF}u",
        ]
    elif op == "INCREMENT_GAME_STAT":
        fields.append(f".a = {_u(number('stat'))}")
    elif op == "BUFFER_LEAD_MON_SPECIES_NAME":
        fields.append(f".a = {_u(number('string_var'))}")
    elif op == "SET_FOLLOWER":
        fields += [
            f".a = {_u(number('local_id'))}",
            f".b = {_u(number('flags'))}",
        ]

    return fields


def generate_c_fixture(
    ir: dict[str, Any],
    source_root: Path,
    *,
    symbol_prefix: str = "gR2Generated",
) -> str:
    if ir.get("schema_version") != 1:
        raise ScriptCGenerationError(
            f"unsupported IR schema version: {ir.get('schema_version')!r}"
        )

    scripts = list(ir.get("scripts", []))
    resolver = build_numeric_resolver(Path(source_root))
    program_indices = {
        str(script["script_id"]): index for index, script in enumerate(scripts)
    }
    prefix = _cident(symbol_prefix)
    lines = [
        "/* Generated from R2 semantic script IR. Do not hand-edit. */",
        '#include "remaster/emerald_script.h"',
        '#include "remaster/emerald_script_runtime.h"',
        "",
    ]

    array_names: list[str] = []
    for index, script in enumerate(scripts):
        script_id = str(script.get("script_id", f"script_{index}"))
        source_info = script.get("source") or {}
        source = (
            Path(str(source_info.get("file")))
            if source_info.get("file")
            else None
        )
        array_name = f"{prefix}Instructions_{index}"
        array_names.append(array_name)
        lines.append(
            f"static const RemasterEmeraldScriptInstruction {array_name}[] = {{"
        )
        for ins in script.get("instructions", []):
            fields = _instruction_fields(
                ins, resolver, source, script_id, program_indices
            )
            lines.append("    { " + ", ".join(fields) + " },")
        lines.append("};")
        lines.append("")

    lines.append(
        f"const RemasterEmeraldScriptProgram {prefix}Programs[] = {{"
    )
    for index, script in enumerate(scripts):
        script_id = str(script.get("script_id", f"script_{index}"))
        lines.append(
            "    { "
            + f".script_id = {_cstr(script_id)}, "
            + f".instructions = {array_names[index]}, "
            + f".instruction_count = sizeof({array_names[index]}) / sizeof({array_names[index]}[0])"
            + " },"
        )
    lines.append("};")
    lines.append("")
    lines.append(
        f"const RemasterEmeraldScriptRegistry {prefix}Registry = {{"
    )
    lines.append(f"    {prefix}Programs,")
    lines.append(
        f"    sizeof({prefix}Programs) / sizeof({prefix}Programs[0]),"
    )
    lines.append("};")
    lines.append("")

    map_entries: list[tuple[str, Any, Any, str]] = []
    for entry in ir.get("map_scripts", []):
        map_entries.append(
            (
                str(entry.get("kind", "")),
                0,
                0,
                str(entry.get("script_id", "")),
            )
        )
    for entry in ir.get("map_script_tables", []):
        map_entries.append(
            (
                "MAP_SCRIPT_ON_FRAME_TABLE",
                entry.get("var"),
                entry.get("value"),
                str(entry.get("script_id", "")),
            )
        )

    lines.append(
        f"const RemasterEmeraldMapScriptEntry {prefix}MapScripts[] = {{"
    )
    hook_map = {
        "MAP_SCRIPT_ON_LOAD": "REMASTER_EMERALD_MAP_SCRIPT_ON_LOAD",
        "MAP_SCRIPT_ON_TRANSITION":
            "REMASTER_EMERALD_MAP_SCRIPT_ON_TRANSITION",
        "MAP_SCRIPT_ON_FRAME_TABLE":
            "REMASTER_EMERALD_MAP_SCRIPT_ON_FRAME_TABLE",
        "MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE":
            "REMASTER_EMERALD_MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE",
    }
    for kind, lhs, rhs, script_id in map_entries:
        if kind not in hook_map:
            raise ScriptCGenerationError(f"unknown map script hook {kind}")
        lhs_num = resolver.resolve(lhs) if lhs not in (None, 0) else 0
        rhs_num = resolver.resolve(rhs) if rhs not in (None, 0) else 0
        lines.append(
            "    { "
            + f".hook = {hook_map[kind]}, "
            + f".lhs = {_u(lhs_num)}, .rhs = {_u(rhs_num)}, "
            + f".script_id = {_cstr(script_id)}"
            + " },"
        )
    lines.append("};")
    lines.append("")
    lines.append(
        f"const size_t {prefix}MapScriptCount = "
        + f"sizeof({prefix}MapScripts) / sizeof({prefix}MapScripts[0]);"
    )
    lines.append("")

    return "\n".join(lines)


__all__ = [
    "NumericResolver",
    "ScriptCGenerationError",
    "build_numeric_resolver",
    "generate_c_fixture",
]
