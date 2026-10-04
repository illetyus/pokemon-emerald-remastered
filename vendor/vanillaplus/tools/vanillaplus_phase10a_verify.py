#!/usr/bin/env python3
"""Static source checks for Vanilla+ Phase 10A quest tracking/navigation."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

def read(path):
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing required file: {path}")
        return ""
    return p.read_text(encoding="utf-8")

def require(source, token, label):
    if token not in source:
        errors.append(f"{label}: missing {token}")

quest_h = read("include/quest_system.h")
quest = read("src/quest_system.c")
start = read("src/start_menu.c")
menu_c = read("src/menu.c")
pokenav_h = read("include/pokenav.h")
pokenav = read("src/pokenav.c")
pokenav_map = read("src/pokenav_region_map.c")
region_h = read("include/region_map.h")
region = read("src/region_map.c")
strings = read("src/strings.c")
makefile = read("Makefile")
global_h = read("include/global.h")
storage_h = read("include/pokemon_storage_system.h")
ld_script = read("ld_script.txt")
workflow = read(".github/workflows/build.yml")
rusturf_map = read("data/maps/RusturfTunnel/map.json")
steven_map = read("data/maps/GraniteCave_StevensRoom/map.json")

# Save compatibility: active quest state is derived, never persisted.
for forbidden in ["currentQuest", "questState", "questObjective", "activeQuest"]:
    if forbidden.lower() in global_h.lower() or forbidden.lower() in storage_h.lower():
        errors.append(f"save-layout guard: unexpected persisted quest field {forbidden}")

require(quest_h, "struct QuestObjective", "quest data model")
require(quest_h, "QUEST_TARGET_OBJECT_EVENT", "local target type")
require(quest_h, "QUEST_CONDITION_FLAG_SET", "flag condition")
require(quest_h, "QUEST_CONDITION_VAR_GE", "var condition")
require(quest, "Quest_GetActiveObjective(void)", "derived objective resolver")
require(quest, "FLAG_DEFEATED_RIVAL_ROUTE103", "early-story inference")
require(quest, "FLAG_DEVON_GOODS_STOLEN", "Devon story inference")
require(quest, "FLAG_RECOVERED_DEVON_GOODS", "Devon recovery inference")
require(
    quest,
    "QC_FLAG_CLEAR(FLAG_RECOVERED_DEVON_GOODS)",
    "Devon recovered-state reactivation guard",
)
require(quest, "FLAG_DELIVERED_STEVEN_LETTER", "Steven story inference")
require(quest, "MAPSEC_RUSTURF_TUNNEL", "Rusturf navigation")
require(quest, "MAPSEC_GRANITE_CAVE", "Granite Cave navigation")
for token in [
    "MAP_GROUP(RUSTBORO_CITY_GYM)",
    "MAP_GROUP(DEWFORD_TOWN_GYM)",
    "MAP_GROUP(MAUVILLE_CITY_GYM)",
    "MAP_GROUP(LAVARIDGE_TOWN_GYM_1F)",
    "MAP_GROUP(PETALBURG_CITY_GYM)",
    "MAP_GROUP(FORTREE_CITY_GYM)",
    "MAP_GROUP(MOSSDEEP_CITY_GYM)",
    "MAP_GROUP(SOOTOPOLIS_CITY_GYM_1F)",
    "MAP_GROUP(SLATEPORT_CITY_OCEANIC_MUSEUM_2F)",
    "MAP_GROUP(MOSSDEEP_CITY_STEVENS_HOUSE)",
    "MAP_GROUP(MT_PYRE_SUMMIT)",
    ".localId = 31",
    "LoadObjectEventPalette(0x1100)",
]:
    require(quest, token, "key local objective marker coverage")

# Full main-story coverage through the Hall of Fame.
for token in [
    "FLAG_BADGE02_GET",
    "FLAG_BADGE03_GET",
    "FLAG_MET_ARCHIE_METEOR_FALLS",
    "FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY",
    "VAR_WEATHER_INSTITUTE_STATE",
    "FLAG_RECEIVED_DEVON_SCOPE",
    "FLAG_RECEIVED_RED_OR_BLUE_ORB",
    "FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT",
    "FLAG_MET_TEAM_AQUA_HARBOR",
    "FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE",
    "VAR_MOSSDEEP_SPACE_CENTER_STATE",
    "VAR_STEVENS_HOUSE_STATE",
    "FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN",
    "VAR_SOOTOPOLIS_CITY_STATE",
    "FLAG_BADGE08_GET",
    "FLAG_LANDMARK_POKEMON_LEAGUE",
    "FLAG_SYS_GAME_CLEAR",
    "MAPSEC_MT_PYRE",
    "MAPSEC_MAGMA_HIDEOUT",
    "MAPSEC_AQUA_HIDEOUT",
    "MAPSEC_SKY_PILLAR",
    "MAPSEC_VICTORY_ROAD",
    "MAPSEC_EVER_GRANDE_CITY",
]:
    require(quest, token, "full main-story objective coverage")

objective_count = quest.count(".id = QUESTOBJ_")
if objective_count < 32:
    errors.append(f"main-story coverage: expected at least 32 objectives, found {objective_count}")

# Known local targets must still correspond to their canonical object maps.
require(rusturf_map, '"script": "RusturfTunnel_EventScript_Grunt"', "Rusturf grunt target")
require(steven_map, '"script": "GraniteCave_StevensRoom_EventScript_Steven"', "Steven target")

# Start menu/modal quest UI.
require(strings, 'gText_MenuQuest[] = _("GÖREV")', "quest menu label")
require(start, "MENU_ACTION_QUEST", "quest Start-menu action")
require(start, "StartMenuQuestCallback", "quest menu callback")
require(start, "HandleQuestMenuInput", "quest modal input")
require(start, "Quest_GetActiveObjective()", "quest modal resolver")
require(start, "CB2_InitPokeNavQuestMap", "quest map launch")
require(start, "sCurrentStartMenuActions[9]", "Start-menu capacity")
require(menu_c, "u8 top = (numActions >= 9) ? 0 : 1;", "nine-item Start menu screen fit")

# Local marker is transient and follows the active ObjectEvent only on its target map.
require(quest_h, "Quest_UpdateLocalMarker", "local marker API")
require(quest, "sQuestMarkerGfx", "quest marker graphic")
require(quest, "TryGetObjectEventIdByLocalIdAndMap", "ObjectEvent target lookup")
require(quest, "CreateSpriteAtEnd(&sQuestMarkerSpriteTemplate", "local marker creation")
require(quest, "DestroySprite(&gSprites", "local marker cleanup")
overworld = read("src/overworld.c")
require(overworld, "Quest_UpdateLocalMarker();", "overworld local marker update")

# PokeNav direct map mode and objective focus.
require(pokenav_h, "POKENAV_MODE_QUEST_MAP", "PokeNav quest mode")
require(pokenav_h, "CB2_InitPokeNavQuestMap", "PokeNav quest entry")
require(pokenav, "SetActivePokenavMenu(POKENAV_REGION_MAP)", "direct Region Map open")
require(pokenav, "mode == POKENAV_MODE_QUEST_MAP", "quest-specific return handling")
require(pokenav_map, "SetRegionMapCursorToMapSec(Quest_GetActiveMapSecId())", "active objective map focus")
require(pokenav_map, "GetPokenavMode() == POKENAV_MODE_QUEST_MAP", "quest map behavior")
require(region_h, "SetRegionMapCursorToMapSec", "Region Map targeting API")
require(region, "void SetRegionMapCursorToMapSec(u16 mapSecId)", "Region Map target implementation")
require(region, "gRegionMapEntries[mapSecId]", "canonical Region Map coordinates")
require(region_h, "CreateRegionMapQuestMarker", "Region Map marker API")
require(region, "SpriteCB_QuestMapMarker", "persistent Region Map marker callback")
require(region, "FLYDESTICON_RED_OUTLINE", "Region Map quest target visual")
require(pokenav_map, "questMarkerSprite", "PokeNav quest marker ownership")
require(pokenav_map, "CreateRegionMapQuestMarker(Quest_GetActiveMapSecId())", "PokeNav persistent quest marker creation")
require(pokenav_map, "FreeRegionMapQuestMarker(state->questMarkerSprite)", "PokeNav quest marker cleanup")

# Ordered linker registration.
require(ld_script, "src/quest_system.o(.text);", "quest text linker entry")
require(ld_script, "src/quest_system.o(.rodata);", "quest rodata linker entry")

# Completed phases are version-agnostic but still require synchronized markers.
release_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT VP(\d{3})\s*$", makefile)
test_match = re.search(r"(?m)^\s*TITLE\s*:=\s*ZUMRUT T(\d{3})\s*$", makefile)
continue_match = re.search(r'gText_ContinueMenuPlayer\[\]\s*=\s*_\("OYUNCU V\+(\d{3})"\)', strings)
if not release_match or not test_match or not continue_match:
    errors.append("Phase 10A synchronized build marker structure is missing")
elif len({release_match.group(1), test_match.group(1), continue_match.group(1)}) != 1:
    errors.append("Phase 10A release/test/Continue build markers must stay synchronized")
require(workflow, "python3 tools/vanillaplus_phase10a_verify.py", "permanent CI Phase 10A verifier")

if errors:
    print("Vanilla+ Phase 10A verification FAILED:")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("Vanilla+ Phase 10A verification PASSED")
print(f" - main-story objectives: {objective_count}")
print(" - quest progress is derived from existing flags/vars")
print(" - Start-menu quest UI and direct Region Map targeting are wired")
print(" - SaveBlock/PokemonStorage persistence remains untouched")
