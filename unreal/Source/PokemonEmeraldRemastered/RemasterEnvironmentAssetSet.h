#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "RemasterEnvironmentMath.h"
#include "RemasterEnvironmentAssetSet.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct FRemasterEnvironmentBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere) FString SourceSha256;
    UPROPERTY(EditAnywhere) FString NormalizedSha256;
    UPROPERTY(EditAnywhere) FString ProvenanceSha256;
    UPROPERTY(EditAnywhere) bool bImportValidated = false;
    UPROPERTY(EditAnywhere) FVector Scale = FVector::OneVector;
    UPROPERTY(EditAnywhere) float HeightOffset = 0.0f;
    UPROPERTY(EditAnywhere) int32 Lod0Triangles = 0;
    UPROPERTY(EditAnywhere) int32 Lod1Triangles = 0;
    UPROPERTY(EditAnywhere) int32 Lod2Triangles = 0;
    UPROPERTY(EditAnywhere) int32 MaterialSlots = 0;
    UPROPERTY(EditAnywhere) int32 MaxTextureDimension = 0;
    UPROPERTY(EditAnywhere) float CullDistance = 4000.0f;
    // Only separate roof/wall fragments may opt in; whole terrain stays visible.
    UPROPERTY(EditAnywhere) bool bCameraOccluder = false;

    bool IsUsable() const
    {
        return bImportValidated && !Mesh.IsNull()
            && remaster::environment::hash_valid(TCHAR_TO_UTF8(*SourceSha256))
            && remaster::environment::hash_valid(TCHAR_TO_UTF8(*NormalizedSha256))
            && remaster::environment::hash_valid(TCHAR_TO_UTF8(*ProvenanceSha256))
            && remaster::environment::budget_valid(Lod0Triangles, Lod1Triangles,
                Lod2Triangles, MaterialSlots, MaxTextureDimension, CullDistance)
            && !Scale.ContainsNaN() && Scale.GetMin() > 0.0f && Scale.GetMax() <= 10.0f
            && FMath::IsFinite(HeightOffset) && FMath::Abs(HeightOffset) <= 1000.0f;
    }
};

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterEnvironmentAssetSet : public UDataAsset
{
    GENERATED_BODY()
public:
    // Exact gTileset_Name:local-id, no behavior-wide or named-map substitution.
    UPROPERTY(EditAnywhere) TMap<FString, FRemasterEnvironmentBinding> Models;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Remaster Environment Presentation"))
class POKEMONEMERALDREMASTERED_API URemasterEnvironmentPresentationSettings
    : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere) TSoftObjectPtr<URemasterEnvironmentAssetSet> AssetSet;
};
