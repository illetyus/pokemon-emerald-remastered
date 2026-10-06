#!/usr/bin/env python3
"""Generate the R11 Pokémon/move/item/evolution catalog from pinned Vanilla+."""

from __future__ import annotations

import ast
import json
from pathlib import Path
import re
import sys
from typing import Any


class CatalogError(RuntimeError):
    pass


class DefineResolver:
    def __init__(self, include_root: Path) -> None:
        self.expr: dict[str, str] = {"TRUE": "1", "FALSE": "0"}
        self.cache: dict[str, int] = {}
        for path in sorted(include_root.rglob("*.h")):
            text = path.read_text(encoding="utf-8", errors="ignore")
            logical = re.sub(r"\\\n", " ", text)
            for raw in logical.splitlines():
                raw = re.sub(r"//.*$", "", raw).strip()
                m = re.match(r"#define\s+([A-Za-z_]\w*)\s+(.+)$", raw)
                if not m:
                    continue
                name, expression = m.groups()
                if "(" in name:
                    continue
                self.expr[name] = expression.strip()

    def resolve(self, name: str) -> int:
        return self.eval_expr(name)

    def eval_expr(self, expression: str, stack: tuple[str, ...] = ()) -> int:
        expression = expression.strip().rstrip(",")
        expression = re.sub(r"/\*.*?\*/", "", expression)
        expression = re.sub(r"\b(0x[0-9A-Fa-f]+|\d+)[uUlL]+\b", r"\1", expression)
        expression = re.sub(r"\((?:u|s)?(?:8|16|32|64)\)", "", expression)

        percent = re.fullmatch(r"PERCENT_FEMALE\((\d+(?:\.\d+)?)\)", expression)
        if percent:
            value = float(percent.group(1))
            return min(254, int((value * 255) / 100))

        item_to_mail = re.fullmatch(r"ITEM_TO_MAIL\(([^)]+)\)", expression)
        if item_to_mail:
            return self.eval_expr(item_to_mail.group(1), stack) - self.resolve("FIRST_MAIL_INDEX")

        item_to_berry = re.fullmatch(r"ITEM_TO_BERRY\(([^)]+)\)", expression)
        if item_to_berry:
            return self.eval_expr(item_to_berry.group(1), stack) - self.resolve("FIRST_BERRY_INDEX") + 1

        if re.fullmatch(r"[A-Za-z_]\w*", expression):
            name = expression
            if name in self.cache:
                return self.cache[name]
            if name in stack:
                raise CatalogError(f"recursive define: {name}")
            if name not in self.expr:
                raise CatalogError(f"unresolved constant: {name}")
            value = self.eval_expr(self.expr[name], stack + (name,))
            self.cache[name] = value
            return value

        identifiers = sorted(set(re.findall(r"\b[A-Za-z_]\w*\b", expression)))
        for identifier in identifiers:
            if identifier in {"min", "max"}:
                continue
            value = self.eval_expr(identifier, stack)
            expression = re.sub(
                rf"\b{re.escape(identifier)}\b",
                str(value),
                expression,
            )

        node = ast.parse(expression, mode="eval").body
        return self._eval_ast(node)

    def _eval_ast(self, node: ast.AST) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return int(node.value)
        if isinstance(node, ast.UnaryOp):
            value = self._eval_ast(node.operand)
            if isinstance(node.op, ast.UAdd):
                return value
            if isinstance(node.op, ast.USub):
                return -value
            if isinstance(node.op, ast.Invert):
                return ~value
        if isinstance(node, ast.BinOp):
            left = self._eval_ast(node.left)
            right = self._eval_ast(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.Mult):
                return left * right
            if isinstance(node.op, (ast.Div, ast.FloorDiv)):
                return left // right
            if isinstance(node.op, ast.Mod):
                return left % right
            if isinstance(node.op, ast.BitOr):
                return left | right
            if isinstance(node.op, ast.BitAnd):
                return left & right
            if isinstance(node.op, ast.BitXor):
                return left ^ right
            if isinstance(node.op, ast.LShift):
                return left << right
            if isinstance(node.op, ast.RShift):
                return left >> right
        raise CatalogError(f"unsupported expression AST: {ast.dump(node)}")


