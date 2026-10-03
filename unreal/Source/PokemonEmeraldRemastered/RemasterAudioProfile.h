#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RemasterAudioProfile.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterAudioProfile
    : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music")
    TMap<FName, TSoftObjectPtr<USoundBase>> Music;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
    TMap<FName, TSoftObjectPtr<USoundBase>> SoundEffects;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ambience")
    TMap<FName, TSoftObjectPtr<USoundBase>> Ambience;
};
