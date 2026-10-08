#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterCharacterCatalogSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FRemasterCharacterPresentationEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 GraphicsId = -1;

    UPROPERTY(BlueprintReadOnly)
    FString GraphicsName;

    UPROPERTY(BlueprintReadOnly)
    FString PresentationId;

    UPROPERTY(BlueprintReadOnly)
    FString PresentationKind;

    UPROPERTY(BlueprintReadOnly)
    FString SourceFamily;

    UPROPERTY(BlueprintReadOnly)
    FString SkeletonFamily;

    UPROPERTY(BlueprintReadOnly)
    FString ModelId;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> MaterialIds;

    UPROPERTY(BlueprintReadOnly)
    FString AnimationSetId;

    UPROPERTY(BlueprintReadOnly)
    float Scale = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    float GroundOffsetCm = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float YawOffsetDeg = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FString LodProfile;

    UPROPERTY(BlueprintReadOnly)
    FString FallbackId;

    UPROPERTY(BlueprintReadOnly)
    FString ProvenanceId;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> SourceSha256;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> NormalizedSha256;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterCharacterCatalogSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    uint32 GetCatalogRevision() const { return CatalogRevision; }

    UFUNCTION(BlueprintCallable, Category="Remaster|Character|Presentation")
    bool ReloadCatalog();

    UFUNCTION(BlueprintPure, Category="Remaster|Character|Presentation")
    bool IsCatalogReady() const
    {
        return bCatalogReady;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|Character|Presentation")
    FString GetManifestPath() const;

    UFUNCTION(BlueprintPure, Category="Remaster|Character|Presentation")
    int32 GetIdentityCount() const
    {
        return bCatalogReady ? EntriesById.Num() : 0;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|Character|Presentation")
    bool ResolveGraphicsId(
        int32 GraphicsId,
        FRemasterCharacterPresentationEntry& OutEntry) const;

    UFUNCTION(BlueprintPure, Category="Remaster|Character|Presentation")
    bool ResolveGraphicsName(
        const FString& GraphicsName,
        FRemasterCharacterPresentationEntry& OutEntry) const;

private:
    uint32 CatalogRevision = 0;
    bool bCatalogReady = false;
    FString ContentSha256;
    TMap<int32, FRemasterCharacterPresentationEntry> EntriesById;
    TMap<FString, int32> GraphicsNameToId;
};
