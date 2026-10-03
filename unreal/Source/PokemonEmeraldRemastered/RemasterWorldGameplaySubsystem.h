#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterWorldData.h"
#include "RemasterWorldGameplaySubsystem.generated.h"


USTRUCT(BlueprintType)
struct FRemasterResolvedConnection
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 SourceConnectionIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 Direction = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Offset = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DestGroupNum = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 DestMapNum = -1;

    UPROPERTY(BlueprintReadOnly)
    FString DestMap;
};

USTRUCT(BlueprintType)
struct FRemasterResolvedWarp
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 SourceEventIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 DestGroupNum = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 DestMapNum = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 DestWarpId = -1;

    UPROPERTY(BlueprintReadOnly)
    FString DestMap;

    UPROPERTY(BlueprintReadOnly)
    bool bDynamicTarget = false;
};

UENUM(BlueprintType)
enum class ERemasterResolvedCoordKind : uint8
{
    None,
    Script,
    Weather
};

USTRUCT(BlueprintType)
struct FRemasterResolvedCoordEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ERemasterResolvedCoordKind Kind = ERemasterResolvedCoordKind::None;

    UPROPERTY(BlueprintReadOnly)
    int32 SourceEventIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    FString Script;

    UPROPERTY(BlueprintReadOnly)
    FString Weather;

    UPROPERTY(BlueprintReadOnly)
    int32 WeatherId = -1;
};

UENUM(BlueprintType)
enum class ERemasterResolvedBackgroundKind : uint8
{
    None,
    Script,
    HiddenItem,
    SecretBase
};

USTRUCT(BlueprintType)
struct FRemasterResolvedBackgroundEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ERemasterResolvedBackgroundKind Kind =
        ERemasterResolvedBackgroundKind::None;

    UPROPERTY(BlueprintReadOnly)
    int32 SourceEventIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    FString Script;

    UPROPERTY(BlueprintReadOnly)
    FString Item;

    UPROPERTY(BlueprintReadOnly)
    int32 ItemId = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 FlagId = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 SecretBaseId = -1;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterWorldGameplaySubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /*
     * Loads the map addressed by the legacy save. When bResetTemporaryState is
     * true this mirrors Vanilla map entry by clearing TEMP flags/vars first.
     */
    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool LoadCurrentMapFromSave(bool bResetTemporaryState = true);

    UFUNCTION(BlueprintPure, Category="Remaster|World|Gameplay")
    bool IsMapReady() const
    {
        return bMapReady;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|World|Gameplay")
    FString GetCurrentMapId() const
    {
        return bMapReady ? CurrentMap.Id : FString();
    }

    UFUNCTION(BlueprintPure, Category="Remaster|World|Gameplay")
    int32 GetCurrentMapGroup() const
    {
        return bMapReady ? CurrentMap.GroupNum : -1;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|World|Gameplay")
    int32 GetCurrentMapNumber() const
    {
        return bMapReady ? CurrentMap.MapNum : -1;
    }

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool IsObjectVisible(int32 LocalId, bool& OutVisible) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool ResolveConnection(
        int32 Direction,
        FRemasterResolvedConnection& OutConnection) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool ApplyResolvedConnection(
        const FRemasterResolvedConnection& Connection);

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool ResolveWarpAt(
        int32 X,
        int32 Y,
        int32 Elevation,
        FRemasterResolvedWarp& OutWarp) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool ResolveCoordEventAt(
        int32 X,
        int32 Y,
        int32 Elevation,
        FRemasterResolvedCoordEvent& OutEvent) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool ResolveBackgroundEventAt(
        int32 X,
        int32 Y,
        int32 Elevation,
        int32 FacingDirection,
        FRemasterResolvedBackgroundEvent& OutEvent) const;

    UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")
    bool ApplyResolvedWarp(const FRemasterResolvedWarp& Warp);

    const FRemasterMapIR* GetCurrentMapForPresentation() const
    {
        return bMapReady ? &CurrentMap : nullptr;
    }

private:
    bool ApplySavedObjectTemplateOverrides();
    bool RefreshSavedObjectTemplateCache(
        const FRemasterMapIR& Map);

    FRemasterMapIR CurrentMap;
    bool bMapReady = false;
};
