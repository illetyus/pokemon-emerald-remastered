#include "RemasterPerformanceSubsystem.h"

#include "Containers/Ticker.h"

void URemasterPerformanceSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetWindow();

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
