#!/usr/bin/env python3
"""Build the portable R10 quest catalog from pinned Vanilla+ identities.

The checked-in C include is generated from data/r10/quest_catalog.json while
numeric flag/var/map/region identities and Region Map marker coordinates are
resolved from the vendored Vanilla+ source. The generator also audits every
local object target against the real map JSON and checks that the declarative
catalog still matches the vendored Phase 10A objective blocks.
"""

from __future__ import annotations

import ast
import json
from pathlib import Path
import re
import sys
from typing import Any


CONDITION_ENUM = {
    "flag_set": "REMASTER_EMERALD_QUEST_CONDITION_FLAG_SET",
    "flag_clear": "REMASTER_EMERALD_QUEST_CONDITION_FLAG_CLEAR",
    "var_eq": "REMASTER_EMERALD_QUEST_CONDITION_VAR_EQ",
    "var_ne": "REMASTER_EMERALD_QUEST_CONDITION_VAR_NE",
    "var_ge": "REMASTER_EMERALD_QUEST_CONDITION_VAR_GE",
    "var_lt": "REMASTER_EMERALD_QUEST_CONDITION_VAR_LT",
}

SOURCE_CONDITION = {
    "flag_set": "QC_FLAG_SET",
    "flag_clear": "QC_FLAG_CLEAR",
    "var_eq": "QC_VAR_EQ",
    "var_ne": "QC_VAR_NE",
    "var_ge": "QC_VAR_GE",
    "var_lt": "QC_VAR_LT",
}

TARGET_ENUM = {
    "region": "REMASTER_EMERALD_QUEST_TARGET_REGION",
    "map": "REMASTER_EMERALD_QUEST_TARGET_MAP",
    "object_event": "REMASTER_EMERALD_QUEST_TARGET_OBJECT_EVENT",
    "coordinate": "REMASTER_EMERALD_QUEST_TARGET_COORDINATE",
}

SOURCE_TARGET = {
    "region": "QUEST_TARGET_REGION",
    "map": "QUEST_TARGET_MAP",
    "object_event": "QUEST_TARGET_OBJECT_EVENT",
    "coordinate": "QUEST_TARGET_COORDINATE",
}


class CatalogError(RuntimeError):
    pass


class DefineResolver:
    def __init__(self, *sources: str) -> None:
        self.expr: dict[str, str] = {}
        self.hints: dict[str, int] = {}
        self.cache: dict[str, int] = {}

        for source in sources:
            for raw in source.splitlines():
                match = re.match(
                    r"^\s*#define\s+([A-Za-z_]\w*)\s+(.+?)\s*(?://(.*))?$",
                    raw,
                )
                if not match:
                    continue

                name, expression, comment = match.groups()
                if "(" in name:
                    continue

                self.expr[name] = expression.strip()
                if comment:
                    hint = re.search(r"\b0x([0-9A-Fa-f]+)\b", comment)
                    if hint:
                        self.hints[name] = int(hint.group(1), 16)

    def resolve(self, name: str) -> int:
        return self._resolve(name, set())

    def _resolve(self, name: str, stack: set[str]) -> int:
        if name in self.cache:
            return self.cache[name]
        if name in stack:
            raise CatalogError(f"recursive #define while resolving {name}")
        if name not in self.expr:
            if name in self.hints:
                return self.hints[name]
            raise CatalogError(f"unresolved constant: {name}")

        stack = set(stack)
        stack.add(name)

        expression = re.sub(
            r"\b(0x[0-9A-Fa-f]+|\d+)[uUlL]+\b",
            r"\1",
            self.expr[name],
        )

        identifiers = sorted(set(re.findall(r"\b[A-Za-z_]\w*\b", expression)))
        for identifier in identifiers:
            if identifier == name:
                raise CatalogError(f"self-referencing #define: {name}")
            try:
                value = self._resolve(identifier, stack)
            except CatalogError:
                if name in self.hints:
                    self.cache[name] = self.hints[name]
                    return self.hints[name]
                raise
            expression = re.sub(
                rf"\b{re.escape(identifier)}\b",
                str(value),
                expression,
            )

        if not re.fullmatch(r"[0-9A-Fa-fxX\s()+\-*/|&<>]+", expression):
            if name in self.hints:
                self.cache[name] = self.hints[name]
                return self.hints[name]
            raise CatalogError(f"unsafe/unhandled expression for {name}: {expression}")

        node = ast.parse(expression, mode="eval")
        value = self._eval_ast(node.body)
        self.cache[name] = value
        return value

    def _eval_ast(self, node: ast.AST) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return int(node.value)
        if isinstance(node, ast.UnaryOp):
            operand = self._eval_ast(node.operand)
            if isinstance(node.op, ast.UAdd):
                return operand
            if isinstance(node.op, ast.USub):
                return -operand
        if isinstance(node, ast.BinOp):
            left = self._eval_ast(node.left)
            right = self._eval_ast(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.Mult):
                return left * right
            if isinstance(node.op, ast.FloorDiv):
                return left // right
            if isinstance(node.op, ast.Div):
                return left // right
            if isinstance(node.op, ast.BitOr):
                return left | right
            if isinstance(node.op, ast.BitAnd):
                return left & right
            if isinstance(node.op, ast.LShift):
                return left << right
            if isinstance(node.op, ast.RShift):
                return left >> right
        raise CatalogError(f"unsupported #define AST: {ast.dump(node)}")


