#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "sprite.h"
#include "quest_system.h"
#include "constants/flags.h"
#include "constants/map_groups.h"
#include "constants/region_map_sections.h"
#include "constants/vars.h"

#define QC_FLAG_SET(flag)   {QUEST_CONDITION_FLAG_SET, (flag), 0}
#define QC_FLAG_CLEAR(flag) {QUEST_CONDITION_FLAG_CLEAR, (flag), 0}
#define QC_VAR_EQ(var, val) {QUEST_CONDITION_VAR_EQ, (var), (val)}
#define QC_VAR_NE(var, val) {QUEST_CONDITION_VAR_NE, (var), (val)}
#define QC_VAR_GE(var, val) {QUEST_CONDITION_VAR_GE, (var), (val)}
#define QC_VAR_LT(var, val) {QUEST_CONDITION_VAR_LT, (var), (val)}


static const u8 sQuestMarkerGfx[] = INCBIN_U8("graphics/field_effects/pics/emotion_exclamation.4bpp");

static const struct OamData sQuestMarkerOam =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteFrameImage sQuestMarkerImages[] =
{
    {
        .data = sQuestMarkerGfx,
        .size = 0x80
    }
};

static const union AnimCmd sQuestMarkerAnim[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sQuestMarkerAnims[] =
{
    sQuestMarkerAnim
};

static void SpriteCB_QuestMarker(struct Sprite *sprite)
{
}

static const struct SpriteTemplate sQuestMarkerSpriteTemplate =
{
    .tileTag = TAG_NONE,
    .paletteTag = 0x1100,
    .oam = &sQuestMarkerOam,
    .anims = sQuestMarkerAnims,
    .images = sQuestMarkerImages,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_QuestMarker
};

static u8 Quest_FindLocalMarkerSprite(void)
{
    u8 i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].callback == SpriteCB_QuestMarker)
            return i;
    }

    return MAX_SPRITES;
}

enum
{
    QUESTOBJ_MEET_RIVAL_ROUTE103 = 1,
    QUESTOBJ_RETURN_TO_BIRCH,
    QUESTOBJ_VISIT_PETALBURG_GYM,
    QUESTOBJ_CHALLENGE_ROXANNE,
    QUESTOBJ_FIND_DEVON_THIEF,
    QUESTOBJ_RECOVER_DEVON_GOODS,
    QUESTOBJ_RETURN_DEVON_GOODS,
    QUESTOBJ_DELIVER_STEVEN_LETTER,
    QUESTOBJ_DELIVER_DEVON_PACKAGE,
    QUESTOBJ_CHALLENGE_BRAWLY,
    QUESTOBJ_CHALLENGE_WATTSON,
    QUESTOBJ_GO_METEOR_FALLS,
    QUESTOBJ_STOP_MT_CHIMNEY,
    QUESTOBJ_CHALLENGE_FLANNERY,
    QUESTOBJ_CHALLENGE_NORMAN,
    QUESTOBJ_CLEAR_WEATHER_INSTITUTE,
    QUESTOBJ_GET_DEVON_SCOPE,
    QUESTOBJ_CHALLENGE_WINONA,
    QUESTOBJ_CLIMB_MT_PYRE,
    QUESTOBJ_ENTER_MAGMA_HIDEOUT,
    QUESTOBJ_GO_SLATEPORT_HARBOR,
    QUESTOBJ_CLEAR_AQUA_HIDEOUT,
    QUESTOBJ_CHALLENGE_TATE_LIZA,
    QUESTOBJ_DEFEND_SPACE_CENTER,
    QUESTOBJ_GET_DIVE_FROM_STEVEN,
    QUESTOBJ_ENTER_SEAFLOOR_CAVERN,
    QUESTOBJ_GO_SOOTOPOLIS,
    QUESTOBJ_WAKE_RAYQUAZA,
    QUESTOBJ_RETURN_SOOTOPOLIS,
    QUESTOBJ_CHALLENGE_JUAN,
    QUESTOBJ_CROSS_VICTORY_ROAD,
    QUESTOBJ_CHALLENGE_POKEMON_LEAGUE,
};

