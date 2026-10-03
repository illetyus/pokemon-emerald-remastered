#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterAudioSubsystem.generated.h"

class UAudioComponent;
class URemasterAudioProfile;
class USoundBase;

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterAudioSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio")
    void SetProfile(URemasterAudioProfile* InProfile);

    UFUNCTION(BlueprintCallable, Category="Remaster|Audio")
    bool PlayMusic(FName Id, float FadeSeconds = 0.5f);

    UFUNCTION(BlueprintCallable, Category="Remaster|Audio")
    bool PlaySfx(FName Id);

    UFUNCTION(BlueprintCallable, Category="Remaster|Audio")
    void StopMusic(float FadeSeconds = 0.5f);

private:
    USoundBase* Resolve(
        const TMap<FName, TSoftObjectPtr<USoundBase>>& Table,
        FName Id) const;

    UPROPERTY()
    TObjectPtr<URemasterAudioProfile> Profile;

    UPROPERTY()
    TObjectPtr<UAudioComponent> MusicComponent;
};
