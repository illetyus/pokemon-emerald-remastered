#include "RemasterPerformanceSubsystem.h"

#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RemasterPerformanceSettings.h"

void URemasterPerformanceSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetWindow();
    ApplyQualityPreset(ERemasterQualityPreset::Balanced);

    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(
            this,
            &URemasterPerformanceSubsystem::Tick));
}

void URemasterPerformanceSubsystem::Deinitialize()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }

    Super::Deinitialize();
}

void URemasterPerformanceSubsystem::ResetWindow()
{
    ElapsedSeconds = 0.0;
    TotalFrameMs = 0.0;
    WorstFrameMs = 0.0;
    FrameCount = 0;
}

bool URemasterPerformanceSubsystem::Tick(float DeltaSeconds)
{
    const double FrameMs = static_cast<double>(DeltaSeconds) * 1000.0;

    ElapsedSeconds += DeltaSeconds;
    TotalFrameMs += FrameMs;
    WorstFrameMs = FMath::Max(WorstFrameMs, FrameMs);
    ++FrameCount;

    if (ElapsedSeconds >= 5.0 && FrameCount > 0)
    {
        const double AverageMs =
            TotalFrameMs / static_cast<double>(FrameCount);

        const double Fps =
            static_cast<double>(FrameCount) / ElapsedSeconds;

        UE_LOG(
            LogTemp,
            Display,
            TEXT("R0 PERF renderer=Unreal fps=%.2f avg_frame_ms=%.3f worst_frame_ms=%.3f frames=%lld"),
            Fps,
            AverageMs,
            WorstFrameMs,
            static_cast<long long>(FrameCount));

        ResetWindow();
    }

    return true;
}


void URemasterPerformanceSubsystem::ApplyQualityPreset(
    ERemasterQualityPreset Preset)
{
    CurrentPreset = Preset;

    const URemasterPerformanceSettings* Settings =
        GetDefault<URemasterPerformanceSettings>();

    int32 TargetFps = Settings ? Settings->DefaultFrameRate : 60;
    float ResolutionScale = 100.0f;
    int32 ShadowQuality = 1;
    int32 EffectsQuality = 2;

    switch (Preset)
    {
    case ERemasterQualityPreset::Battery:
        TargetFps = 60;
        ResolutionScale = 80.0f;
        ShadowQuality = 0;
        EffectsQuality = 1;
        break;

    case ERemasterQualityPreset::High:
        TargetFps = Settings ? Settings->HighRefreshFrameRate : 120;
        ResolutionScale = 100.0f;
        ShadowQuality = 2;
        EffectsQuality = 3;
        break;

    case ERemasterQualityPreset::Balanced:
    default:
        break;
    }

    if (IConsoleVariable* MaxFps = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
        MaxFps->Set(static_cast<float>(TargetFps), ECVF_SetByGameSetting);

    if (IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
        ScreenPercentage->Set(ResolutionScale, ECVF_SetByGameSetting);

    if (IConsoleVariable* Shadows = IConsoleManager::Get().FindConsoleVariable(TEXT("sg.ShadowQuality")))
        Shadows->Set(ShadowQuality, ECVF_SetByGameSetting);

    if (IConsoleVariable* Effects = IConsoleManager::Get().FindConsoleVariable(TEXT("sg.EffectsQuality")))
        Effects->Set(EffectsQuality, ECVF_SetByGameSetting);

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Remaster quality preset=%d target_fps=%d resolution=%.0f shadow=%d effects=%d"),
        static_cast<int32>(Preset),
        TargetFps,
        ResolutionScale,
        ShadowQuality,
        EffectsQuality);
}
