#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "RemasterCharacterAssetSet.generated.h"

class USkeletalMesh;

USTRUCT(BlueprintType)
struct FRemasterCharacterAssetBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<USkeletalMesh> Mesh;

    // Exact output hash from the local normalization/import receipt.
    UPROPERTY(EditAnywhere)
    FString NormalizedSha256;

    UPROPERTY(EditAnywhere)
    bool bUnrealImportValidated = false;
};

// Created locally. No commercial Unreal assets are required by the public build.
UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterCharacterAssetSet : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    TMap<FString, FRemasterCharacterAssetBinding> Models;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Remaster Character Presentation"))
class POKEMONEMERALDREMASTERED_API URemasterCharacterPresentationSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Local Assets")
    TSoftObjectPtr<URemasterCharacterAssetSet> AssetSet;
};
