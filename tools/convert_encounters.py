#!/usr/bin/env python3
"""Convert Vanilla+ wild encounter data into portable R3 JSON."""

from __future__ import annotations

import argparse
import ast
import json
import re
from pathlib import Path
from typing import Any

from r3_source_catalog import build_source_catalog


SCHEMA_VERSION = 1


class EncounterConversionError(ValueError):
    pass


_ALLOWED_BINOPS = {
    ast.Add: lambda a, b: a + b,
    ast.Sub: lambda a, b: a - b,
    ast.Mult: lambda a, b: a * b,
    ast.FloorDiv: lambda a, b: a // b,
    ast.Div: lambda a, b: a // b,
    ast.Mod: lambda a, b: a % b,
    ast.LShift: lambda a, b: a << b,
    ast.RShift: lambda a, b: a >> b,
    ast.BitOr: lambda a, b: a | b,
    ast.BitAnd: lambda a, b: a & b,
}


def _eval_int(expr: str, symbols: dict[str, int]) -> int:
    node = ast.parse(expr, mode="eval")

    def visit(value: ast.AST) -> int:
        if isinstance(value, ast.Expression):
            return visit(value.body)
        if isinstance(value, ast.Constant) and isinstance(value.value, int):
            return int(value.value)
        if isinstance(value, ast.Name):
            if value.id not in symbols:
                raise KeyError(value.id)
            return symbols[value.id]
        if isinstance(value, ast.UnaryOp):
            operand = visit(value.operand)
            if isinstance(value.op, ast.UAdd):
                return operand
            if isinstance(value.op, ast.USub):
                return -operand
            if isinstance(value.op, ast.Invert):
                return ~operand
        if isinstance(value, ast.BinOp) and type(value.op) in _ALLOWED_BINOPS:
            return _ALLOWED_BINOPS[type(value.op)](
                visit(value.left),
                visit(value.right),
            )
        raise ValueError(expr)

    return visit(node)


def build_species_index(source_root: Path) -> dict[str, int]:
    path = Path(source_root) / "include/constants/species.h"
    if not path.is_file():
        raise EncounterConversionError(f"missing species constants: {path}")

    pattern = re.compile(
        r"^\s*#define\s+(SPECIES_[A-Za-z0-9_]+)\s+(.+?)\s*$"
    )
    expressions: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("//", 1)[0].strip()
        match = pattern.match(line)
        if not match:
            continue
        name, expr = match.groups()
        expr = re.sub(r"(?<=\d)[uUlL]+\b", "", expr.strip())
        expressions[name] = expr

    resolved: dict[str, int] = {}
    pending = dict(expressions)
    while pending:
        progressed = False
        for name, expr in list(pending.items()):
            try:
                resolved[name] = _eval_int(expr, resolved)
            except (KeyError, SyntaxError, ValueError, ZeroDivisionError):
                continue
            del pending[name]
            progressed = True
        if not progressed:
            break

    if "SPECIES_NONE" not in resolved:
        raise EncounterConversionError(
            f"{path}: failed to resolve SPECIES_NONE"
        )
    return resolved


def _convert_mon(
    mon: dict[str, Any],
    species: dict[str, int],
    context: str,
) -> dict[str, Any]:
    min_level = mon.get("min_level")
    max_level = mon.get("max_level")
    species_name = mon.get("species")

    if (
        not isinstance(min_level, int)
        or not isinstance(max_level, int)
        or min_level < 1
        or max_level > 100
        or min_level > max_level
    ):
        raise EncounterConversionError(
            f"{context}: invalid level range {min_level!r}..{max_level!r}"
        )
    if not isinstance(species_name, str) or species_name not in species:
        raise EncounterConversionError(
            f"{context}: unresolved species {species_name!r}"
        )

    return {
        **mon,
        "species_id": species[species_name],
    }


def _convert_encounter_field(
    field_name: str,
    value: dict[str, Any],
    species: dict[str, int],
    expected_slots: int | None,
    context: str,
) -> dict[str, Any]:
    rate = value.get("encounter_rate")
    mons = value.get("mons")

    if not isinstance(rate, int) or rate < 0 or rate > 100:
        raise EncounterConversionError(
            f"{context}.{field_name}: invalid encounter_rate {rate!r}"
        )
    if not isinstance(mons, list):
        raise EncounterConversionError(
            f"{context}.{field_name}: mons must be an array"
        )
    if expected_slots is not None and len(mons) != expected_slots:
        raise EncounterConversionError(
            f"{context}.{field_name}: slot count {len(mons)} "
            f"!= expected {expected_slots}"
        )

    return {
        **value,
        "mons": [
            _convert_mon(
                mon,
                species,
                f"{context}.{field_name}.mons[{index}]",
            )
            for index, mon in enumerate(mons)
        ],
    }


