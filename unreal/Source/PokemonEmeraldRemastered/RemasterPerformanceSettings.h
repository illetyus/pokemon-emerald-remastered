#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RemasterPerformanceSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Pokemon Emerald Remastered Performance"))
class POKEMONEMERALDREMASTERED_API URemasterPerformanceSettings
    : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Frame Rate", meta=(ClampMin="30", ClampMax="120"))
    int32 DefaultFrameRate = 60;

    UPROPERTY(Config, EditAnywhere, Category="Frame Rate", meta=(ClampMin="30", ClampMax="120"))
    int32 HighRefreshFrameRate = 120;

    UPROPERTY(Config, EditAnywhere, Category="World")
    bool bEnableDynamicShadowsOnHigh = true;

    UPROPERTY(Config, EditAnywhere, Category="World", meta=(ClampMin="0.25", ClampMax="2.0"))
    float HighParticleScale = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category="World", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BatteryParticleScale = 0.45f;
};
