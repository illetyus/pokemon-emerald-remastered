#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "RemasterPokemonAssetSet.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UAnimSequence;

USTRUCT(BlueprintType)
struct FRemasterPokemonBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> StaticMesh;
    UPROPERTY(EditAnywhere) TMap<FName, TSoftObjectPtr<UAnimSequence>> Animations;
    UPROPERTY(EditAnywhere) FString SourceSha256;
    UPROPERTY(EditAnywhere) FString NormalizedSha256;
    UPROPERTY(EditAnywhere) FString ProvenanceSha256;
    UPROPERTY(EditAnywhere) float UniformScale = 1.0f;
    UPROPERTY(EditAnywhere) float GroundOffsetCm = 0.0f;
    UPROPERTY(EditAnywhere) bool bUnrealImportValidated = false;
    UPROPERTY(EditAnywhere) bool bSpecialChannelsInspected = false;
    UPROPERTY(EditAnywhere) TArray<FName> RequiredSpecialChannels;
};

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterPokemonAssetSet : public UDataAsset
{
    GENERATED_BODY()
public:
    // Exact key: pokemon.<national>.<form>.<normal|shiny>, or trainer.pic.<000>.
    UPROPERTY(EditAnywhere) TMap<FString, FRemasterPokemonBinding> Models;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Remaster Battle Presentation"))
class POKEMONEMERALDREMASTERED_API URemasterBattlePresentationSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Local Assets") TSoftObjectPtr<URemasterPokemonAssetSet> AssetSet;
};