static const u8 sQuestTitleMeetRival[] = _("RAKİBİNİ BUL");
static const u8 sQuestDescMeetRival[] = _("Route 103'e git ve rakibinle\nbuluş.");

static const u8 sQuestTitleReturnBirch[] = _("LABORATUVARA DÖN");
static const u8 sQuestDescReturnBirch[] = _("Littleroot'taki Prof. Birch'ün\nlaboratuvarına dön.");

static const u8 sQuestTitlePetalburg[] = _("PETALBURG'A GİT");
static const u8 sQuestDescPetalburg[] = _("Petalburg Şehri'ndeki Spor\nSalonu'nda Norman'ı bul.");

static const u8 sQuestTitleRoxanne[] = _("İLK ROZET");
static const u8 sQuestDescRoxanne[] = _("Rustboro Spor Salonu'nda\nRoxanne'a meydan oku.");

static const u8 sQuestTitleFindThief[] = _("DEVON MALLARI");
static const u8 sQuestDescFindThief[] = _("Devon çalışanını takip ederek\nRoute 116'ya ilerle.");

static const u8 sQuestTitleRecoverGoods[] = _("HIRSIZI YAKALA");
static const u8 sQuestDescRecoverGoods[] = _("Rusturf Tüneli'nde çalınan\nDevon Mallarını geri al.");

static const u8 sQuestTitleReturnGoods[] = _("DEVON'A DÖN");
static const u8 sQuestDescReturnGoods[] = _("Devon Mallarını Rustboro'daki\nDevon Corp'a geri götür.");

static const u8 sQuestTitleStevenLetter[] = _("STEVEN'A MEKTUP");
static const u8 sQuestDescStevenLetter[] = _("Dewford yakınındaki Granit\nMağarası'nda Steven'ı bul.");

static const u8 sQuestTitleDevonPackage[] = _("PAKETİ TESLİM ET");
static const u8 sQuestDescDevonPackage[] = _("Slateport'ta Kaptan Stern'e\nDevon paketini teslim et.");

static const u8 sQuestTitleBrawly[] = _("BRAWLY'Yİ YEN");
static const u8 sQuestDescBrawly[] = _("Dewford Spor Salonu'nda\nBrawly'ye meydan oku.");

static const u8 sQuestTitleWattson[] = _("WATTSON'I YEN");
static const u8 sQuestDescWattson[] = _("Mauville Spor Salonu'nda\nWattson'a meydan oku.");

static const u8 sQuestTitleMeteorFalls[] = _("METEOR FALLS");
static const u8 sQuestDescMeteorFalls[] = _("Route 114 üzerinden Meteor\nFalls'a ilerle.");

static const u8 sQuestTitleMtChimney[] = _("MT. CHIMNEY");
static const u8 sQuestDescMtChimney[] = _("Teleferikle Mt. Chimney'e çık\nve Team Magma'yı durdur.");

static const u8 sQuestTitleFlannery[] = _("FLANNERY'Yİ YEN");
static const u8 sQuestDescFlannery[] = _("Lavaridge Spor Salonu'nda\nFlannery'ye meydan oku.");

static const u8 sQuestTitleNorman[] = _("NORMAN'I YEN");
static const u8 sQuestDescNorman[] = _("Petalburg Spor Salonu'na dön\nve Norman'a meydan oku.");

static const u8 sQuestTitleWeatherInstitute[] = _("HAVA ENSTİTÜSÜ");
static const u8 sQuestDescWeatherInstitute[] = _("Route 119'daki Hava\nEnstitüsü'nü Team Aqua'dan kurtar.");

static const u8 sQuestTitleDevonScope[] = _("STEVEN'I BUL");
static const u8 sQuestDescDevonScope[] = _("Route 120'de Steven'la buluş\nve görünmez engeli çöz.");

static const u8 sQuestTitleWinona[] = _("WINONA'YI YEN");
static const u8 sQuestDescWinona[] = _("Fortree Spor Salonu'nda\nWinona'ya meydan oku.");

static const u8 sQuestTitleMtPyre[] = _("MT. PYRE'A GİT");
static const u8 sQuestDescMtPyre[] = _("Route 122'den Mt. Pyre'ın\nzirvesine ilerle.");