def c_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def objective_source_block(source: str, key: str) -> str:
    needle = f".id = QUESTOBJ_{key}"
    index = source.find(needle)
    if index < 0:
        raise CatalogError(f"vendored Phase 10A objective missing: {key}")

    start = source.rfind("{", 0, index)
    if start < 0:
        raise CatalogError(f"could not find source block start for {key}")

    depth = 0
    for pos in range(start, len(source)):
        char = source[pos]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : pos + 1]

    raise CatalogError(f"could not find source block end for {key}")


def source_condition_token(condition: dict[str, Any]) -> str:
    macro = SOURCE_CONDITION[condition["type"]]
    if condition["type"].startswith("flag_"):
        return f"{macro}({condition['id']})"
    return f"{macro}({condition['id']}, {condition['value']})"


def validate_against_phase10a(
    vendor: Path,
    objectives: list[dict[str, Any]],
) -> None:
    source = (vendor / "src" / "quest_system.c").read_text(encoding="utf-8")

    for objective in objectives:
        block = objective_source_block(source, objective["key"])
        target = objective["target"]

        expected_tokens = [
            f".mapSecId = {objective['map_section']}",
            f".targetType = {SOURCE_TARGET[target['type']]}",
        ]

        if target["type"] in {"map", "object_event", "coordinate"}:
            expected_tokens.extend(
                [
                    f".mapGroup = MAP_GROUP({target['map'][4:]})",
                    f".mapNum = MAP_NUM({target['map'][4:]})",
                ]
            )
        if target["type"] == "object_event":
            expected_tokens.append(f".localId = {target['local_id']}")
        if target["type"] == "coordinate":
            expected_tokens.extend(
                [f".x = {target['x']}", f".y = {target['y']}"]
            )

        for token in expected_tokens:
            if token not in block:
                raise CatalogError(
                    f"{objective['key']}: Phase 10A mismatch, missing {token}"
                )

        activation = re.search(
            r"\.activation\s*=\s*\{([^}]*)\}",
            block,
            flags=re.S,
        )
        completion = re.search(
            r"\.completion\s*=\s*\{([^}]*)\}",
            block,
            flags=re.S,
        )
        if not activation or not completion:
            raise CatalogError(
                f"{objective['key']}: could not parse Phase 10A conditions"
            )

        for condition in objective["activation"]:
            token = source_condition_token(condition)
            if token not in activation.group(1):
                raise CatalogError(
                    f"{objective['key']}: activation mismatch, missing {token}"
                )

        for condition in objective["completion"]:
            token = source_condition_token(condition)
            if token not in completion.group(1):
                raise CatalogError(
                    f"{objective['key']}: completion mismatch, missing {token}"
                )