def extract_blocks(text: str) -> dict[str, str]:
    blocks: dict[str, str] = {}
    pattern = re.compile(r"\[\s*([A-Z][A-Z0-9_]*)\s*\]\s*=\s*")
    for match in pattern.finditer(text):
        symbol = match.group(1)
        pos = match.end()
        while pos < len(text) and text[pos].isspace():
            pos += 1
        if pos >= len(text):
            continue
        if text[pos] == "{":
            depth = 0
            end = pos
            while end < len(text):
                if text[end] == "{":
                    depth += 1
                elif text[end] == "}":
                    depth -= 1
                    if depth == 0:
                        blocks[symbol] = text[pos : end + 1]
                        break
                end += 1
        else:
            end = pos
            while end < len(text) and text[end] not in ",\n":
                end += 1
            blocks[symbol] = text[pos:end].strip()
    return blocks


def field_expr(block: str, name: str, default: str = "0") -> str:
    m = re.search(
        rf"\.{re.escape(name)}\s*=\s*([^,\n}}]+)",
        block,
    )
    return m.group(1).strip() if m else default


def pair_expr(
    block: str,
    name: str,
    default: tuple[str, str] = ("0", "0"),
) -> tuple[str, str]:
    m = re.search(
        rf"\.{re.escape(name)}\s*=\s*\{{\s*([^,}}]+)\s*,\s*([^,}}]+)\s*,?\s*\}}",
        block,
    )
    if not m:
        return default
    return m.group(1).strip(), m.group(2).strip()


def parse_charmap(vendor: Path) -> dict[str, tuple[int, ...]]:
    """Parse literal chars and named {TOKENS} into encoded byte runs."""
    mapping: dict[str, tuple[int, ...]] = {}
    text = (vendor / "charmap.txt").read_text(encoding="utf-8")
    byte_run = r"([0-9A-Fa-f]{2}(?:\s+[0-9A-Fa-f]{2})*)"

    for original in text.splitlines():
        raw = original.split("@", 1)[0].strip()
        if not raw:
            continue

        char_match = re.fullmatch(
            rf"'((?:\\.|[^'])+)'\s*=\s*{byte_run}",
            raw,
        )
        if char_match:
            token = char_match.group(1)
            if token == r"\'":
                token = "'"
            elif token == r"\\":
                token = "\\"
            if len(token) == 1:
                mapping.setdefault(
                    token,
                    tuple(int(value, 16) for value in char_match.group(2).split()),
                )
            continue

        named_match = re.fullmatch(
            rf"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*{byte_run}",
            raw,
        )
        if named_match:
            mapping.setdefault(
                named_match.group(1),
                tuple(int(value, 16) for value in named_match.group(2).split()),
            )

    return mapping


def item_name(block: str) -> str:
    match = re.search(r'\.name\s*=\s*_\("((?:\\.|[^"\\])*)"\)', block)
    if not match:
        raise CatalogError("item name missing")
    return ast.literal_eval('"' + match.group(1) + '"')


def turkish_sort_weight(encoded: int) -> int:
    upper = [
        1, 2, 3, 5, 6, 7, 8, 10, 11, 13, 14, 15, 16,
        17, 18, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32,
    ]
    lower = [
        1, 2, 3, 5, 6, 7, 8, 10, 12, 13, 14, 15, 16,
        17, 18, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32,
    ]
    if 0xBB <= encoded <= 0xD4:
        return upper[encoded - 0xBB]
    if 0xD5 <= encoded <= 0xEE:
        return lower[encoded - 0xD5]
    special = {
        0x04: 4,
        0x19: 4,
        0x01: 9,
        0x16: 9,
        0x1E: 11,
        0x09: 12,
        0xF2: 19,
        0xF5: 19,
        0x05: 24,
        0x1A: 24,
        0xF3: 27,
        0xF6: 27,
        0xFF: 0,
    }
    return special.get(encoded, 0x100 + encoded)


