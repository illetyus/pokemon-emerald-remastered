#include "RemasterEnvironmentController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"

ARemasterEnvironmentController::ARemasterEnvironmentController()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(SceneRoot);
    Sun->SetMobility(EComponentMobility::Movable);

    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("Sky"));
    Sky->SetupAttachment(SceneRoot);
    Sky->SetMobility(EComponentMobility::Movable);

    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
    Fog->SetupAttachment(SceneRoot);

    SetTimeOfDay(12.0f);
}

void ARemasterEnvironmentController::SetTimeOfDay(float Hour)
{
    TimeOfDay = FMath::Fmod(
        FMath::Max(0.0f, Hour),
        24.0f);

    const float SolarAlpha =
        FMath::Clamp(
            FMath::Sin(
                ((TimeOfDay - 6.0f) / 12.0f) * PI),
            0.0f,
            1.0f);

    const float Pitch =
        FMath::Lerp(
            -15.0f,
            -75.0f,
            SolarAlpha);

    Sun->SetWorldRotation(
        FRotator(Pitch, -35.0f, 0.0f));

    Sun->SetIntensity(
        FMath::Lerp(
            NightSunIntensity,
            DaySunIntensity,
            SolarAlpha));

    const FLinearColor NightColor(0.32f, 0.42f, 0.70f);
    const FLinearColor DayColor(1.0f, 0.93f, 0.78f);

    Sun->SetLightColor(
        FLinearColor::LerpUsingHSV(
            NightColor,
            DayColor,
            SolarAlpha));

    Sky->SetIntensity(
        FMath::Lerp(
            0.18f,
            1.0f,
            SolarAlpha));

    Fog->SetFogDensity(
        FMath::Lerp(
            0.012f,
            0.003f,
            SolarAlpha));
}

void ARemasterEnvironmentController::SetWeatherTag(FName InWeather)
{
    WeatherTag = InWeather;

    if (WeatherTag == TEXT("Fog"))
    {
        Fog->SetFogDensity(0.04f);
    }
    else if (WeatherTag == TEXT("Rain"))
    {
        Fog->SetFogDensity(0.012f);
    }
    else
    {
        SetTimeOfDay(TimeOfDay);
    }
}
