#include "RemasterAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Sound/SoundBase.h"
void URemasterAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    if (GConfig) {
        GConfig->GetFloat(TEXT("Remaster.Audio"),TEXT("Master"),Levels.master,GGameUserSettingsIni);
        GConfig->GetFloat(TEXT("Remaster.Audio"),TEXT("Music"),Levels.music,GGameUserSettingsIni);
        GConfig->GetFloat(TEXT("Remaster.Audio"),TEXT("Sfx"),Levels.sfx,GGameUserSettingsIni);
        GConfig->GetFloat(TEXT("Remaster.Audio"),TEXT("Ambience"),Levels.ambience,GGameUserSettingsIni);
        int32 Pack=0;GConfig->GetInt(TEXT("Remaster.Audio"),TEXT("Pack"),Pack,GGameUserSettingsIni);
        SelectedPack=Pack==1?ERemasterAudioPack::Modern:ERemasterAudioPack::Original;
    }
    Levels.Sanitize();
    BackgroundHandle=FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&URemasterAudioSubsystem::Background);
    ForegroundHandle=FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,&URemasterAudioSubsystem::Foreground);
    TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&URemasterAudioSubsystem::Tick),.1f);
}
void URemasterAudioSubsystem::Deinitialize() {
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    ResetPresentation();Profile=nullptr;Super::Deinitialize();
}
void URemasterAudioSubsystem::SaveSettings() {
    if (!GConfig) return;
    GConfig->SetFloat(TEXT("Remaster.Audio"),TEXT("Master"),Levels.master,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Remaster.Audio"),TEXT("Music"),Levels.music,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Remaster.Audio"),TEXT("Sfx"),Levels.sfx,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Remaster.Audio"),TEXT("Ambience"),Levels.ambience,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Remaster.Audio"),TEXT("Pack"),SelectedPack==ERemasterAudioPack::Modern?1:0,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void URemasterAudioSubsystem::SetSettings(FRemasterAudioSettings Settings) {
    Levels={Settings.Master,Settings.Music,Settings.Sfx,Settings.Ambience};Levels.Sanitize();ApplyVolumes();SaveSettings();
}
FRemasterAudioSettings URemasterAudioSubsystem::GetSettings() const {
    FRemasterAudioSettings S;S.Master=Levels.master;S.Music=Levels.music;S.Sfx=Levels.sfx;S.Ambience=Levels.ambience;return S;
}
void URemasterAudioSubsystem::SetProfile(URemasterAudioProfile* InProfile) { ResetPresentation();Profile=InProfile; }
void URemasterAudioSubsystem::SetPack(ERemasterAudioPack Pack) {
    if (Pack!=ERemasterAudioPack::Original && Pack!=ERemasterAudioPack::Modern) return;
    if (SelectedPack!=Pack) { ResetPresentation();SelectedPack=Pack;SaveSettings(); }
}
const FRemasterAudioAsset* URemasterAudioSubsystem::Entry(FName Id) const {
    if (!Profile) return nullptr;
    if (SelectedPack==ERemasterAudioPack::Modern) {
        const auto* A=Profile->Modern.Find(Id);
        if (A && A->bImportValidated && !A->Sound.IsNull()) return A;
        if (!Profile->bAllowOriginalFallback) return nullptr;
    }
    return Profile->Original.Find(Id);
}
USoundBase* URemasterAudioSubsystem::Resolve(FName Id,const FRemasterAudioAsset*& Asset) {
    Asset=Entry(Id);
    if (!Asset || !Asset->bImportValidated || Asset->Sound.IsNull() || Asset->Frames<=0 || Asset->SampleRate!=48000 ||
        !FMath::IsFinite(Asset->Gain) || Asset->Gain<0.f || Asset->Gain>4.f ||
        !FMath::IsFinite(Asset->CrossfadeSeconds) || Asset->CrossfadeSeconds<0.f || Asset->CrossfadeSeconds>10.f ||
        (Asset->bLoop && (!Asset->bLoopValidated || Asset->LoopStartFrame<0 || Asset->LoopEndFrame<=Asset->LoopStartFrame || Asset->LoopEndFrame>Asset->Frames))) {
        Fail(FString::Printf(TEXT("Missing or unvalidated audio: %s"),*Id.ToString()));return nullptr;
    }
    USoundBase* Sound=Asset->Sound.LoadSynchronous();
    if (!Sound || Sound->IsLooping()!=Asset->bLoop) { Fail(FString::Printf(TEXT("Audio load/loop contract failed: %s"),*Id.ToString()));return nullptr; }
    return Sound;
}
void URemasterAudioSubsystem::Prune() {
    for (int32 i=Voices.Num()-1;i>=0;--i) {
        if (!IsValid(Voices[i].Component) || (!Policy.Suspended() && !Voices[i].Component->IsPlaying())) {
            Policy.Finished(Voices[i].Id);Voices.RemoveAt(i);
        }
    }
    RetiringMusic.RemoveAll([this](const TObjectPtr<UAudioComponent>& C){return !IsValid(C)||(!Policy.Suspended()&&!C->IsPlaying());});
    bool AnyCry=false;
    for (const auto& Voice:Voices) AnyCry|=Voice.Category==RemasterAudio::Category::Cry;
    if (!AnyCry && bCryDuck) { bCryDuck=false;ApplyVolumes(); }
}
bool URemasterAudioSubsystem::Tick(float DeltaSeconds) { (void)DeltaSeconds;Prune();return true; }
bool URemasterAudioSubsystem::PlayMusic(FName Id,float FadeSeconds) {
    Prune();if (Policy.Suspended()||!GetWorld()) return Fail(TEXT("Audio suspended or world unavailable"));
    if (!FMath::IsFinite(FadeSeconds)||FadeSeconds<0.f||FadeSeconds>10.f) return Fail(TEXT("Invalid crossfade"));
    const FRemasterAudioAsset* A=nullptr;USoundBase* Sound=Resolve(Id,A);if (!Sound) return false;
    UAudioComponent* New=UGameplayStatics::SpawnSound2D(GetWorld(),Sound,0.f,1.f,0.f,nullptr,false,true);
    if (!New) return Fail(TEXT("Music spawn failed"));
    if (RetiringMusic.Num()>=2) { if (IsValid(RetiringMusic[0])) RetiringMusic[0]->Stop();RetiringMusic.RemoveAt(0); }
    if (IsValid(MusicComponent)) {
        if (FadeSeconds>0.f) { MusicComponent->FadeOut(FadeSeconds,0.f);RetiringMusic.Add(MusicComponent); }
        else MusicComponent->Stop();
    }
    MusicComponent=New;MusicGain=A->Gain;
    float Gain=Levels.Gain(RemasterAudio::Category::Music)*MusicGain*(bCryDuck?85.f/256.f:1.f);
    if (FadeSeconds>0.f) MusicComponent->FadeIn(FadeSeconds,Gain);else MusicComponent->SetVolumeMultiplier(Gain);
    LastFailure.Reset();return true;
}
bool URemasterAudioSubsystem::PlayVoice(FName Id,RemasterAudio::Category Category,unsigned Priority) {
    Prune();if (Policy.Suspended()||!GetWorld()) return Fail(TEXT("Audio suspended or world unavailable"));
    const FRemasterAudioAsset* A=nullptr;USoundBase* Sound=Resolve(Id,A);if (!Sound) return false;
    if (Category!=RemasterAudio::Category::Ambience && A->bLoop) return Fail(TEXT("One shot cannot loop"));
    if (Category==RemasterAudio::Category::Ambience) for (int32 i=Voices.Num()-1;i>=0;--i) if (Voices[i].Category==Category) {
        if (IsValid(Voices[i].Component)) Voices[i].Component->Stop();Policy.Finished(Voices[i].Id);Voices.RemoveAt(i);
    }
    uint64 IdNumber=++NextVoice;auto Admit=Policy.Admit(IdNumber,Priority);
    if (!Admit.accepted) return Fail(TEXT("Audio voice budget exhausted"));
    if (Admit.evicted) for (int32 i=Voices.Num()-1;i>=0;--i) if (Voices[i].Id==Admit.evicted) {
        if (IsValid(Voices[i].Component)) Voices[i].Component->Stop();Voices.RemoveAt(i);
    }
    UAudioComponent* C=UGameplayStatics::SpawnSound2D(GetWorld(),Sound,Levels.Gain(Category)*A->Gain,1.f,0.f,nullptr,false,true);
    if (!C) { Policy.Finished(IdNumber);return Fail(TEXT("Audio spawn failed")); }
    FRemasterAudioVoice V;V.Component=C;V.Id=IdNumber;V.Category=Category;V.Gain=A->Gain;Voices.Add(V);LastFailure.Reset();return true;
}
bool URemasterAudioSubsystem::PlaySfx(FName Id) { return PlayVoice(Id,RemasterAudio::Category::Sfx,5); }
bool URemasterAudioSubsystem::PlayAmbience(FName Id) { return PlayVoice(Id,RemasterAudio::Category::Ambience,1); }
bool URemasterAudioSubsystem::Dispatch(const RemasterAudio::Request& Request) {
    if (Request.stopMusic) { StopMusic(0.f);LastFailure.Reset();return true; }
    FName Id(UTF8_TO_TCHAR(Request.identity.c_str()));
    if (Request.category==RemasterAudio::Category::Music) { const auto* A=Entry(Id);return PlayMusic(Id,A?A->CrossfadeSeconds:.5f); }
    return PlayVoice(Id,Request.category,5);
}
bool URemasterAudioSubsystem::PlaySongId(int32 SourceId) {
    RemasterAudio::Request R;if (SourceId<0||!RemasterAudio::Song(static_cast<unsigned>(SourceId),R)) return Fail(TEXT("Unknown source song ID"));return Dispatch(R);
}
bool URemasterAudioSubsystem::PlayFanfareId(int32 SourceId) {
    RemasterAudio::Request R;
    if (SourceId<0||!RemasterAudio::Song(static_cast<unsigned>(SourceId),R)||R.category!=RemasterAudio::Category::Jingle) return Fail(TEXT("Unknown source fanfare song ID"));
    return Dispatch(R);
}
bool URemasterAudioSubsystem::PresentCoreRequest(const RemasterEmeraldScriptRequest& Request) {
    RemasterAudio::Request R;
    if (RemasterAudio::Script(Request,R)!=RemasterAudio::ScriptRead::Ready) return Fail(TEXT("Wait/non-audio/unsupported request remains owner responsibility"));
    return Dispatch(R);
}
bool URemasterAudioSubsystem::PlayCry(int32 CoreSpecies,int32 Mode,bool bDuckMusic) {
    RemasterAudio::Request R;
    if (CoreSpecies<=0||Mode<0||!RemasterAudio::Cry(static_cast<unsigned>(CoreSpecies),static_cast<unsigned>(Mode),R)) return Fail(TEXT("Unknown species/cry mode"));
    if (!PlayVoice(FName(UTF8_TO_TCHAR(R.identity.c_str())),RemasterAudio::Category::Cry,10)) return false;
    if (bDuckMusic && Mode!=1) { bCryDuck=true;ApplyVolumes(); }return true;
}
void URemasterAudioSubsystem::StopMusic(float FadeSeconds) {
    if (!IsValid(MusicComponent)) { MusicComponent=nullptr;return; }
    if (FMath::IsFinite(FadeSeconds) && FadeSeconds>0.f) {
        if (RetiringMusic.Num()>=2) { if (IsValid(RetiringMusic[0])) RetiringMusic[0]->Stop();RetiringMusic.RemoveAt(0); }
        MusicComponent->FadeOut(FMath::Min(FadeSeconds,10.f),0.f);RetiringMusic.Add(MusicComponent);
    } else MusicComponent->Stop();
    MusicComponent=nullptr;
}
void URemasterAudioSubsystem::ApplyVolumes() {
    if (IsValid(MusicComponent)) MusicComponent->SetVolumeMultiplier(Levels.Gain(RemasterAudio::Category::Music)*MusicGain*(bCryDuck?85.f/256.f:1.f));
    for (auto& Voice:Voices) if (IsValid(Voice.Component)) Voice.Component->SetVolumeMultiplier(Levels.Gain(Voice.Category)*Voice.Gain);
}
void URemasterAudioSubsystem::ApplyPause() {
    if (IsValid(MusicComponent)) MusicComponent->SetPaused(Policy.Suspended());
    for (auto& C:RetiringMusic) if (IsValid(C)) C->SetPaused(Policy.Suspended());
    for (auto& Voice:Voices) if (IsValid(Voice.Component)) Voice.Component->SetPaused(Policy.Suspended());
}
void URemasterAudioSubsystem::Background() { Policy.Background(true);ApplyPause(); }
void URemasterAudioSubsystem::Foreground() { Policy.Background(false);ApplyPause(); }
void URemasterAudioSubsystem::NotifyAudioFocus(bool bHasFocus) { Policy.Focus(bHasFocus);ApplyPause(); }
void URemasterAudioSubsystem::ResetPresentation() {
    if (IsValid(MusicComponent)) MusicComponent->Stop();
    MusicComponent=nullptr;
    for (auto& C:RetiringMusic) if (IsValid(C)) C->Stop();
    RetiringMusic.Reset();
    for (auto& Voice:Voices) if (IsValid(Voice.Component)) Voice.Component->Stop();
    Voices.Reset();Policy.Reset();bCryDuck=false;LastFailure.Reset();
}
