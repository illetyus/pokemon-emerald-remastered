#include "RemasterGameSessionSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/CoreDelegates.h"
#include "RemasterAudioSubsystem.h"
#include "RemasterCoreSubsystem.h"
#include "RemasterInputSubsystem.h"
#include "RemasterOverworldPawn.h"
#include "RemasterUISubsystem.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterWorldActor.h"

void URemasterGameSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Collection.InitializeDependency<URemasterVanillaPlusSaveSubsystem>();
    Collection.InitializeDependency<URemasterWorldGameplaySubsystem>();
    Collection.InitializeDependency<URemasterInputSubsystem>();
    Super::Initialize(Collection);
    auto* Save=GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    Menu.status=static_cast<RemasterEmeraldSaveStatus>(Save->GetStatus());Menu.Build();
    BackgroundHandle=FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddLambda([this](){bBackground=true;Accumulator=0;});
    ForegroundHandle=FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddLambda([this](){bBackground=false;Accumulator=0;});
    TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&URemasterGameSessionSubsystem::TickSession));
    UE_LOG(LogTemp,Display,TEXT("AP04_TITLE_READY status=%d"),static_cast<int32>(Menu.status));
}
void URemasterGameSessionSubsystem::Deinitialize() {
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    Session.Cancel();Super::Deinitialize();
}
void URemasterGameSessionSubsystem::Fail(const FString& Message) {
    Session.Fail(Session.generation);
    Menu.notice=TCHAR_TO_UTF8(*Message);
    if(auto* Save=GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>())Menu.status=static_cast<RemasterEmeraldSaveStatus>(Save->GetStatus());
    Menu.Show(RemasterSession::MenuScreen::Error);
    if(auto* PC=GetGameInstance()->GetFirstLocalPlayerController())if(PC->PlayerCameraManager)PC->PlayerCameraManager->StopCameraFade();
    if(auto* Input=GetGameInstance()->GetSubsystem<URemasterInputSubsystem>())Input->ResetInputs();
    UE_LOG(LogTemp,Error,TEXT("AP04_SESSION_ERROR generation=%llu reason=%s"),Session.generation,*Message);
}
void URemasterGameSessionSubsystem::BackToTitle() {
    if(IsBusy())return;
    Session.Cancel();
    if(auto* UI=GetGameInstance()->GetSubsystem<URemasterUISubsystem>())UI->DetachCoreDialogue();
    if(auto* Input=GetGameInstance()->GetSubsystem<URemasterInputSubsystem>())Input->ResetInputs();
    auto* Save=GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    Menu.status=static_cast<RemasterEmeraldSaveStatus>(Save->GetStatus());Menu.notice.clear();Menu.Show(RemasterSession::MenuScreen::Title);
}
bool URemasterGameSessionSubsystem::BeginLoad() {
    if(!Session.Begin())return false;
    GetGameInstance()->GetSubsystem<URemasterUISubsystem>()->DetachCoreDialogue();
    GetGameInstance()->GetSubsystem<URemasterInputSubsystem>()->ResetInputs();
    Menu.Show(RemasterSession::MenuScreen::Busy);return true;
}
bool URemasterGameSessionSubsystem::Activate(unsigned Row,uint64 Revision) {
    if(IsBusy() || IsPlaying())return false;
    const auto Command=Menu.Activate(Row,Revision);
    using C=RemasterSession::MenuCommand;
    if(Command==C::None)return true;
    if(!BeginLoad())return false;
    auto* Save=GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    FString Error;
    if(Command==C::Start) {
        // Capture entropy once at the ownership boundary. Renderer/ticks cannot
        // reseed or change the source trainer ID while creating the checkpoint.
        const uint64 Cycles=FPlatformTime::Cycles64();
        RemasterEmeraldNewGameRuntime Runtime{};
        if(!Save->CreateNewGame(UTF8_TO_TCHAR(Menu.name.c_str()),Menu.avatar,
            static_cast<uint32>(Cycles>>16),static_cast<uint16>(Cycles),Runtime,Error)) {Fail(Error);return false;}
        GetGameInstance()->GetSubsystem<URemasterCoreSubsystem>()->ResetCore();
        GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>()->SetEncounterSeed(Runtime.rng_state);
        GetGameInstance()->GetSubsystem<URemasterAudioSubsystem>()->ResetPresentation();
        return LoadWorld(true);
    }
    if(Command==C::ImportCopy) {
        if(!Save->ImportCopy(UTF8_TO_TCHAR(Menu.path.c_str()),static_cast<uint8>(Menu.format),Error)){Fail(Error);return false;}
    } else if(Command!=C::Continue || !RemasterSession::CanContinue(static_cast<RemasterEmeraldSaveStatus>(Save->LoadLegacySave()))) {
        Fail(TEXT("Kayıt okunamadı veya devam etmeye uygun değil."));return false;
    }
    return LoadWorld(false);
}
bool URemasterGameSessionSubsystem::LoadWorld(bool NewGame) {
    auto* Save=GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    auto* Gameplay=GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>();
    if(!Gameplay->LoadCurrentMapFromSave(true) || !Gameplay->IsMapReady()) {Fail(TEXT("Kayıt yüklendi; harita açılamadı."));return false;}
    bool Rendered=false;
    for(TActorIterator<ARemasterWorldActor> It(GetWorld());It;++It)Rendered=It->LoadAuthoritativeMap()||Rendered;
    auto* PC=GetGameInstance()->GetFirstLocalPlayerController();
    auto* Pawn=PC?Cast<ARemasterOverworldPawn>(PC->GetPawn()):nullptr;
    if(!Rendered || !Pawn || !Pawn->SyncFromAuthoritativeState()){Fail(TEXT("Harita veya karakter sunumu hazırlanamadı."));return false;}
    const auto Status=static_cast<RemasterEmeraldSaveStatus>(Save->GetStatus());
    Menu.status=Status;
    if(NewGame) {
        if(!PC->PlayerCameraManager || !SetDoor(false) || !Session.FinishIntro(Session.generation,Status,true)) {
            Fail(TEXT("Kamyon giriş sahnesi başlatılamadı."));return false;
        }
        Truck={};IntroGeneration=Session.generation;Accumulator=0;
        PC->PlayerCameraManager->SetManualCameraFade(1,FLinearColor::Black,false);
        Menu.Show(RemasterSession::MenuScreen::Intro);
        UE_LOG(LogTemp,Display,TEXT("AP04_TRUCK_INTRO_BEGIN generation=%llu"),IntroGeneration);
    } else {
        // Source ON_LOAD makes a saved truck's exit open without replaying the
        // one-time moving-truck callback. Continue always uses persisted map.
        if(RemasterSession::IsTruckMap(TCHAR_TO_UTF8(*Gameplay->GetCurrentMapId())) && !SetDoor(true)) {Fail(TEXT("Kamyon çıkışı hazırlanamadı."));return false;}
        if(!Session.Finish(Session.generation,Status,true))return false;
        UE_LOG(LogTemp,Display,TEXT("AP04_CONTINUE_PLAYING generation=%llu map=%s"),Session.generation,*Gameplay->GetCurrentMapId());
    }
    return true;
}
bool URemasterGameSessionSubsystem::ReloadCurrent() {
    if(!BeginLoad())return false;
    auto* Save=GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    if(!RemasterSession::CanContinue(static_cast<RemasterEmeraldSaveStatus>(Save->LoadLegacySave()))) {
        Fail(TEXT("Kayıt yeniden okunamadı."));return false;
    }
    return LoadWorld(false);
}
bool URemasterGameSessionSubsystem::SetDoor(bool Open) {
    auto* Gameplay=GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>();
    return Gameplay && Gameplay->SetTruckDoor(Open);
}
void URemasterGameSessionSubsystem::PlayTruckSound(int32 SongId) {
    if(auto* Audio=GetGameInstance()->GetSubsystem<URemasterAudioSubsystem>())
        if(!Audio->PlaySongId(SongId))UE_LOG(LogTemp,Warning,TEXT("AP04_TRUCK_AUDIO_UNAVAILABLE song=%d reason=%s"),SongId,*Audio->GetLastFailure());
}
bool URemasterGameSessionSubsystem::TickSession(float DeltaSeconds) {
    if(Session.state!=RemasterSession::State::Intro || bBackground || !GetWorld() || GetWorld()->IsPaused())return true;
    if(IntroGeneration!=Session.generation)return true;
    auto* PC=GetGameInstance()->GetFirstLocalPlayerController();
    auto* Camera=PC?PC->PlayerCameraManager.Get():nullptr;
    auto* Gameplay=GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>();
    if(!Camera || !Gameplay->IsMapReady() || !RemasterSession::IsTruckMap(TCHAR_TO_UTF8(*Gameplay->GetCurrentMapId()))) {Fail(TEXT("Giriş sahnesinin haritası veya kamerası kayboldu."));return true;}
    Accumulator+=FMath::Clamp(static_cast<double>(DeltaSeconds),0.0,0.1);
    while(Accumulator>=1.0/60.0 && Session.state==RemasterSession::State::Intro) {
        Accumulator-=1.0/60.0;
        const unsigned Effect=Truck.Tick(Camera->FadeAmount>0.001f && Truck.state>=2);
        using namespace RemasterSession;
        if(Effect&TruckMove)PlayTruckSound(49);
        if(Effect&TruckFadeIn)Camera->StartCameraFade(1,0,32.0f/60.0f,FLinearColor::Black,false,false);
        if(Effect&TruckStop)PlayTruckSound(50);
        if(Effect&TruckUnload)PlayTruckSound(51);
        if(Effect&TruckDoor) {
            if(!SetDoor(true) || !Session.CompleteIntro(IntroGeneration,true)){Fail(TEXT("Kamyon kapısı açılamadı."));break;}
            Camera->StopCameraFade();PlayTruckSound(52);
            GetGameInstance()->GetSubsystem<URemasterInputSubsystem>()->ResetInputs();
            UE_LOG(LogTemp,Display,TEXT("AP04_TRUCK_INTRO_COMPLETE generation=%llu frames=%u"),IntroGeneration,Truck.frames);
        }
    }
    return true;
}
FVector URemasterGameSessionSubsystem::GetTruckCameraOffset() const {
    if(Session.state!=RemasterSession::State::Intro)return FVector::ZeroVector;
    return FVector(Truck.cameraX*6.25,0,-Truck.cameraY*6.25);
}
FVector URemasterGameSessionSubsystem::GetTruckBoxOffset(int32 LocalId) const {
    if(LocalId<1 || LocalId>3)return FVector::ZeroVector;
    if(Session.state==RemasterSession::State::Intro)
        return FVector(Truck.boxX[LocalId-1]*6.25,0,-Truck.boxY[LocalId-1]*6.25);
    const auto* Gameplay=GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>();
    if(Session.Playing() && Gameplay && RemasterSession::IsTruckMap(TCHAR_TO_UTF8(*Gameplay->GetCurrentMapId()))) {
        const int X[3]={3,0,-3},Y[3]={3,-3,0};
        return FVector(X[LocalId-1]*6.25,0,-Y[LocalId-1]*6.25);
    }
    return FVector::ZeroVector;
}