def item_name_sort_key(
    name: str,
    charmap: dict[str, tuple[int, ...]],
) -> tuple[int, ...]:
    key: list[int] = []
    pos = 0
    while pos < len(name):
        if name[pos] == "{":
            end = name.find("}", pos + 1)
            if end < 0:
                raise CatalogError(f"unterminated charmap token in item name: {name!r}")
            token = name[pos + 1 : end]
            encoded = charmap.get(token)
            if encoded is None:
                raise CatalogError(
                    f"item-name token missing from charmap: {token!r} in {name!r}"
                )
            pos = end + 1
        else:
            char = name[pos]
            encoded = charmap.get(char)
            if encoded is None:
                raise CatalogError(
                    f"item-name character missing from charmap: {char!r} in {name!r}"
                )
            pos += 1

        key.extend(turkish_sort_weight(byte) for byte in encoded)

    return tuple(key)


def old_unown_block(text: str) -> str:
    start = text.index("#define OLD_UNOWN_SPECIES_INFO")
    end = text.index("const struct SpeciesInfo", start)
    chunk = text[start:end]
    chunk = chunk.replace("\\\n", " ")
    brace = chunk.find("{")
    if brace < 0:
        raise CatalogError("OLD_UNOWN_SPECIES_INFO body missing")
    depth = 0
    for i in range(brace, len(chunk)):
        if chunk[i] == "{":
            depth += 1
        elif chunk[i] == "}":
            depth -= 1
            if depth == 0:
                return chunk[brace : i + 1]
    raise CatalogError("OLD_UNOWN_SPECIES_INFO body unterminated")


def parse_species(
    vendor: Path,
    r: DefineResolver,
) -> tuple[list[dict[str, Any]], list[int]]:
    text = (vendor / "src/data/pokemon/species_info.h").read_text(encoding="utf-8")
    blocks = extract_blocks(text)
    old = old_unown_block(text)
    count = r.resolve("NUM_SPECIES")
    data: list[dict[str, Any] | None] = [None] * count

    for symbol, block in blocks.items():
        if not symbol.startswith("SPECIES_"):
            continue
        species_id = r.resolve(symbol)
        if species_id >= count:
            continue
        if block == "OLD_UNOWN_SPECIES_INFO":
            block = old
        if block == "{0}":
            data[species_id] = {
                "id": species_id,
                "base": [0] * 6,
                "types": [0, 0],
                "catch": 0,
                "exp": 0,
                "ev": [0] * 6,
                "items": [0, 0],
                "female_threshold": 0,
                "egg_cycles": 0,
                "friendship": 0,
                "growth": 0,
                "egg_groups": [0, 0],
                "abilities": [0, 0],
            }
            continue

        base_names = [
            "baseHP", "baseAttack", "baseDefense",
            "baseSpeed", "baseSpAttack", "baseSpDefense",
        ]
        ev_names = [
            "evYield_HP", "evYield_Attack", "evYield_Defense",
            "evYield_Speed", "evYield_SpAttack", "evYield_SpDefense",
        ]
        types = pair_expr(block, "types")
        eggs = pair_expr(block, "eggGroups")
        abilities = pair_expr(block, "abilities")
        data[species_id] = {
            "id": species_id,
            "base": [r.eval_expr(field_expr(block, x)) for x in base_names],
            "types": [r.eval_expr(types[0]), r.eval_expr(types[1])],
            "catch": r.eval_expr(field_expr(block, "catchRate")),
            "exp": r.eval_expr(field_expr(block, "expYield")),
            "ev": [r.eval_expr(field_expr(block, x)) for x in ev_names],
            "items": [
                r.eval_expr(field_expr(block, "itemCommon")),
                r.eval_expr(field_expr(block, "itemRare")),
            ],
            "female_threshold": r.eval_expr(field_expr(block, "genderRatio")),
            "egg_cycles": r.eval_expr(field_expr(block, "eggCycles")),
            "friendship": r.eval_expr(field_expr(block, "friendship")),
            "growth": r.eval_expr(field_expr(block, "growthRate")),
            "egg_groups": [r.eval_expr(eggs[0]), r.eval_expr(eggs[1])],
            "abilities": [r.eval_expr(abilities[0]), r.eval_expr(abilities[1])],
        }

    missing = [i for i, row in enumerate(data) if row is None]
    return [row or {} for row in data], missing


