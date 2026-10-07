#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterAudioProfile.h"
#include "RemasterAudioRead.h"
#include "RemasterAudioSubsystem.generated.h"
class UAudioComponent;
class USoundBase;
USTRUCT(BlueprintType)
struct FRemasterAudioSettings {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Master=1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Music=.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Sfx=.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Ambience=.6f;
};
USTRUCT()
struct FRemasterAudioVoice {
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UAudioComponent> Component;
    uint64 Id=0;
    RemasterAudio::Category Category=RemasterAudio::Category::Sfx;
    float Gain=1.f;
};
UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterAudioSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") void SetProfile(URemasterAudioProfile* InProfile);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") void SetPack(ERemasterAudioPack Pack);
    UFUNCTION(BlueprintPure, Category="Remaster|Audio") ERemasterAudioPack GetPack() const { return SelectedPack; }
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") void SetSettings(FRemasterAudioSettings Settings);
    UFUNCTION(BlueprintPure, Category="Remaster|Audio") FRemasterAudioSettings GetSettings() const;
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") bool PlayMusic(FName Id, float FadeSeconds=.5f);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") bool PlaySfx(FName Id);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") bool PlayAmbience(FName Id);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") void StopMusic(float FadeSeconds=.5f);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") bool PlaySongId(int32 SourceId);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") bool PlayFanfareId(int32 SourceId);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") bool PlayCry(int32 CoreSpecies, int32 Mode=0, bool bDuckMusic=true);
    // Android owner delivers actual focus callbacks. Background and focus are independent.
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") void NotifyAudioFocus(bool bHasFocus);
    UFUNCTION(BlueprintCallable, Category="Remaster|Audio") void ResetPresentation();
    UFUNCTION(BlueprintPure, Category="Remaster|Audio") FString GetLastFailure() const { return LastFailure; }
    // Dispatch committed semantic requests only; this subsystem never completes core waits.
    bool PresentCoreRequest(const RemasterEmeraldScriptRequest& Request);
private:
    const FRemasterAudioAsset* Entry(FName Id) const;
    USoundBase* Resolve(FName Id,const FRemasterAudioAsset*& Asset);
    bool PlayVoice(FName Id,RemasterAudio::Category Category,unsigned Priority);
    bool Dispatch(const RemasterAudio::Request& Request);
    bool Tick(float DeltaSeconds);
    void Background();
    void Foreground();
    void ApplyPause();
    void ApplyVolumes();
    void Prune();
    void SaveSettings();
    bool Fail(const FString& Message) { LastFailure=Message;return false; }
    UPROPERTY() TObjectPtr<URemasterAudioProfile> Profile;
    UPROPERTY() TObjectPtr<UAudioComponent> MusicComponent;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> RetiringMusic;
    UPROPERTY() TArray<FRemasterAudioVoice> Voices;
    ERemasterAudioPack SelectedPack=ERemasterAudioPack::Original;
    RemasterAudio::Settings Levels;
    RemasterAudio::Policy Policy;
    uint64 NextVoice=0;
    float MusicGain=1.f;
    bool bCryDuck=false;
    FString LastFailure;
    FDelegateHandle BackgroundHandle,ForegroundHandle;
    FTSTicker::FDelegateHandle TickHandle;
};
