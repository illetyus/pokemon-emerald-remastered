#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RemasterInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterInputConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputMappingContext> MappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> Move;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> Interact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> Cancel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> Menu;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> Map;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> Quest;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UInputAction> QuickItem;

    // Presentation-only touch authoring; never stored in Emerald saves.
    UPROPERTY(EditAnywhere, Category="Remaster|Touch")
    bool bTouchEnabled = true;

    // Pixel insets: left, top, right, bottom. Actual device safe area needs runtime validation.
    UPROPERTY(EditAnywhere, Category="Remaster|Touch")
    FVector4 TouchSafeInsets = FVector4(0, 0, 0, 0);

    // Empty uses the validated common defaults; overrides require ten nonoverlapping rectangles.
    UPROPERTY(EditAnywhere, Category="Remaster|Touch")
    TArray<FVector4> TouchRectangles;
};

