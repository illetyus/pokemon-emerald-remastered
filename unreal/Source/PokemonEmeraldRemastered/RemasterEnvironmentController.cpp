#include "RemasterEnvironmentController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/GameInstance.h"
#include "RemasterEnvironmentMath.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldGameplaySubsystem.h"

ARemasterEnvironmentController::ARemasterEnvironmentController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(SceneRoot);
    Sun->SetMobility(EComponentMobility::Movable);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("Sky"));
    Sky->SetupAttachment(SceneRoot);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->bRealTimeCapture = false;
    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
    Fog->SetupAttachment(SceneRoot);
    SetTimeOfDay(12.0f);
}

void ARemasterEnvironmentController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;
    const auto* Save = GI->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    const auto* Gameplay = GI->GetSubsystem<URemasterWorldGameplaySubsystem>();
    const FRemasterMapIR* Map = Gameplay ? Gameplay->GetCurrentMapForPresentation() : nullptr;
    FRemasterLegacyOverworldSnapshot Snapshot;
    if (!Save || !Map || !Map->IsValid() || !Save->GetOverworldSnapshot(Snapshot)
        || Map->GroupNum != Snapshot.MapGroup || Map->MapNum != Snapshot.MapNum) return;
    RuntimeWeatherId = Snapshot.Weather;
    MapTypeId = Map->MapTypeId;
    FRemasterLegacyRtcSnapshot Rtc;
    if (Save->GetLocalRtcNow(Rtc))
        TimeOfDay = static_cast<float>(remaster::environment::hour(
            Rtc.Hours + Rtc.Minutes / 60.0 + Rtc.Seconds / 3600.0));
    // A missing RTC retains the last valid visual hour. Never advances/re-aligns it.
    ApplyLighting();
}

void ARemasterEnvironmentController::SetTimeOfDay(float Hour)
{
    TimeOfDay = static_cast<float>(remaster::environment::hour(Hour));
    ApplyLighting();
}

void ARemasterEnvironmentController::ApplyLighting()
{
    using namespace remaster::environment;
    const Weather Value = weather(RuntimeWeatherId);
    switch (Value)
    {
    case Weather::Rain: WeatherTag = TEXT("Rain"); break;
    case Weather::Fog: WeatherTag = TEXT("Fog"); break;
    case Weather::Dust: WeatherTag = TEXT("Dust"); break;
    case Weather::Underwater: WeatherTag = TEXT("Underwater"); break;
    case Weather::Shade: WeatherTag = TEXT("Shade"); break;
    case Weather::Drought: WeatherTag = TEXT("Drought"); break;
    case Weather::Unresolved: WeatherTag = TEXT("Unresolved"); break;
    default: WeatherTag = TEXT("Clear"); break;
    }
    const Context MapContext = context(MapTypeId);
    const Lighting Profile = lighting(TimeOfDay, MapContext, Value);
    const bool bExterior = MapContext == Context::Outdoor;
    Sun->SetVisibility(bExterior);
    Sun->SetWorldRotation(FRotator(-15.0 - 60.0 * Profile.solar, -35.0, 0.0));
    Sun->SetIntensity(FMath::Lerp(NightSunIntensity, DaySunIntensity,
        static_cast<float>(Profile.solar)));
    Sun->SetLightColor(FLinearColor::LerpUsingHSV(FLinearColor(0.32f, 0.42f, 0.70f),
        FLinearColor(1.0f, 0.93f, 0.78f), static_cast<float>(Profile.solar)));
    Sky->SetIntensity(static_cast<float>(Profile.ambient));
    Fog->SetFogDensity(static_cast<float>(Profile.fog));
}

void ARemasterEnvironmentController::SetWeatherTag(FName InWeather)
{
    // Debug presentation override; authoritative snapshot replaces it next tick.
    RuntimeWeatherId = InWeather == TEXT("Rain") ? 3 : InWeather == TEXT("Fog") ? 6 : 0;
    ApplyLighting();
}