def resolve_map_directory(
    map_value: int,
    map_groups: dict[str, Any],
) -> tuple[int, int, str]:
    group = (map_value >> 8) & 0xFF
    number = map_value & 0xFF
    order = map_groups["group_order"]

    try:
        group_name = order[group]
        directory = map_groups[group_name][number]
    except (IndexError, KeyError) as exc:
        raise CatalogError(
            f"map identity {group}:{number} not present in map_groups.json"
        ) from exc

    return group, number, directory


def condition_initializer(
    condition: dict[str, Any],
    flags: DefineResolver,
    vars_: DefineResolver,
) -> str:
    ctype = CONDITION_ENUM[condition["type"]]
    if condition["type"].startswith("flag_"):
        identity = flags.resolve(condition["id"])
        value = 0
    else:
        identity = vars_.resolve(condition["id"])
        value = int(condition["value"])

    return (
        "{"
        f"{ctype}, 0x{identity:04X}, {value}"
        "}"
    )


def padded_conditions(
    conditions: list[dict[str, Any]],
    flags: DefineResolver,
    vars_: DefineResolver,
) -> str:
    if len(conditions) > 4:
        raise CatalogError("R10 supports at most four conditions per set")

    rendered = [
        condition_initializer(condition, flags, vars_)
        for condition in conditions
    ]
    rendered.extend(["{0, 0, 0}"] * (4 - len(rendered)))
    return ", ".join(rendered)


