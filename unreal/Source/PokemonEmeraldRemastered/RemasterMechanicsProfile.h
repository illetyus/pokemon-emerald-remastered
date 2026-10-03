#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RemasterMechanicsProfile.generated.h"

UENUM(BlueprintType)
enum class ERemasterMechanicsGeneration : uint8
{
    EmeraldAuthentic,
    SelectiveModern,
    Modern
};

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterMechanicsProfile
    : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERemasterMechanicsGeneration Generation =
        ERemasterMechanicsGeneration::EmeraldAuthentic;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bPhysicalSpecialSplit = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bModernTypeChart = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bModernAbilities = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bAlternativeTradeEvolutions = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bShowMoveEffectiveness = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bFastBattleAnimationsOption = true;
};