def parse_moves(
    vendor: Path,
    r: DefineResolver,
) -> tuple[list[dict[str, int]], list[int]]:
    text = (vendor / "src/data/battle_moves.h").read_text(encoding="utf-8")
    blocks = extract_blocks(text)
    count = r.resolve("MOVES_COUNT")
    data: list[dict[str, int] | None] = [None] * count
    for symbol, block in blocks.items():
        if not symbol.startswith("MOVE_"):
            continue
        move_id = r.resolve(symbol)
        if move_id >= count:
            continue
        fields = [
            "effect", "power", "type", "accuracy", "pp",
            "secondaryEffectChance", "target", "priority", "flags",
        ]
        data[move_id] = {"id": move_id}
        for name in fields:
            data[move_id][name] = r.eval_expr(field_expr(block, name))
    missing = [i for i, row in enumerate(data) if row is None]
    return [row or {} for row in data], missing


def parse_items(
    vendor: Path,
    r: DefineResolver,
) -> tuple[list[dict[str, Any]], list[int]]:
    text = (vendor / "src/data/items.h").read_text(encoding="utf-8")
    blocks = extract_blocks(text)
    charmap = parse_charmap(vendor)
    count = r.resolve("ITEMS_COUNT")
    data: list[dict[str, Any] | None] = [None] * count
    for symbol, block in blocks.items():
        if not symbol.startswith("ITEM_"):
            continue
        try:
            item_id = r.resolve(symbol)
        except CatalogError:
            continue
        if item_id >= count:
            continue
        data[item_id] = {
            "id": item_id,
            "name": item_name(block),
            "price": r.eval_expr(field_expr(block, "price")),
            "pocket": r.eval_expr(field_expr(block, "pocket")),
            "hold_effect": r.eval_expr(field_expr(block, "holdEffect")),
            "hold_effect_param": r.eval_expr(field_expr(block, "holdEffectParam")),
            "importance": r.eval_expr(field_expr(block, "importance")),
            "type": r.eval_expr(field_expr(block, "type")),
            "battle_usage": r.eval_expr(field_expr(block, "battleUsage")),
            "secondary_id": r.eval_expr(field_expr(block, "secondaryId")),
        }

    missing = [i for i, row in enumerate(data) if row is None]
    rows = [row or {} for row in data]
    if not missing:
        ordered = sorted(
            rows,
            key=lambda row: (
                item_name_sort_key(str(row["name"]), charmap),
                int(row["id"]),
            ),
        )
        for rank, row in enumerate(ordered):
            row["name_sort_rank"] = rank
    return rows, missing


def parse_evolutions(
    vendor: Path,
    r: DefineResolver,
    species_count: int,
) -> list[list[tuple[int, int, int]]]:
    text = (vendor / "src/data/pokemon/evolution.h").read_text(encoding="utf-8")
    table = [[(0, 0, 0) for _ in range(5)] for _ in range(species_count)]
    pattern = re.compile(r"\[(SPECIES_[A-Z0-9_]+)\]\s*=\s*\{")
    for match in pattern.finditer(text):
        species_id = r.resolve(match.group(1))
        if species_id >= species_count:
            continue
        start = match.end() - 1
        depth = 0
        end = start
        while end < len(text):
            if text[end] == "{":
                depth += 1
            elif text[end] == "}":
                depth -= 1
                if depth == 0:
                    break
            end += 1
        block = text[start : end + 1]
        triples = re.findall(
            r"\{\s*([^,{}]+)\s*,\s*([^,{}]+)\s*,\s*([^,{}]+)\s*\}",
            block,
        )
        for i, triple in enumerate(triples[:5]):
            table[species_id][i] = tuple(r.eval_expr(x) for x in triple)
    return table


