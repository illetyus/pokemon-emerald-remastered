#!/usr/bin/env python3
"""Generate the portable R13 trainer catalog from pinned Vanilla+ data."""

from __future__ import annotations

import ast
import json
from pathlib import Path
import re
import sys
from typing import Any


class CatalogError(RuntimeError):
    pass


class Resolver:
    def __init__(self) -> None:
        self.expr: dict[str, str] = {"TRUE": "1", "FALSE": "0"}
        self.cache: dict[str, int] = {}

    def add_text(self, text: str) -> None:
        text = re.sub(r"\\\r?\n", " ", text)
        for raw in text.splitlines():
            raw = re.sub(r"//.*$", "", raw).strip()
            match = re.match(r"#define\s+([A-Za-z_]\w*)\s+(.+)$", raw)
            if not match:
                continue
            name, expression = match.groups()
            if "(" in name:
                continue
            self.expr[name] = expression.strip()

    def eval(self, expression: str, stack: tuple[str, ...] = ()) -> int:
        expression = expression.strip().rstrip(",")
        expression = re.sub(r"/\*.*?\*/", "", expression)
        expression = re.sub(
            r"\b(0x[0-9A-Fa-f]+|\d+)[uUlL]+\b", r"\1", expression
        )
        expression = re.sub(r"\((?:u|s)?(?:8|16|32|64)\)", "", expression)

        if re.fullmatch(r"[A-Za-z_]\w*", expression):
            if expression in self.cache:
                return self.cache[expression]
            if expression in stack:
                raise CatalogError(f"recursive define: {expression}")
            if expression not in self.expr:
                raise CatalogError(f"unresolved constant: {expression}")
            value = self.eval(self.expr[expression], stack + (expression,))
            self.cache[expression] = value
            return value

        for identifier in sorted(set(re.findall(r"\b[A-Za-z_]\w*\b", expression))):
            value = self.eval(identifier, stack)
            expression = re.sub(
                rf"\b{re.escape(identifier)}\b", str(value), expression
            )

        return self._eval_ast(ast.parse(expression, mode="eval").body)

    def _eval_ast(self, node: ast.AST) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
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
        raise CatalogError(f"unsupported expression: {ast.dump(node)}")


PARTY_FLAGS = {
    "TrainerMonNoItemDefaultMoves": 0,
    "TrainerMonNoItemCustomMoves": 1,
    "TrainerMonItemDefaultMoves": 2,
    "TrainerMonItemCustomMoves": 3,
}

PARTY_MACROS = {
    "NO_ITEM_DEFAULT_MOVES": 0,
    "NO_ITEM_CUSTOM_MOVES": 1,
    "ITEM_DEFAULT_MOVES": 2,
    "ITEM_CUSTOM_MOVES": 3,
}


def braced_block(text: str, open_brace: int) -> tuple[str, int]:
    depth = 0
    for index in range(open_brace, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace + 1:index], index + 1
    raise CatalogError("unclosed brace block")


def top_level_blocks(text: str) -> list[str]:
    result: list[str] = []
    depth = 0
    start = -1
    for index, char in enumerate(text):
        if char == "{":
            if depth == 0:
                start = index
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0 and start >= 0:
                result.append(text[start + 1:index])
                start = -1
            if depth < 0:
                raise CatalogError("unexpected closing brace")
    if depth != 0:
        raise CatalogError("unclosed nested block")
    return result


def field(block: str, name: str) -> str:
    match = re.search(
        rf"\.{re.escape(name)}\s*=\s*([^,\n}}]+)", block
    )
    if not match:
        raise CatalogError(f"missing field {name}")
    return match.group(1).strip()


