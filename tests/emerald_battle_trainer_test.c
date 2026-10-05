#include "remaster/emerald_battle.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    RemasterEmeraldTrainer trainer;
    const RemasterEmeraldTrainer *catalog_trainer;

    if (!check(
            remaster_emerald_battle_trainer_count() == 857u,
            "generated trainer catalog must cover IDs 0 through 856"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(1);
    if (!check(
            catalog_trainer != 0
                && catalog_trainer->trainer_id == 1
                && catalog_trainer->trainer_class == 2
                && strcmp(catalog_trainer->trainer_name, "SAWYER") == 0
                && catalog_trainer->ai_flags
                    == (REMASTER_EMERALD_AI_CHECK_BAD_MOVE
                        | REMASTER_EMERALD_AI_TRY_TO_FAINT
                        | REMASTER_EMERALD_AI_CHECK_VIABILITY)
                && catalog_trainer->party_size == 1
                && catalog_trainer->party[0].species == 74
                && catalog_trainer->party[0].level == 21,
            "Sawyer must round-trip from pinned Vanilla+ trainer data"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(11);
    if (!check(
            catalog_trainer != 0
                && strcmp(catalog_trainer->trainer_name, "MARCEL") == 0
                && catalog_trainer->items[0] == 21
                && catalog_trainer->party_size == 2,
            "trainer items and party size must survive catalog generation"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(38);
    if (!check(
            catalog_trainer != 0
                && strcmp(catalog_trainer->trainer_name, "FELIX") == 0
                && catalog_trainer->party_flags
                    == REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET
                && catalog_trainer->party[0].moves[0] == 94,
            "custom trainer moves must survive catalog generation"))
        return 1;

    catalog_trainer = remaster_emerald_battle_trainer_find(51);
    if (!check(
            catalog_trainer != 0
                && strcmp(catalog_trainer->trainer_name, "GABBY & TY") == 0
                && catalog_trainer->double_battle
                && catalog_trainer->party_size == 2,
            "double-battle trainer metadata must survive catalog generation"))
        return 1;

    if (!check(
            remaster_emerald_battle_trainer_find(857) == 0,
            "out-of-range trainer IDs must not resolve"))
        return 1;

    memset(&trainer, 0, sizeof(trainer));
    trainer.trainer_id = 1;
    trainer.trainer_class = 1;
    trainer.ai_flags =
        REMASTER_EMERALD_AI_CHECK_BAD_MOVE
        | REMASTER_EMERALD_AI_TRY_TO_FAINT
        | REMASTER_EMERALD_AI_CHECK_VIABILITY;
    trainer.party_size = 1;
    trainer.party[0].species = 1;
    trainer.party[0].level = 21;

    if (!check(
            remaster_emerald_battle_trainer_validate(&trainer),
            "default-moves trainer contract should validate"))
        return 1;

    trainer.party[0].moves[0] = 1;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "custom move data requires the custom-moves flag"))
        return 1;

    trainer.party_flags = REMASTER_EMERALD_TRAINER_PARTY_CUSTOM_MOVESET;
    if (!check(
            remaster_emerald_battle_trainer_validate(&trainer),
            "custom move trainer contract should validate with flag"))
        return 1;

    trainer.party[0].held_item = 1;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "held item data requires the held-item flag"))
        return 1;

    trainer.party_flags |= REMASTER_EMERALD_TRAINER_PARTY_HELD_ITEM;
    if (!check(
            remaster_emerald_battle_trainer_validate(&trainer),
            "held-item custom-move trainer contract should validate"))
        return 1;

    trainer.party[0].level = 101;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "trainer level must remain within Gen III bounds"))
        return 1;

    trainer.party[0].level = 21;
    trainer.party_size = 0;
    if (!check(
            !remaster_emerald_battle_trainer_validate(&trainer),
            "empty trainer parties must be rejected"))
        return 1;

    puts("r13 trainer contract test passed");
    return 0;
}