def parse_level_up_learnsets(
    vendor: Path,
    r: DefineResolver,
    species_count: int,
) -> tuple[list[tuple[int, int]], list[tuple[int, int]], int]:
    learnset_text = (
        vendor / "src/data/pokemon/level_up_learnsets.h"
    ).read_text(encoding="utf-8")
    pointer_text = (
        vendor / "src/data/pokemon/level_up_learnset_pointers.h"
    ).read_text(encoding="utf-8")

    arrays: dict[str, list[tuple[int, int]]] = {}
    for match in re.finditer(
        r"static const u16\s+(s[A-Za-z0-9_]+LevelUpLearnset)"
        r"\[\]\s*=\s*\{(.*?)\};",
        learnset_text,
        flags=re.S,
    ):
        entries: list[tuple[int, int]] = []
        for item in re.finditer(
            r"LEVEL_UP_MOVE\(\s*(\d+)\s*,\s*(MOVE_[A-Z0-9_]+)\s*\)",
            match.group(2),
        ):
            entries.append((int(item.group(1)), r.resolve(item.group(2))))
        arrays[match.group(1)] = entries

    pointer_by_species: dict[int, str] = {}
    for match in re.finditer(
        r"\[(SPECIES_[A-Z0-9_]+)\]\s*=\s*"
        r"(s[A-Za-z0-9_]+LevelUpLearnset)",
        pointer_text,
    ):
        species_id = r.resolve(match.group(1))
        if species_id < species_count:
            pointer_by_species[species_id] = match.group(2)

    flat: list[tuple[int, int]] = []
    slices: list[tuple[int, int]] = []
    populated = 0
    for species_id in range(species_count):
        entries = arrays.get(pointer_by_species.get(species_id, ""), [])
        start = len(flat)
        flat.extend(entries)
        slices.append((start, len(entries)))
        if entries:
            populated += 1

    return flat, slices, populated