def parse_parties(vendor: Path, resolver: Resolver) -> dict[str, dict[str, Any]]:
    text = (vendor / "src/data/trainer_parties.h").read_text(encoding="utf-8")
    parties: dict[str, dict[str, Any]] = {}

    pattern = re.compile(
        r"static const struct\s+(TrainerMon\w+)\s+"
        r"(sParty_[A-Za-z0-9_]+)\[\]\s*=\s*\{"
    )
    for match in pattern.finditer(text):
        struct_name, party_name = match.groups()
        body, _ = braced_block(text, match.end() - 1)
        if struct_name not in PARTY_FLAGS:
            raise CatalogError(f"unknown party struct: {struct_name}")

        mons: list[dict[str, Any]] = []
        for mon_block in top_level_blocks(body):
            moves_match = re.search(r"\.moves\s*=\s*\{([^}]*)\}", mon_block)
            moves = [0, 0, 0, 0]
            if moves_match:
                tokens = [
                    token.strip()
                    for token in moves_match.group(1).split(",")
                    if token.strip()
                ]
                if len(tokens) > 4:
                    raise CatalogError(f"too many moves in {party_name}")
                for index, token in enumerate(tokens):
                    moves[index] = resolver.eval(token)

            held_match = re.search(
                r"\.heldItem\s*=\s*([^,\n}]+)", mon_block
            )
            mons.append(
                {
                    "iv": resolver.eval(field(mon_block, "iv")),
                    "level": resolver.eval(field(mon_block, "lvl")),
                    "species": resolver.eval(field(mon_block, "species")),
                    "held_item": (
                        resolver.eval(held_match.group(1).strip())
                        if held_match
                        else 0
                    ),
                    "moves": moves,
                }
            )

        if not mons or len(mons) > 6:
            raise CatalogError(f"invalid party size for {party_name}: {len(mons)}")
        parties[party_name] = {
            "flags": PARTY_FLAGS[struct_name],
            "struct": struct_name,
            "mons": mons,
        }

    return parties


def parse_items(block: str, resolver: Resolver) -> list[int]:
    match = re.search(r"\.items\s*=\s*\{([^}]*)\}", block)
    if not match:
        raise CatalogError("missing trainer items")
    tokens = [
        token.strip()
        for token in match.group(1).split(",")
        if token.strip()
    ]
    if len(tokens) > 4:
        raise CatalogError("trainer has more than four items")
    values = [resolver.eval(token) for token in tokens]
    values.extend([0] * (4 - len(values)))
    return values


def parse_trainers(
    vendor: Path,
    resolver: Resolver,
    parties: dict[str, dict[str, Any]],
) -> list[dict[str, Any]]:
    text = (vendor / "src/data/trainers.h").read_text(encoding="utf-8")
    parsed: dict[int, dict[str, Any]] = {}
    pattern = re.compile(r"\[(TRAINER_[A-Z0-9_]+)\]\s*=\s*\{")

    for match in pattern.finditer(text):
        symbol = match.group(1)
        trainer_id = resolver.eval(symbol)
        body, _ = braced_block(text, match.end() - 1)

        name_match = re.search(
            r'\.trainerName\s*=\s*_\("((?:\\.|[^"\\])*)"\)', body
        )
        if not name_match:
            raise CatalogError(f"missing trainerName for {symbol}")

        if symbol == "TRAINER_NONE":
            party_name = ""
            party_flags = resolver.eval(field(body, "partyFlags"))
            party_mons: list[dict[str, Any]] = []
        else:
            party_match = re.search(
                r"\.party\s*=\s*"
                r"(NO_ITEM_DEFAULT_MOVES|NO_ITEM_CUSTOM_MOVES|"
                r"ITEM_DEFAULT_MOVES|ITEM_CUSTOM_MOVES)"
                r"\((sParty_[A-Za-z0-9_]+)\)",
                body,
            )
            if not party_match:
                raise CatalogError(f"missing portable party macro for {symbol}")
            macro, party_name = party_match.groups()
            if party_name not in parties:
                raise CatalogError(f"missing party array {party_name}")
            party_flags = PARTY_MACROS[macro]
            if parties[party_name]["flags"] != party_flags:
                raise CatalogError(f"party flag mismatch for {symbol}")
            party_mons = parties[party_name]["mons"]

        parsed[trainer_id] = {
            "symbol": symbol,
            "trainer_id": trainer_id,
            "trainer_class": resolver.eval(field(body, "trainerClass")),
            "encounter_music_gender": resolver.eval(
                field(body, "encounterMusic_gender")
            ),
            "trainer_pic": resolver.eval(field(body, "trainerPic")),
            "trainer_name": name_match.group(1),
            "items": parse_items(body, resolver),
            "double_battle": resolver.eval(field(body, "doubleBattle")),
            "ai_flags": resolver.eval(field(body, "aiFlags")),
            "party_size": len(party_mons),
            "party_flags": party_flags,
            "party_name": party_name,
            "party": party_mons,
        }

    if not parsed:
        raise CatalogError("no trainer entries found")

    max_id = max(parsed)
    missing = [trainer_id for trainer_id in range(max_id + 1) if trainer_id not in parsed]
    if missing:
        raise CatalogError(f"trainer ID gaps: {missing[:16]}")
    return [parsed[index] for index in range(max_id + 1)]