def convert_encounters(source_root: Path) -> dict[str, Any]:
    source_root = Path(source_root)
    path = source_root / "src/data/wild_encounters.json"
    if not path.is_file():
        raise EncounterConversionError(f"missing encounter source: {path}")

    source = json.loads(path.read_text(encoding="utf-8"))
    groups = source.get("wild_encounter_groups")
    if not isinstance(groups, list):
        raise EncounterConversionError(
            f"{path}: wild_encounter_groups must be an array"
        )

    source_catalog = build_source_catalog(source_root)
    map_by_id = {
        item["id"]: item
        for item in source_catalog["maps"]
    }
    species = build_species_index(source_root)

    converted_groups: list[dict[str, Any]] = []
    map_encounter_count = 0

    for group_index, group in enumerate(groups):
        if not isinstance(group, dict):
            raise EncounterConversionError(
                f"group {group_index}: expected object"
            )
        label = group.get("label")
        for_maps = bool(group.get("for_maps", False))
        fields = group.get("fields", [])
        encounters = group.get("encounters", [])

        if not isinstance(label, str) or not label:
            raise EncounterConversionError(
                f"group {group_index}: invalid label"
            )
        if not isinstance(fields, list) or not isinstance(encounters, list):
            raise EncounterConversionError(
                f"{label}: fields/encounters must be arrays"
            )

        expected_slots: dict[str, int] = {}
        converted_fields: list[dict[str, Any]] = []
        for field_index, field in enumerate(fields):
            if not isinstance(field, dict):
                raise EncounterConversionError(
                    f"{label}.fields[{field_index}]: expected object"
                )
            field_type = field.get("type")
            rates = field.get("encounter_rates")
            if not isinstance(field_type, str) or not isinstance(rates, list):
                raise EncounterConversionError(
                    f"{label}.fields[{field_index}]: invalid field"
                )
            if not all(
                isinstance(rate, int) and 0 <= rate <= 100
                for rate in rates
            ):
                raise EncounterConversionError(
                    f"{label}.{field_type}: invalid slot probability"
                )
            expected_slots[field_type] = len(rates)

            rod_groups = field.get("groups")
            if rod_groups is not None:
                if not isinstance(rod_groups, dict):
                    raise EncounterConversionError(
                        f"{label}.{field_type}: groups must be an object"
                    )
                for rod_name, indices in rod_groups.items():
                    if (
                        not isinstance(indices, list)
                        or not all(
                            isinstance(index, int)
                            and 0 <= index < len(rates)
                            for index in indices
                        )
                    ):
                        raise EncounterConversionError(
                            f"{label}.{field_type}.{rod_name}: invalid slot index"
                        )

            converted_fields.append(dict(field))

        converted_encounters: list[dict[str, Any]] = []
        for encounter_index, encounter in enumerate(encounters):
            if not isinstance(encounter, dict):
                raise EncounterConversionError(
                    f"{label}.encounters[{encounter_index}]: expected object"
                )
            context = f"{label}.encounters[{encounter_index}]"
            converted: dict[str, Any] = {}

            for key, value in encounter.items():
                if key.endswith("_mons"):
                    if not isinstance(value, dict):
                        raise EncounterConversionError(
                            f"{context}.{key}: expected object"
                        )
                    converted[key] = _convert_encounter_field(
                        key,
                        value,
                        species,
                        expected_slots.get(key),
                        context,
                    )
                else:
                    converted[key] = value

            if for_maps:
                map_id = encounter.get("map")
                if not isinstance(map_id, str) or map_id not in map_by_id:
                    raise EncounterConversionError(
                        f"{context}: unknown map {map_id!r}"
                    )
                map_item = map_by_id[map_id]
                converted["map_name"] = map_item["name"]
                converted["group_num"] = map_item["group_num"]
                converted["map_num"] = map_item["map_num"]
                map_encounter_count += 1

            converted_encounters.append(converted)

        converted_groups.append(
            {
                **group,
                "fields": converted_fields,
                "encounters": converted_encounters,
            }
        )

    return {
        "schema_version": SCHEMA_VERSION,
        "source": "src/data/wild_encounters.json",
        "group_count": len(converted_groups),
        "map_encounter_count": map_encounter_count,
        "groups": converted_groups,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    result = convert_encounters(args.source_root.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
