#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

MART_NAMES = [
    "FallarborTown_Mart",
    "FortreeCity_Mart",
    "LavaridgeTown_Mart",
    "MauvilleCity_Mart",
    "MossdeepCity_Mart",
    "OldaleTown_Mart",
    "PetalburgCity_Mart",
    "RustboroCity_Mart",
    "SlateportCity_Mart",
    "SootopolisCity_Mart",
    "VerdanturfTown_Mart",
]

PHASE_ITEMS = [
    "ITEM_FIRE_STONE",
    "ITEM_WATER_STONE",
    "ITEM_THUNDER_STONE",
    "ITEM_LEAF_STONE",
    "ITEM_MOON_STONE",
    "ITEM_SUN_STONE",
    "ITEM_PP_MAX",
]

DIRECT_TRADE_ITEMS = [
    "ITEM_KINGS_ROCK",
    "ITEM_METAL_COAT",
    "ITEM_DRAGON_SCALE",
    "ITEM_UP_GRADE",
    "ITEM_DEEP_SEA_TOOTH",
    "ITEM_DEEP_SEA_SCALE",
]


def update_makefile() -> None:
    path = ROOT / "Makefile"
    text = path.read_text(encoding="utf-8")

    old = "TITLE       := ZUMRUT VP001\nGAME_CODE   := BPEE"
    new = """VANILLAPLUS_TEST ?= 0

ifeq ($(VANILLAPLUS_TEST),1)
TITLE       := ZUMRUT T002
else
TITLE       := ZUMRUT VP002
endif
GAME_CODE   := BPEE"""
    if old in text:
        text = text.replace(old, new, 1)
    elif "TITLE       := ZUMRUT VP002" not in text:
        raise SystemExit("Unexpected Makefile TITLE block")

    cpp_old = "CPPFLAGS := -iquote include -iquote $(GFLIB_SUBDIR) -Wno-trigraphs -DMODERN=$(MODERN)"
    cpp_new = cpp_old + " -DVANILLAPLUS_TEST=$(VANILLAPLUS_TEST)"
    if "-DVANILLAPLUS_TEST=$(VANILLAPLUS_TEST)" not in text:
        if cpp_old not in text:
            raise SystemExit("Unexpected Makefile CPPFLAGS")
        text = text.replace(cpp_old, cpp_new, 1)

    path.write_text(text, encoding="utf-8")


def update_strings() -> None:
    path = ROOT / "src/strings.c"
    text = path.read_text(encoding="utf-8")
    if "OYUNCU V+001" in text:
        text = text.replace("OYUNCU V+001", "OYUNCU V+002", 1)
    elif "OYUNCU V+002" not in text:
        raise SystemExit("Unexpected Continue build marker")
    path.write_text(text, encoding="utf-8")


def update_marts() -> None:
    inventory_re = re.compile(
        r"(?ms)(^[A-Za-z0-9_]+_Pokemart(?:_[A-Za-z0-9_]+)?:\s*\n)"
        r"((?:\s*\.2byte\s+ITEM_[A-Z0-9_]+\s*\n)+?)"
        r"(^\s*\.2byte\s+ITEM_NONE\s*$)"
    )

    for mart in MART_NAMES:
        path = ROOT / f"data/maps/{mart}/scripts.inc"
        text = path.read_text(encoding="utf-8")

        def add_stock(match: re.Match[str]) -> str:
            prefix, body, end = match.groups()
            missing = [item for item in PHASE_ITEMS if item not in body]
            addition = "".join(f"\t.2byte {item}\n" for item in missing)
            return prefix + body + addition + end

        new_text, count = inventory_re.subn(add_stock, text)
        if count == 0:
            raise SystemExit(f"No Pokemart inventory found in {path}")
        path.write_text(new_text, encoding="utf-8")


def update_trade_items() -> None:
    path = ROOT / "src/data/items.h"
    items = path.read_text(encoding="utf-8")

    for item in DIRECT_TRADE_ITEMS:
        pattern = re.compile(rf"(?ms)(^\s*\[{item}\]\s*=\s*\{{)(.*?)(^\s*\}},)")
        match = pattern.search(items)
        if not match:
            raise SystemExit(f"Item block not found: {item}")

        body = match.group(2)
        if ".type = ITEM_USE_PARTY_MENU" not in body:
            if ".type = ITEM_USE_BAG_MENU" not in body:
                raise SystemExit(f"Unexpected type for {item}")
            body = body.replace(".type = ITEM_USE_BAG_MENU", ".type = ITEM_USE_PARTY_MENU", 1)

        if ".fieldUseFunc = ItemUseOutOfBattle_EvolutionStone" not in body:
            if ".fieldUseFunc = ItemUseOutOfBattle_CannotUse" not in body:
                raise SystemExit(f"Unexpected field use func for {item}")
            body = body.replace(
                ".fieldUseFunc = ItemUseOutOfBattle_CannotUse",
                ".fieldUseFunc = ItemUseOutOfBattle_EvolutionStone",
                1,
            )

        items = items[: match.start(2)] + body + items[match.end(2) :]

    path.write_text(items, encoding="utf-8")


def update_reusable_stones() -> None:
    path = ROOT / "src/party_menu.c"
    party = path.read_text(encoding="utf-8")

    helper = """static bool8 IsReusableEvolutionStone(u16 itemId)
{
    switch (itemId)
    {
    case ITEM_FIRE_STONE:
    case ITEM_WATER_STONE:
    case ITEM_THUNDER_STONE:
    case ITEM_LEAF_STONE:
    case ITEM_MOON_STONE:
    case ITEM_SUN_STONE:
        return TRUE;
    default:
        return FALSE;
    }
}

"""
    marker = "void ItemUseCB_EvolutionStone(u8 taskId, TaskFunc task)"
    if "static bool8 IsReusableEvolutionStone(u16 itemId)" not in party:
        if marker not in party:
            raise SystemExit("Evolution stone callback not found")
        party = party.replace(marker, helper + marker, 1)

    start = party.index(marker)
    remove = "RemoveBagItem(gSpecialVar_ItemId, 1);"
    guard = "if (!IsReusableEvolutionStone(gSpecialVar_ItemId))\n            RemoveBagItem(gSpecialVar_ItemId, 1);"
    if guard not in party[start:]:
        pos = party.find(remove, start)
        if pos < 0:
            raise SystemExit("Evolution stone bag-removal call not found")
        party = party[:pos] + guard + party[pos + len(remove):]

    path.write_text(party, encoding="utf-8")


def main() -> None:
    update_makefile()
    update_strings()
    update_marts()
    update_trade_items()
    update_reusable_stones()
    print("Phase 0-1 source changes applied")


if __name__ == "__main__":
    main()
