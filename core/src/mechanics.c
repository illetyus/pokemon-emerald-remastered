#include "remaster/mechanics.h"

static RemasterMechanicsConfig sConfig = {
    REMASTER_MECHANICS_EMERALD_AUTHENTIC,
    0,
    0,
    0,
    1,
    1,
    1
};

void remaster_mechanics_set(const RemasterMechanicsConfig *config)
{
    if (config == 0)
        return;

    sConfig = *config;
}

RemasterMechanicsConfig remaster_mechanics_get(void)
{
    return sConfig;
}

void remaster_mechanics_reset(void)
{
    RemasterMechanicsConfig defaults = {
        REMASTER_MECHANICS_EMERALD_AUTHENTIC,
        0,
        0,
        0,
        1,
        1,
        1
    };

    sConfig = defaults;
}
