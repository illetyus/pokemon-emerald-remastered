#include "remaster/platform.h"

static RemasterPlatformVTable sPlatform;
static int sInstalled = 0;

void remaster_platform_install(const RemasterPlatformVTable *platform)
{
    if (platform == 0) {
        sInstalled = 0;
        return;
    }

    sPlatform = *platform;
    sInstalled = 1;
}

const RemasterPlatformVTable *remaster_platform_get(void)
{
    return sInstalled ? &sPlatform : 0;
}
