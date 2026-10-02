#include "RemasterAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RemasterAudioProfile.h"
#include "Sound/SoundBase.h"

void URemasterAudioSubsystem::SetProfile(
    URemasterAudioProfile* InProfile)
{
    Profile = InProfile;
}

USoundBase* URemasterAudioSubsystem::Resolve(
    const TMap<FName, TSoftObjectPtr<USoundBase>>& Table,
    FName Id) const
{
    const TSoftObjectPtr<USoundBase>* Entry = Table.Find(Id);
    return Entry ? Entry->LoadSynchronous() : nullptr;
}

bool URemasterAudioSubsystem::PlayMusic(
    FName Id,
    float FadeSeconds)
{
    if (!Profile || !GetWorld())
    {
        return false;
    }

    USoundBase* Sound = Resolve(Profile->Music, Id);
    if (!Sound)
    {
        return false;
    }

    if (MusicComponent)
    {
        MusicComponent->FadeOut(
            FMath::Max(0.0f, FadeSeconds),
            0.0f);
    }

    MusicComponent = UGameplayStatics::SpawnSound2D(
        GetWorld(),
        Sound,
        1.0f,
        1.0f,
        0.0f,
        nullptr,
        true,
        false);

    if (MusicComponent && FadeSeconds > 0.0f)
    {
        MusicComponent->FadeIn(FadeSeconds, 1.0f);
    }

    return MusicComponent != nullptr;
}

bool URemasterAudioSubsystem::PlaySfx(FName Id)
{
    if (!Profile || !GetWorld())
    {
        return false;
    }

    USoundBase* Sound = Resolve(
        Profile->SoundEffects,
        Id);

    if (!Sound)
    {
        return false;
    }

    UGameplayStatics::PlaySound2D(
        GetWorld(),
        Sound);

    return true;
}

void URemasterAudioSubsystem::StopMusic(float FadeSeconds)
{
    if (!MusicComponent)
    {
        return;
    }

    MusicComponent->FadeOut(
        FMath::Max(0.0f, FadeSeconds),
        0.0f);
    MusicComponent = nullptr;
}
