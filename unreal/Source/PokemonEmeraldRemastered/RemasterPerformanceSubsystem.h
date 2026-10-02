#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterPerformanceSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERemasterQualityPreset : uint8
{
    Battery,
    Balanced,
    High
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterPerformanceSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Remaster|Performance")
    void ApplyQualityPreset(ERemasterQualityPreset Preset);

    UFUNCTION(BlueprintPure, Category="Remaster|Performance")
    ERemasterQualityPreset GetQualityPreset() const
    {
        return CurrentPreset;
    }

private:
    bool Tick(float DeltaSeconds);
    void ResetWindow();

    FTSTicker::FDelegateHandle TickerHandle;
    double ElapsedSeconds = 0.0;
    double TotalFrameMs = 0.0;
    double WorstFrameMs = 0.0;
    int64 FrameCount = 0;
    ERemasterQualityPreset CurrentPreset = ERemasterQualityPreset::Balanced;
};
