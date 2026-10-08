#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RemasterAudioProfile.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class ERemasterAudioPack : uint8 { Original, Modern };

USTRUCT(BlueprintType)
struct FRemasterAudioAsset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USoundBase> Sound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bImportValidated = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bLoopValidated = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bLoop = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Gain = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float CrossfadeSeconds = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SampleRate = 48000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Frames = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LoopStartFrame = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LoopEndFrame = 0;
};

UCLASS(BlueprintType)
class POKEMONEMERALDREMASTERED_API URemasterAudioProfile
    : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Semantic Audio")
    TMap<FName, FRemasterAudioAsset> Original;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Semantic Audio")
    TMap<FName, FRemasterAudioAsset> Modern;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Semantic Audio")
    bool bAllowOriginalFallback = true;
    // Legacy authoring maps; source semantic dispatch uses the maps above.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music")
    TMap<FName, TSoftObjectPtr<USoundBase>> Music;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
    TMap<FName, TSoftObjectPtr<USoundBase>> SoundEffects;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ambience")
    TMap<FName, TSoftObjectPtr<USoundBase>> Ambience;
};