static const u8 sQuestTitleMagmaHideout[] = _("MAGMA ÜSSÜ");
static const u8 sQuestDescMagmaHideout[] = _("Jagged Pass'teki Team Magma\nüssünü bul ve içeri gir.");

static const u8 sQuestTitleSlateportHarbor[] = _("SLATEPORT LİMANI");
static const u8 sQuestDescSlateportHarbor[] = _("Slateport Limanı'nda Kaptan\nStern'in yanına git.");

static const u8 sQuestTitleAquaHideout[] = _("AQUA ÜSSÜ");
static const u8 sQuestDescAquaHideout[] = _("Lilycove'daki Team Aqua\nüssüne gir.");

static const u8 sQuestTitleTateLiza[] = _("TATE & LIZA");
static const u8 sQuestDescTateLiza[] = _("Mossdeep Spor Salonu'nda\nTate ve Liza'ya meydan oku.");

static const u8 sQuestTitleSpaceCenter[] = _("UZAY MERKEZİ");
static const u8 sQuestDescSpaceCenter[] = _("Mossdeep Uzay Merkezi'ne git\nve Team Magma'yı durdur.");

static const u8 sQuestTitleDive[] = _("STEVEN'IN EVİ");
static const u8 sQuestDescDive[] = _("Mossdeep'te Steven'ın evine\ngit ve onunla konuş.");

static const u8 sQuestTitleSeafloor[] = _("SEAFLOOR CAVERN");
static const u8 sQuestDescSeafloor[] = _("Route 128 altında Seafloor\nCavern'ı bul ve içeri gir.");

static const u8 sQuestTitleSootopolis[] = _("SOOTOPOLIS");
static const u8 sQuestDescSootopolis[] = _("Sootopolis Şehri'ne git ve\nkrizin kaynağını araştır.");

static const u8 sQuestTitleRayquaza[] = _("SKY PILLAR");
static const u8 sQuestDescRayquaza[] = _("Route 131'deki Sky Pillar'a\ngit ve zirveye ulaş.");

static const u8 sQuestTitleReturnSootopolis[] = _("SOOTOPOLIS'E DÖN");
static const u8 sQuestDescReturnSootopolis[] = _("Rayquaza'dan sonra Sootopolis\nŞehri'ne geri dön.");

static const u8 sQuestTitleJuan[] = _("JUAN'I YEN");
static const u8 sQuestDescJuan[] = _("Sootopolis Spor Salonu'nda\nJuan'a meydan oku.");

static const u8 sQuestTitleVictoryRoad[] = _("VICTORY ROAD");
static const u8 sQuestDescVictoryRoad[] = _("Ever Grande'e ulaş ve Victory\nRoad'u geç.");

static const u8 sQuestTitlePokemonLeague[] = _("POKéMON LEAGUE");
static const u8 sQuestDescPokemonLeague[] = _("Elite Four ve Şampiyon'a\nmeydan okumaya hazırlan.");

