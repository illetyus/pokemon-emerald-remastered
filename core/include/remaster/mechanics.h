#ifndef REMASTER_MECHANICS_H
#define REMASTER_MECHANICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RemasterMechanicsGeneration {
    REMASTER_MECHANICS_EMERALD_AUTHENTIC = 0,
    REMASTER_MECHANICS_SELECTIVE_MODERN = 1,
    REMASTER_MECHANICS_MODERN = 2
} RemasterMechanicsGeneration;

typedef struct RemasterMechanicsConfig {
    RemasterMechanicsGeneration generation;

    uint8_t physical_special_split;
    uint8_t modern_type_chart;
    uint8_t modern_abilities;
    uint8_t alternative_trade_evolutions;
    uint8_t show_move_effectiveness;
    uint8_t fast_battle_animations_option;
} RemasterMechanicsConfig;

void remaster_mechanics_set(const RemasterMechanicsConfig *config);
RemasterMechanicsConfig remaster_mechanics_get(void);
void remaster_mechanics_reset(void);

#ifdef __cplusplus
}
#endif

#endif