def render(
    species: list[dict[str, Any]],
    moves: list[dict[str, int]],
    items: list[dict[str, Any]],
    evolutions: list[list[tuple[int, int, int]]],
    level_up_moves: list[tuple[int, int]],
    level_up_slices: list[tuple[int, int]],
) -> str:
    lines = [
        "/* AUTO-GENERATED by tools/build_r11_domain_catalog.py. */",
        "/* Source: pinned vendor/vanillaplus Pokémon, move, item and evolution data. */",
        "",
        "static const RemasterEmeraldSpeciesInfo kRemasterEmeraldSpeciesInfo[] = {",
    ]
    for s in species:
        lines.append(
            "    {"
            f"{s['id']},"
            + ",".join(str(x) for x in s["base"])
            + f",{s['types'][0]},{s['types'][1]},{s['catch']},{s['exp']},"
            + "{" + ",".join(str(x) for x in s["ev"]) + "},"
            + f"{s['items'][0]},{s['items'][1]},{s['female_threshold']},"
            + f"{s['egg_cycles']},{s['friendship']},{s['growth']},"
            + "{" + ",".join(str(x) for x in s["egg_groups"]) + "},"
            + "{" + ",".join(str(x) for x in s["abilities"]) + "}"
            + "},"
        )

    lines.extend([
        "};",
        "",
        "static const RemasterEmeraldMoveInfo kRemasterEmeraldMoveInfo[] = {",
    ])
    for m in moves:
        lines.append(
            "    {"
            f"{m['id']},{m['effect']},{m['power']},{m['type']},{m['accuracy']},{m['pp']},"
            f"{m['secondaryEffectChance']},{m['target']},{m['priority']},{m['flags']}"
            "},"
        )

    lines.extend([
        "};",
        "",
        "const RemasterEmeraldItemInfo kRemasterEmeraldItemInfo[] = {",
    ])
    for item in items:
        lines.append(
            "    {"
            f"{item['id']},{item['price']},{item['name_sort_rank']},{item['pocket']},"
            f"{item['hold_effect']},{item['hold_effect_param']},{item['importance']},"
            f"{item['type']},{item['battle_usage']},{item['secondary_id']}"
            "},"
        )

    lines.extend([
        "};",
        "",
        "static const RemasterEmeraldEvolution kRemasterEmeraldEvolutionTable[][5] = {",
    ])
    for row in evolutions:
        lines.append(
            "    {"
            + ",".join(
                "{" + ",".join(str(x) for x in triple) + "}"
                for triple in row
            )
            + "},"
        )

    lines.extend([
        "};",
        "",
        "static const RemasterEmeraldLevelUpMove kRemasterEmeraldLevelUpMoves[] = {",
    ])
    lines.extend(
        f"    {{{move_id},{level}}},"
        for level, move_id in level_up_moves
    )

    lines.extend([
        "};",
        "",
        "static const uint16_t kRemasterEmeraldLevelUpMoveSlices[][2] = {",
    ])
    lines.extend(
        f"    {{{start},{count}}},"
        for start, count in level_up_slices
    )
    lines.extend(["};", ""])
    return "\n".join(lines)


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: build_r11_domain_catalog.py <vendor-root> <output.inc> <report.json>",
            file=sys.stderr,
        )
        return 2

    vendor = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()
    report_path = Path(sys.argv[3]).resolve()

    try:
        resolver = DefineResolver(vendor / "include")
        species, species_missing = parse_species(vendor, resolver)
        moves, moves_missing = parse_moves(vendor, resolver)
        items, items_missing = parse_items(vendor, resolver)
        evolutions = parse_evolutions(vendor, resolver, len(species))
        level_up_moves, level_up_slices, populated_learnsets = (
            parse_level_up_learnsets(vendor, resolver, len(species))
        )

        if species_missing or moves_missing or items_missing:
            raise CatalogError(
                f"missing source rows species={species_missing[:10]} "
                f"moves={moves_missing[:10]} items={items_missing[:10]}"
            )

        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(
            render(
                species,
                moves,
                items,
                evolutions,
                level_up_moves,
                level_up_slices,
            ),
            encoding="utf-8",
            newline="\n",
        )

        report = {
            "species_internal_count": len(species),
            "move_count": len(moves),
            "item_count": len(items),
            "item_name_sort_rank_count": len(items),
            "evolution_slots_per_species": 5,
            "level_up_move_entry_count": len(level_up_moves),
            "level_up_learnset_species_count": populated_learnsets,
            "species_entries_missing": species_missing,
            "move_entries_missing": moves_missing,
            "item_entries_missing": items_missing,
            "box_pokemon_bytes": 80,
            "party_pokemon_bytes": 100,
            "party_size": resolver.resolve("PARTY_SIZE"),
            "storage_box_count": resolver.resolve("TOTAL_BOXES_COUNT"),
            "storage_box_capacity": resolver.resolve("IN_BOX_COUNT"),
            "storage_box_payload_offset": 4,
            "storage_box_names_offset": 0x8344,
            "bag_pocket_capacities": {
                "items": resolver.resolve("BAG_ITEMS_COUNT"),
                "key_items": resolver.resolve("BAG_KEYITEMS_COUNT"),
                "poke_balls": resolver.resolve("BAG_POKEBALLS_COUNT"),
                "tm_hm": resolver.resolve("BAG_TMHM_COUNT"),
                "berries": resolver.resolve("BAG_BERRIES_COUNT"),
            },
        }
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
            newline="\n",
        )
    except (CatalogError, KeyError, ValueError, SyntaxError) as exc:
        print(f"R11 domain catalog generation FAILED: {exc}", file=sys.stderr)
        return 1

    print("R11 domain catalog generation PASSED")
    print(f" - species entries: {len(species)}")
    print(f" - move entries: {len(moves)}")
    print(f" - item entries: {len(items)}")
    print(f" - level-up move entries: {len(level_up_moves)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