def build(
    vendor: Path,
    catalog_path: Path,
) -> str:
    payload = json.loads(catalog_path.read_text(encoding="utf-8"))
    objectives: list[dict[str, Any]] = payload["objectives"]

    if len(objectives) != 32:
        raise CatalogError(f"expected 32 objectives, found {len(objectives)}")
    if [objective["id"] for objective in objectives] != list(range(1, 33)):
        raise CatalogError("objective IDs must be sequential 1..32")

    validate_against_phase10a(vendor, objectives)

    flags_text = (vendor / "include" / "constants" / "flags.h").read_text(
        encoding="utf-8"
    )
    vars_text = (vendor / "include" / "constants" / "vars.h").read_text(
        encoding="utf-8"
    )
    maps_text = (vendor / "include" / "constants" / "map_groups.h").read_text(
        encoding="utf-8"
    )
    sections_text = (
        vendor / "include" / "constants" / "region_map_sections.h"
    ).read_text(encoding="utf-8")

    flags = DefineResolver(flags_text)
    vars_ = DefineResolver(vars_text)
    maps = DefineResolver(maps_text)
    sections = DefineResolver(sections_text)

    map_groups = json.loads(
        (vendor / "data" / "maps" / "map_groups.json").read_text(
            encoding="utf-8"
        )
    )
    region_entries = json.loads(
        (
            vendor
            / "src"
            / "data"
            / "region_map"
            / "region_map_sections.json"
        ).read_text(encoding="utf-8")
    )["map_sections"]
    regions = {entry["map_section"]: entry for entry in region_entries}

    lines = [
        "/*",
        " * AUTO-GENERATED by tools/build_r10_quest_catalog.py.",
        " * Source: data/r10/quest_catalog.json + pinned vendor/vanillaplus.",
        " * Do not edit this file by hand.",
        " */",
        "",
        "static const RemasterEmeraldQuestObjective kRemasterEmeraldQuestObjectives[] =",
        "{",
    ]

    object_target_count = 0

    for objective in objectives:
        target = objective["target"]
        map_section_name = objective["map_section"]
        map_section_id = sections.resolve(map_section_name)

        if map_section_name not in regions:
            raise CatalogError(
                f"{objective['key']}: Region Map entry missing for {map_section_name}"
            )
        marker = regions[map_section_name]

        map_group = 0
        map_num = 0
        local_id = 0
        target_x = int(target.get("x", 0))
        target_y = int(target.get("y", 0))

        if target["type"] in {"map", "object_event", "coordinate"}:
            map_value = maps.resolve(target["map"])
            map_group, map_num, directory = resolve_map_directory(
                map_value,
                map_groups,
            )

            map_json = json.loads(
                (
                    vendor
                    / "data"
                    / "maps"
                    / directory
                    / "map.json"
                ).read_text(encoding="utf-8")
            )
            if map_json["id"] != target["map"]:
                raise CatalogError(
                    f"{objective['key']}: map JSON identity mismatch for {target['map']}"
                )

            if target["type"] == "object_event":
                object_target_count += 1
                local_id = int(target["local_id"])
                events = map_json.get("object_events", [])
                if local_id < 1 or local_id > len(events):
                    raise CatalogError(
                        f"{objective['key']}: local ID {local_id} missing on {target['map']}"
                    )
                event = events[local_id - 1]
                if event.get("script") != target["expected_script"]:
                    raise CatalogError(
                        f"{objective['key']}: local ID {local_id} script mismatch: "
                        f"{event.get('script')} != {target['expected_script']}"
                    )

        activation = objective["activation"]
        completion = objective["completion"]

        lines.extend(
            [
                "    {",
                f"        .id = {objective['id']},",
                f"        .key = {c_string(objective['key'])},",
                f"        .title = {c_string(objective['title'])},",
                f"        .description = {c_string(objective['description'])},",
                f"        .map_section_id = 0x{map_section_id:04X},",
                f"        .target_type = {TARGET_ENUM[target['type']]},",
                f"        .map_group = {map_group},",
                f"        .map_num = {map_num},",
                f"        .local_id = {local_id},",
                f"        .x = {target_x},",
                f"        .y = {target_y},",
                "        .region_marker = {",
                f"            0x{map_section_id:04X},",
                f"            {int(marker['x'])},",
                f"            {int(marker['y'])},",
                f"            {int(marker['width'])},",
                f"            {int(marker['height'])}",
                "        },",
                f"        .activation_count = {len(activation)},",
                f"        .completion_count = {len(completion)},",
                "        .activation = {"
                + padded_conditions(activation, flags, vars_)
                + "},",
                "        .completion = {"
                + padded_conditions(completion, flags, vars_)
                + "}",
                "    },",
            ]
        )

    if object_target_count != 17:
        raise CatalogError(
            f"expected 17 object-event targets, found {object_target_count}"
        )

    lines.extend(["};", ""])
    return "\n".join(lines)


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: build_r10_quest_catalog.py "
            "<vendor-root> <quest-catalog.json> <output.inc>",
            file=sys.stderr,
        )
        return 2

    vendor = Path(sys.argv[1]).resolve()
    catalog_path = Path(sys.argv[2]).resolve()
    output = Path(sys.argv[3]).resolve()

    try:
        rendered = build(vendor, catalog_path)
    except (CatalogError, KeyError, ValueError, json.JSONDecodeError) as exc:
        print(f"R10 quest catalog generation FAILED: {exc}", file=sys.stderr)
        return 1

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8", newline="\n")

    payload = json.loads(catalog_path.read_text(encoding="utf-8"))
    object_targets = sum(
        1
        for objective in payload["objectives"]
        if objective["target"]["type"] == "object_event"
    )

    print("R10 quest catalog generation PASSED")
    print(f" - objectives: {len(payload['objectives'])}")
    print(f" - object-event targets: {object_targets}")
    print(" - numeric identities resolved from pinned Vanilla+")
    print(" - Region Map marker coordinates resolved from canonical data")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
