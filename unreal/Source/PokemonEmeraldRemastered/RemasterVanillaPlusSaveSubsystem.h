#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterVanillaPlusSaveSubsystem.generated.h"

UENUM(BlueprintType)
enum class ERemasterLegacySaveStatus : uint8
{
    Empty,
    Ok,
    Degraded,
    Corrupt,
    IoError,
    Unsupported
};

USTRUCT(BlueprintType)
struct FRemasterLegacyOverworldSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerX = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerY = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MapGroup = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MapNum = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 WarpId = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 WarpX = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 WarpY = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MapLayoutId = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 SavedMusic = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Weather = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 WeatherCycleStage = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 FlashLevel = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PartyCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int64 Money = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Coins = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RegisteredItem = 0;
};

USTRUCT(BlueprintType)
struct FRemasterLegacyRtcSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 Days = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Hours = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Minutes = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Seconds = 0;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterVanillaPlusSaveSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    ERemasterLegacySaveStatus LoadLegacySave();

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool StoreLegacySave();

    UFUNCTION(BlueprintPure, Category="Remaster|VanillaPlus")
    ERemasterLegacySaveStatus GetStatus() const
    {
        return Status;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|VanillaPlus")
    bool HasUsableSave() const
    {
        return Status == ERemasterLegacySaveStatus::Ok
            || Status == ERemasterLegacySaveStatus::Degraded;
    }

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool GetOverworldSnapshot(FRemasterLegacyOverworldSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool GetPersistentFlag(int32 FlagId, bool& OutValue) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool SetPersistentFlag(int32 FlagId, bool bValue);

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool GetPersistentVar(int32 VarId, int32& OutValue) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool SetPersistentVar(int32 VarId, int32 Value);

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool GetLocalRtcNow(FRemasterLegacyRtcSnapshot& OutTime) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|VanillaPlus")
    bool RealignRtcNow(const FRemasterLegacyRtcSnapshot& DesiredLocalTime);

    UFUNCTION(BlueprintPure, Category="Remaster|VanillaPlus")
    FString GetLegacySavePath() const;

    // Internal C++ bridge. Presentation code should use typed public APIs.
    const void* GetNativeSaveHandle() const
    {
        return NativeSave;
    }

    void* GetMutableNativeSaveHandle()
    {
        return NativeSave;
    }

private:
    void* NativeSave = nullptr;
    TArray<uint8> ScratchImage;
    ERemasterLegacySaveStatus Status = ERemasterLegacySaveStatus::Empty;
};
