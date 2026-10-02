#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RemasterFeedbackProfile.generated.h"

class UCameraShakeBase;
class UNiagaraSystem;
class USoundBase;

USTRUCT(BlueprintType)
struct FRemasterFeedbackCue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName EventName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UNiagaraSystem> Niagara;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UCameraShakeBase> CameraShake;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float HapticStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float HapticDuration = 0.0f;
};

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterFeedbackProfile
    : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FRemasterFeedbackCue> Cues;
};