static const struct QuestObjective sMainStoryObjectives[] =
{
    {
        .id = QUESTOBJ_MEET_RIVAL_ROUTE103,
        .title = sQuestTitleMeetRival,
        .description = sQuestDescMeetRival,
        .mapSecId = MAPSEC_ROUTE_103,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_SYS_POKEMON_GET), QC_FLAG_CLEAR(FLAG_DEFEATED_RIVAL_ROUTE103)},
        .completion = {QC_FLAG_SET(FLAG_DEFEATED_RIVAL_ROUTE103)},
    },
    {
        .id = QUESTOBJ_RETURN_TO_BIRCH,
        .title = sQuestTitleReturnBirch,
        .description = sQuestDescReturnBirch,
        .mapSecId = MAPSEC_LITTLEROOT_TOWN,
        .targetType = QUEST_TARGET_MAP,
        .mapGroup = MAP_GROUP(LITTLEROOT_TOWN_PROFESSOR_BIRCHS_LAB),
        .mapNum = MAP_NUM(LITTLEROOT_TOWN_PROFESSOR_BIRCHS_LAB),
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_DEFEATED_RIVAL_ROUTE103), QC_FLAG_CLEAR(FLAG_SYS_POKEDEX_GET)},
        .completion = {QC_FLAG_SET(FLAG_SYS_POKEDEX_GET)},
    },
    {
        .id = QUESTOBJ_VISIT_PETALBURG_GYM,
        .title = sQuestTitlePetalburg,
        .description = sQuestDescPetalburg,
        .mapSecId = MAPSEC_PETALBURG_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(PETALBURG_CITY_GYM),
        .mapNum = MAP_NUM(PETALBURG_CITY_GYM),
        .localId = 1,
        .activationCount = 3,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_SYS_POKEDEX_GET), QC_VAR_LT(VAR_PETALBURG_GYM_STATE, 2), QC_FLAG_CLEAR(FLAG_BADGE01_GET)},
        .completion = {QC_VAR_GE(VAR_PETALBURG_GYM_STATE, 2)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_ROXANNE,
        .title = sQuestTitleRoxanne,
        .description = sQuestDescRoxanne,
        .mapSecId = MAPSEC_RUSTBORO_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(RUSTBORO_CITY_GYM),
        .mapNum = MAP_NUM(RUSTBORO_CITY_GYM),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_PETALBURG_GYM_STATE, 2), QC_FLAG_CLEAR(FLAG_BADGE01_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE01_GET)},
    },
    {
        .id = QUESTOBJ_FIND_DEVON_THIEF,
        .title = sQuestTitleFindThief,
        .description = sQuestDescFindThief,
        .mapSecId = MAPSEC_ROUTE_116,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 3,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE01_GET), QC_FLAG_CLEAR(FLAG_DEVON_GOODS_STOLEN), QC_FLAG_CLEAR(FLAG_RECOVERED_DEVON_GOODS)},
        .completion = {QC_FLAG_SET(FLAG_DEVON_GOODS_STOLEN)},
    },
    {
        .id = QUESTOBJ_RECOVER_DEVON_GOODS,
        .title = sQuestTitleRecoverGoods,
        .description = sQuestDescRecoverGoods,
        .mapSecId = MAPSEC_RUSTURF_TUNNEL,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(RUSTURF_TUNNEL),
        .mapNum = MAP_NUM(RUSTURF_TUNNEL),
        .localId = 6,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_DEVON_GOODS_STOLEN), QC_FLAG_CLEAR(FLAG_RECOVERED_DEVON_GOODS)},
        .completion = {QC_FLAG_SET(FLAG_RECOVERED_DEVON_GOODS)},
    },
    {
        .id = QUESTOBJ_RETURN_DEVON_GOODS,
        .title = sQuestTitleReturnGoods,
        .description = sQuestDescReturnGoods,
        .mapSecId = MAPSEC_RUSTBORO_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(RUSTBORO_CITY_DEVON_CORP_1F),
        .mapNum = MAP_NUM(RUSTBORO_CITY_DEVON_CORP_1F),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_RECOVERED_DEVON_GOODS), QC_FLAG_CLEAR(FLAG_RETURNED_DEVON_GOODS)},
        .completion = {QC_FLAG_SET(FLAG_RETURNED_DEVON_GOODS)},
    },
    {
        .id = QUESTOBJ_DELIVER_STEVEN_LETTER,
        .title = sQuestTitleStevenLetter,
        .description = sQuestDescStevenLetter,
        .mapSecId = MAPSEC_GRANITE_CAVE,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(GRANITE_CAVE_STEVENS_ROOM),
        .mapNum = MAP_NUM(GRANITE_CAVE_STEVENS_ROOM),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_RETURNED_DEVON_GOODS), QC_FLAG_CLEAR(FLAG_DELIVERED_STEVEN_LETTER)},
        .completion = {QC_FLAG_SET(FLAG_DELIVERED_STEVEN_LETTER)},
    },
    {
        .id = QUESTOBJ_DELIVER_DEVON_PACKAGE,
        .title = sQuestTitleDevonPackage,
        .description = sQuestDescDevonPackage,
        .mapSecId = MAPSEC_SLATEPORT_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(SLATEPORT_CITY_OCEANIC_MUSEUM_2F),
        .mapNum = MAP_NUM(SLATEPORT_CITY_OCEANIC_MUSEUM_2F),
        .localId = 1,
        .activationCount = 3,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_RETURNED_DEVON_GOODS), QC_FLAG_SET(FLAG_DELIVERED_STEVEN_LETTER), QC_FLAG_CLEAR(FLAG_DELIVERED_DEVON_GOODS)},
        .completion = {QC_FLAG_SET(FLAG_DELIVERED_DEVON_GOODS)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_BRAWLY,
        .title = sQuestTitleBrawly,
        .description = sQuestDescBrawly,
        .mapSecId = MAPSEC_DEWFORD_TOWN,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(DEWFORD_TOWN_GYM),
        .mapNum = MAP_NUM(DEWFORD_TOWN_GYM),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_DELIVERED_DEVON_GOODS), QC_FLAG_CLEAR(FLAG_BADGE02_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE02_GET)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_WATTSON,
        .title = sQuestTitleWattson,
        .description = sQuestDescWattson,
        .mapSecId = MAPSEC_MAUVILLE_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(MAUVILLE_CITY_GYM),
        .mapNum = MAP_NUM(MAUVILLE_CITY_GYM),
        .localId = 1,
        .activationCount = 3,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_DELIVERED_DEVON_GOODS), QC_FLAG_SET(FLAG_BADGE02_GET), QC_FLAG_CLEAR(FLAG_BADGE03_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE03_GET)},
    },
    {
        .id = QUESTOBJ_GO_METEOR_FALLS,
        .title = sQuestTitleMeteorFalls,
        .description = sQuestDescMeteorFalls,
        .mapSecId = MAPSEC_METEOR_FALLS,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE03_GET), QC_FLAG_CLEAR(FLAG_MET_ARCHIE_METEOR_FALLS)},
        .completion = {QC_FLAG_SET(FLAG_MET_ARCHIE_METEOR_FALLS)},
    },
    {
        .id = QUESTOBJ_STOP_MT_CHIMNEY,
        .title = sQuestTitleMtChimney,
        .description = sQuestDescMtChimney,
        .mapSecId = MAPSEC_MT_CHIMNEY,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_MET_ARCHIE_METEOR_FALLS), QC_FLAG_CLEAR(FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY)},
        .completion = {QC_FLAG_SET(FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_FLANNERY,
        .title = sQuestTitleFlannery,
        .description = sQuestDescFlannery,
        .mapSecId = MAPSEC_LAVARIDGE_TOWN,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(LAVARIDGE_TOWN_GYM_1F),
        .mapNum = MAP_NUM(LAVARIDGE_TOWN_GYM_1F),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY), QC_FLAG_CLEAR(FLAG_BADGE04_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE04_GET)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_NORMAN,
        .title = sQuestTitleNorman,
        .description = sQuestDescNorman,
        .mapSecId = MAPSEC_PETALBURG_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(PETALBURG_CITY_GYM),
        .mapNum = MAP_NUM(PETALBURG_CITY_GYM),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE04_GET), QC_FLAG_CLEAR(FLAG_BADGE05_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE05_GET)},
    },
    {
        .id = QUESTOBJ_CLEAR_WEATHER_INSTITUTE,
        .title = sQuestTitleWeatherInstitute,
        .description = sQuestDescWeatherInstitute,
        .mapSecId = MAPSEC_ROUTE_119,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE05_GET), QC_VAR_LT(VAR_WEATHER_INSTITUTE_STATE, 1)},
        .completion = {QC_VAR_GE(VAR_WEATHER_INSTITUTE_STATE, 1)},
    },
    {
        .id = QUESTOBJ_GET_DEVON_SCOPE,
        .title = sQuestTitleDevonScope,
        .description = sQuestDescDevonScope,
        .mapSecId = MAPSEC_ROUTE_120,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(ROUTE120),
        .mapNum = MAP_NUM(ROUTE120),
        .localId = 31,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_WEATHER_INSTITUTE_STATE, 1), QC_FLAG_CLEAR(FLAG_RECEIVED_DEVON_SCOPE)},
        .completion = {QC_FLAG_SET(FLAG_RECEIVED_DEVON_SCOPE)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_WINONA,
        .title = sQuestTitleWinona,
        .description = sQuestDescWinona,
        .mapSecId = MAPSEC_FORTREE_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(FORTREE_CITY_GYM),
        .mapNum = MAP_NUM(FORTREE_CITY_GYM),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_RECEIVED_DEVON_SCOPE), QC_FLAG_CLEAR(FLAG_BADGE06_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE06_GET)},
    },
    {
        .id = QUESTOBJ_CLIMB_MT_PYRE,
        .title = sQuestTitleMtPyre,
        .description = sQuestDescMtPyre,
        .mapSecId = MAPSEC_MT_PYRE,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(MT_PYRE_SUMMIT),
        .mapNum = MAP_NUM(MT_PYRE_SUMMIT),
        .localId = 3,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE06_GET), QC_FLAG_CLEAR(FLAG_RECEIVED_RED_OR_BLUE_ORB)},
        .completion = {QC_FLAG_SET(FLAG_RECEIVED_RED_OR_BLUE_ORB)},
    },
    {
        .id = QUESTOBJ_ENTER_MAGMA_HIDEOUT,
        .title = sQuestTitleMagmaHideout,
        .description = sQuestDescMagmaHideout,
        .mapSecId = MAPSEC_MAGMA_HIDEOUT,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_RECEIVED_RED_OR_BLUE_ORB), QC_FLAG_CLEAR(FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT)},
        .completion = {QC_FLAG_SET(FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT)},
    },
    {
        .id = QUESTOBJ_GO_SLATEPORT_HARBOR,
        .title = sQuestTitleSlateportHarbor,
        .description = sQuestDescSlateportHarbor,
        .mapSecId = MAPSEC_SLATEPORT_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(SLATEPORT_CITY_HARBOR),
        .mapNum = MAP_NUM(SLATEPORT_CITY_HARBOR),
        .localId = 4,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT), QC_FLAG_CLEAR(FLAG_MET_TEAM_AQUA_HARBOR)},
        .completion = {QC_FLAG_SET(FLAG_MET_TEAM_AQUA_HARBOR)},
    },
    {
        .id = QUESTOBJ_CLEAR_AQUA_HIDEOUT,
        .title = sQuestTitleAquaHideout,
        .description = sQuestDescAquaHideout,
        .mapSecId = MAPSEC_AQUA_HIDEOUT,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_MET_TEAM_AQUA_HARBOR), QC_FLAG_CLEAR(FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE)},
        .completion = {QC_FLAG_SET(FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_TATE_LIZA,
        .title = sQuestTitleTateLiza,
        .description = sQuestDescTateLiza,
        .mapSecId = MAPSEC_MOSSDEEP_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(MOSSDEEP_CITY_GYM),
        .mapNum = MAP_NUM(MOSSDEEP_CITY_GYM),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE), QC_FLAG_CLEAR(FLAG_BADGE07_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE07_GET)},
    },
    {
        .id = QUESTOBJ_DEFEND_SPACE_CENTER,
        .title = sQuestTitleSpaceCenter,
        .description = sQuestDescSpaceCenter,
        .mapSecId = MAPSEC_MOSSDEEP_CITY,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE07_GET), QC_VAR_LT(VAR_MOSSDEEP_SPACE_CENTER_STATE, 2)},
        .completion = {QC_VAR_GE(VAR_MOSSDEEP_SPACE_CENTER_STATE, 2)},
    },
    {
        .id = QUESTOBJ_GET_DIVE_FROM_STEVEN,
        .title = sQuestTitleDive,
        .description = sQuestDescDive,
        .mapSecId = MAPSEC_MOSSDEEP_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(MOSSDEEP_CITY_STEVENS_HOUSE),
        .mapNum = MAP_NUM(MOSSDEEP_CITY_STEVENS_HOUSE),
        .localId = 1,
        .activationCount = 3,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_MOSSDEEP_SPACE_CENTER_STATE, 2), QC_VAR_GE(VAR_STEVENS_HOUSE_STATE, 1), QC_VAR_LT(VAR_STEVENS_HOUSE_STATE, 2)},
        .completion = {QC_VAR_GE(VAR_STEVENS_HOUSE_STATE, 2)},
    },
    {
        .id = QUESTOBJ_ENTER_SEAFLOOR_CAVERN,
        .title = sQuestTitleSeafloor,
        .description = sQuestDescSeafloor,
        .mapSecId = MAPSEC_SEAFLOOR_CAVERN,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_STEVENS_HOUSE_STATE, 2), QC_FLAG_CLEAR(FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN)},
        .completion = {QC_FLAG_SET(FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN)},
    },
    {
        .id = QUESTOBJ_GO_SOOTOPOLIS,
        .title = sQuestTitleSootopolis,
        .description = sQuestDescSootopolis,
        .mapSecId = MAPSEC_SOOTOPOLIS_CITY,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN), QC_VAR_LT(VAR_SOOTOPOLIS_CITY_STATE, 3)},
        .completion = {QC_VAR_GE(VAR_SOOTOPOLIS_CITY_STATE, 3)},
    },
    {
        .id = QUESTOBJ_WAKE_RAYQUAZA,
        .title = sQuestTitleRayquaza,
        .description = sQuestDescRayquaza,
        .mapSecId = MAPSEC_SKY_PILLAR,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_SOOTOPOLIS_CITY_STATE, 3), QC_VAR_LT(VAR_SOOTOPOLIS_CITY_STATE, 5)},
        .completion = {QC_VAR_GE(VAR_SOOTOPOLIS_CITY_STATE, 5)},
    },
    {
        .id = QUESTOBJ_RETURN_SOOTOPOLIS,
        .title = sQuestTitleReturnSootopolis,
        .description = sQuestDescReturnSootopolis,
        .mapSecId = MAPSEC_SOOTOPOLIS_CITY,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_SOOTOPOLIS_CITY_STATE, 5), QC_VAR_LT(VAR_SOOTOPOLIS_CITY_STATE, 6)},
        .completion = {QC_VAR_GE(VAR_SOOTOPOLIS_CITY_STATE, 6)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_JUAN,
        .title = sQuestTitleJuan,
        .description = sQuestDescJuan,
        .mapSecId = MAPSEC_SOOTOPOLIS_CITY,
        .targetType = QUEST_TARGET_OBJECT_EVENT,
        .mapGroup = MAP_GROUP(SOOTOPOLIS_CITY_GYM_1F),
        .mapNum = MAP_NUM(SOOTOPOLIS_CITY_GYM_1F),
        .localId = 1,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_VAR_GE(VAR_SOOTOPOLIS_CITY_STATE, 6), QC_FLAG_CLEAR(FLAG_BADGE08_GET)},
        .completion = {QC_FLAG_SET(FLAG_BADGE08_GET)},
    },
    {
        .id = QUESTOBJ_CROSS_VICTORY_ROAD,
        .title = sQuestTitleVictoryRoad,
        .description = sQuestDescVictoryRoad,
        .mapSecId = MAPSEC_VICTORY_ROAD,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_BADGE08_GET), QC_FLAG_CLEAR(FLAG_LANDMARK_POKEMON_LEAGUE)},
        .completion = {QC_FLAG_SET(FLAG_LANDMARK_POKEMON_LEAGUE)},
    },
    {
        .id = QUESTOBJ_CHALLENGE_POKEMON_LEAGUE,
        .title = sQuestTitlePokemonLeague,
        .description = sQuestDescPokemonLeague,
        .mapSecId = MAPSEC_EVER_GRANDE_CITY,
        .targetType = QUEST_TARGET_REGION,
        .activationCount = 2,
        .completionCount = 1,
        .activation = {QC_FLAG_SET(FLAG_LANDMARK_POKEMON_LEAGUE), QC_FLAG_CLEAR(FLAG_SYS_GAME_CLEAR)},
        .completion = {QC_FLAG_SET(FLAG_SYS_GAME_CLEAR)},
    },
};

