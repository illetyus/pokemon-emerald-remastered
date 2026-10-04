#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RemasterVisualStyle.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FRemasterTileVisualRule
{
    GENERATED_BODY()

    /*
     * Canonical R5 visual identity. Metatile IDs 0..511 are local to the
     * primary tileset; 512..1023 are local to the active secondary tileset.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Tileset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 LocalMetatileId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UMaterialInterface> Material;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector Scale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float HeightOffset = 0.0f;
};

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterVisualStyle : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
    TArray<FRemasterTileVisualRule> TileRules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
    TSoftObjectPtr<UMaterialInterface> WaterMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Characters")
    TSoftObjectPtr<UMaterialInterface> CharacterMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
    TSoftObjectPtr<UNiagaraSystem> FootstepEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
    TSoftObjectPtr<UNiagaraSystem> EncounterEffect;
};