def render_mon(mon: dict[str, Any]) -> str:
    moves = ",".join(str(value) for value in mon["moves"])
    return (
        "{"
        f"{mon['species']},{mon['held_item']},{{{moves}}},"
        f"{mon['iv']},{mon['level']}"
        "}"
    )


def render_trainers(trainers: list[dict[str, Any]]) -> str:
    lines = [
        "/* AUTO-GENERATED by tools/build_r13_trainer_catalog.py. */",
        "/* Source: pinned vendor/vanillaplus trainer data. */",
        "",
        "static const RemasterEmeraldTrainer kRemasterEmeraldTrainers[] = {",
    ]
    zero_mon = "{0,0,{0,0,0,0},0,0}"
    for trainer in trainers:
        party = [render_mon(mon) for mon in trainer["party"]]
        party.extend([zero_mon] * (6 - len(party)))
        items = ",".join(str(value) for value in trainer["items"])
        name = trainer["trainer_name"]
        lines.append(f"    /* {trainer['symbol']} */")
        lines.append(
            "    {"
            f"{trainer['trainer_id']},"
            f"{trainer['trainer_class']},"
            f"{trainer['encounter_music_gender']},"
            f"{trainer['trainer_pic']},"
            f'"{name}",'
            f"{{{items}}},"
            f"{trainer['double_battle']},"
            f"{trainer['ai_flags']}u,"
            f"{trainer['party_size']},"
            f"{trainer['party_flags']},"
            "{" + ",".join(party) + "}"
            "},"
        )
    lines.extend(["};", ""])
    return "\n".join(lines)


def build_resolver(vendor: Path) -> Resolver:
    resolver = Resolver()
    for rel in (
        "include/constants/opponents.h",
        "include/constants/trainers.h",
        "include/constants/items.h",
        "include/constants/species.h",
        "include/constants/moves.h",
        "include/constants/battle_ai.h",
    ):
        resolver.add_text((vendor / rel).read_text(encoding="utf-8"))
    return resolver


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: build_r13_trainer_catalog.py "
            "<vendor> <catalog.inc> <report.json>",
            file=sys.stderr,
        )
        return 2

    vendor = Path(sys.argv[1])
    catalog_path = Path(sys.argv[2])
    report_path = Path(sys.argv[3])
    resolver = build_resolver(vendor)
    parties = parse_parties(vendor, resolver)
    trainers = parse_trainers(vendor, resolver, parties)

    variant_counts = {
        key: sum(1 for party in parties.values() if party["struct"] == key)
        for key in PARTY_FLAGS
    }
    report = {
        "trainer_entry_count": len(trainers),
        "max_trainer_id": trainers[-1]["trainer_id"],
        "party_array_count": len(parties),
        "party_variant_counts": variant_counts,
        "double_battle_count": sum(t["double_battle"] != 0 for t in trainers),
        "trainers_with_items": sum(any(t["items"]) for t in trainers),
        "trainers_with_custom_moves": sum(
            bool(t["party_flags"] & 1) for t in trainers
        ),
        "trainers_with_held_items": sum(
            bool(t["party_flags"] & 2) for t in trainers
        ),
    }

    catalog_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    catalog_path.write_text(render_trainers(trainers), encoding="utf-8")
    report_path.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except CatalogError as exc:
        print(f"trainer catalog error: {exc}", file=sys.stderr)
        raise SystemExit(1)