static bool8 Quest_ConditionMet(const struct QuestCondition *condition)
{
    switch (condition->type)
    {
    case QUEST_CONDITION_FLAG_SET:
        return FlagGet(condition->id);
    case QUEST_CONDITION_FLAG_CLEAR:
        return !FlagGet(condition->id);
    case QUEST_CONDITION_VAR_EQ:
        return VarGet(condition->id) == condition->value;
    case QUEST_CONDITION_VAR_NE:
        return VarGet(condition->id) != condition->value;
    case QUEST_CONDITION_VAR_GE:
        return VarGet(condition->id) >= condition->value;
    case QUEST_CONDITION_VAR_LT:
        return VarGet(condition->id) < condition->value;
    }

    return FALSE;
}

static bool8 Quest_ConditionsMet(const struct QuestCondition *conditions, u8 count)
{
    u8 i;

    for (i = 0; i < count; i++)
    {
        if (!Quest_ConditionMet(&conditions[i]))
            return FALSE;
    }

    return TRUE;
}

const struct QuestObjective *Quest_GetActiveObjective(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sMainStoryObjectives); i++)
    {
        const struct QuestObjective *objective = &sMainStoryObjectives[i];

        if (Quest_ConditionsMet(objective->activation, objective->activationCount)
         && !Quest_ConditionsMet(objective->completion, objective->completionCount))
            return objective;
    }

    return NULL;
}

bool8 Quest_HasActiveObjective(void)
{
    return Quest_GetActiveObjective() != NULL;
}

u16 Quest_GetActiveMapSecId(void)
{
    const struct QuestObjective *objective = Quest_GetActiveObjective();

    if (objective == NULL)
        return MAPSEC_NONE;

    return objective->mapSecId;
}

bool8 Quest_TargetMatchesCurrentMap(const struct QuestObjective *objective)
{
    if (objective == NULL)
        return FALSE;

    if (objective->targetType != QUEST_TARGET_MAP
     && objective->targetType != QUEST_TARGET_OBJECT_EVENT
     && objective->targetType != QUEST_TARGET_COORDINATE)
        return FALSE;

    return gSaveBlock1Ptr->location.mapGroup == objective->mapGroup
        && gSaveBlock1Ptr->location.mapNum == objective->mapNum;
}

void Quest_RemoveLocalMarker(void)
{
    u8 spriteId = Quest_FindLocalMarkerSprite();

    if (spriteId != MAX_SPRITES)
        DestroySprite(&gSprites[spriteId]);
}

void Quest_UpdateLocalMarker(void)
{
    const struct QuestObjective *objective = Quest_GetActiveObjective();
    struct ObjectEvent *objectEvent;
    struct Sprite *objectSprite;
    struct Sprite *markerSprite;
    u8 objectEventId;
    u8 markerSpriteId = Quest_FindLocalMarkerSprite();

    if (objective == NULL
     || objective->targetType != QUEST_TARGET_OBJECT_EVENT
     || !Quest_TargetMatchesCurrentMap(objective)
     || TryGetObjectEventIdByLocalIdAndMap(objective->localId, objective->mapNum, objective->mapGroup, &objectEventId))
    {
        if (markerSpriteId != MAX_SPRITES)
            DestroySprite(&gSprites[markerSpriteId]);
        return;
    }

    objectEvent = &gObjectEvents[objectEventId];
    objectSprite = &gSprites[objectEvent->spriteId];

    if (!objectEvent->active || !objectSprite->inUse)
    {
        if (markerSpriteId != MAX_SPRITES)
            DestroySprite(&gSprites[markerSpriteId]);
        return;
    }

    if (markerSpriteId == MAX_SPRITES)
    {
        LoadObjectEventPalette(0x1100);
        markerSpriteId = CreateSpriteAtEnd(&sQuestMarkerSpriteTemplate, objectSprite->x, objectSprite->y - 16, 0x52);
        if (markerSpriteId == MAX_SPRITES)
            return;
    }

    markerSprite = &gSprites[markerSpriteId];
    markerSprite->x = objectSprite->x;
    markerSprite->y = objectSprite->y - 16;
    markerSprite->x2 = objectSprite->x2;
    markerSprite->y2 = objectSprite->y2;
    markerSprite->invisible = objectEvent->invisible || objectSprite->invisible;
}
