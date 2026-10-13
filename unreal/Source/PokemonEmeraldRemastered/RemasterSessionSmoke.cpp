#include "RemasterSessionSmoke.h"
#include "CoreMinimal.h"
#if !UE_BUILD_SHIPPING
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "RemasterGameSessionSubsystem.h"
#include "RemasterUISubsystem.h"
#include "RemasterUiWidget.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "remaster_ap04_build_tag.inc"

namespace {
struct FSessionSmoke : TSharedFromThis<FSessionSmoke> {
    TWeakObjectPtr<UWorld> World;
    FTimerHandle Timer;
    FString Case,Directory;
    int32 Phase=0,Ticks=0,PhaseTick=0;
    unsigned Checks=0,Failures=0;
    int64 OldRevision=0;
    uint64 BeforeGeneration=0;
    TArray<uint8> BeforeFile;
    FString BeforePath;
    bool bFinished=false;
    void Check(bool Ok,const TCHAR* Label) {
        ++Checks;if(!Ok)++Failures;
        UE_LOG(LogTemp,Display,TEXT("AP04_RUNTIME_CHECK %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Label);
    }
    URemasterUISubsystem* UI() const { return World.IsValid()?World->GetGameInstance()->GetSubsystem<URemasterUISubsystem>():nullptr; }
    URemasterGameSessionSubsystem* Session() const { return World.IsValid()?World->GetGameInstance()->GetSubsystem<URemasterGameSessionSubsystem>():nullptr; }
    URemasterVanillaPlusSaveSubsystem* Save() const { return World.IsValid()?World->GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>():nullptr; }
    void Screenshot(const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory,Name),true,false); }
    // Delivers the actual UMG OnClicked listener; no assertion of OS/hardware input.
    bool Click(const FString& Label) {
        auto* Widget=UI()?UI()->GetCurrentScreen():nullptr;
        if(!Widget || !Widget->WidgetTree)return false;
        TArray<UWidget*> Widgets;Widget->WidgetTree->GetAllWidgets(Widgets);
        for(auto* Item:Widgets) {
            auto* Row=Cast<URemasterUiRowWidget>(Item);if(!Row || !Row->WidgetTree)continue;
            TArray<UWidget*> Children;Row->WidgetTree->GetAllWidgets(Children);
            UButton* Button=nullptr;bool Found=false;
            for(auto* Child:Children) {
                if(auto* Text=Cast<UTextBlock>(Child))Found=Found||Text->GetText().ToString().Contains(Label);
                if(auto* Value=Cast<UButton>(Child))Button=Value;
            }
            if(Found && Button && Button->GetIsEnabled()){Button->OnClicked.Broadcast();return true;}
        }
        return false;
    }
    bool Edit(const FString& Value,bool Name) {
        auto* Widget=UI()?UI()->GetCurrentScreen():nullptr;if(!Widget || !Widget->WidgetTree)return false;
        TArray<UWidget*> Widgets;Widget->WidgetTree->GetAllWidgets(Widgets);
        for(auto* Item:Widgets)if(auto* Entry=Cast<UEditableTextBox>(Item)) {
            if(Entry->GetVisibility()!=ESlateVisibility::Visible)continue;
            if(Name!=Entry->GetHintText().ToString().Contains(TEXT("Adın")))continue;
            Entry->SetText(FText::FromString(Value));Entry->OnTextCommitted.Broadcast(Entry->GetText(),ETextCommit::OnEnter);return true;
        }
        return false;
    }
    bool SameFile() const {
        TArray<uint8> Now;return FFileHelper::LoadFileToArray(Now,*Save()->GetLegacySavePath()) && Now==BeforeFile;
    }
    void Next(){++Phase;PhaseTick=Ticks;}
    void Finish() {
        if(bFinished)return;bFinished=true;
        auto Report=MakeShared<FJsonObject>();
        Report->SetStringField(TEXT("Schema"),TEXT("remaster.ap04.actual-session-smoke"));
        Report->SetStringField(TEXT("Case"),Case);Report->SetStringField(TEXT("CompiledHead"),TEXT(REMASTER_AP04_HEAD));
        Report->SetStringField(TEXT("CompiledInputsSHA256"),TEXT(REMASTER_AP04_INPUT_SHA));
        Report->SetBoolField(TEXT("CompiledDirtyProbe"),REMASTER_AP04_DIRTY!=0);
        Report->SetBoolField(TEXT("Passed"),Failures==0);Report->SetNumberField(TEXT("Checks"),Checks);
        Report->SetNumberField(TEXT("Failures"),Failures);Report->SetBoolField(TEXT("ActualUMGDelegateDelivery"),true);
        Report->SetBoolField(TEXT("PhysicalInputVerified"),false);Report->SetBoolField(TEXT("FullStoryVerified"),false);
        Report->SetStringField(TEXT("SavedDirectory"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));
        Report->SetStringField(TEXT("ActiveSavePath"),Save()?Save()->GetLegacySavePath():TEXT(""));
        FString Json;const auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
        FFileHelper::SaveStringToFile(Json,*FPaths::Combine(Directory,TEXT("receipt.json")));
        UE_LOG(LogTemp,Display,TEXT("AP04_RUNTIME_SESSION_RESULT case=%s checks=%u failures=%u compiled=%s"),*Case,Checks,Failures,TEXT(REMASTER_AP04_HEAD));
        if(FParse::Param(FCommandLine::Get(),TEXT("AP04SmokeExit")))FPlatformMisc::RequestExit(false);
        if(World.IsValid())World->GetTimerManager().ClearTimer(Timer);
    }
    void Tick() {
        ++Ticks;
        if(bFinished)return;
        if(Ticks>5400){Check(false,TEXT("session smoke timeout"));Finish();return;}
        if(!UI() || !Session() || !Save() || !UI()->GetCurrentScreen())return;
        auto* Gameplay=World->GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>();
        if(Phase==0 && Ticks>30) {
            Check(UI()->GetFrame().ScreenId==1000,TEXT("startup title is the production UMG frontend"));
            Check(!Session()->IsPlaying()&&UI()->BlocksWorldInput(),TEXT("boot does not enable gameplay"));
            Screenshot(TEXT("title.png"));OldRevision=UI()->GetFrame().Revision;Next();return;
        }
        if(Phase==1 && Ticks-PhaseTick>30) {
            if(Case==TEXT("corrupt")) {
                Check(Save()->GetStatus()==ERemasterLegacySaveStatus::Corrupt,TEXT("found corrupt file is not absence"));
                const auto Frame=UI()->GetFrame();Check(!Frame.Rows[0].bEnabled&&!Frame.Rows[1].bEnabled,TEXT("corrupt file blocks Continue and New Game"));
                Check(FFileHelper::LoadFileToArray(BeforeFile,*Save()->GetLegacySavePath()),TEXT("negative fixture read"));
                UI()->ActivateRow(1,Frame.Revision);Check(SameFile(),TEXT("disabled New Game preserves actual corrupt file"));
                Screenshot(TEXT("corrupt.png"));Phase=90;PhaseTick=Ticks;return;
            }
            if(Case==TEXT("continue")) {
                Check(Save()->HasUsableSave(),TEXT("persisted selection and checkpoint reopened"));
                Check(!UI()->GetFrame().Rows[1].bEnabled,TEXT("existing save keeps New Game disabled"));
                Check(FFileHelper::LoadFileToArray(BeforeFile,*Save()->GetLegacySavePath()),TEXT("existing working-copy bytes captured"));
                Check(Click(TEXT("Devam Et")),TEXT("actual UMG Continue listener delivered"));
                Check(Session()->IsPlaying()&&Gameplay->IsMapReady(),TEXT("Continue publishes only a ready map"));
                Check(SameFile(),TEXT("Continue does not rewrite actual input file"));
                Screenshot(TEXT("continue.png"));Phase=90;PhaseTick=Ticks;return;
            }
            Check(Save()->GetStatus()==ERemasterLegacySaveStatus::Empty,TEXT("fresh profile is typed MISSING"));
            Check(Click(TEXT("Yeni Oyun")),TEXT("actual UMG New Game listener delivered"));
            Check(!UI()->ActivateRow(1,OldRevision),TEXT("stale title listener rejected across form boundary"));
            Next();return;
        }
        if(Phase==2 && Ticks-PhaseTick>15) {
            Check(Edit(TEXT("Cemal"),true),TEXT("native UMG text event supplies valid source keyboard name"));
            Screenshot(TEXT("new-game.png"));Next();return;
        }
        if(Phase==3 && Ticks-PhaseTick>30) {
            OldRevision=UI()->GetFrame().Revision;
            Check(Click(TEXT("Yolculuğa Başla")),TEXT("actual UMG start listener delivered"));
            Check(Session()->IsBusy()&&!Session()->IsPlaying(),TEXT("map readiness waits for moving-truck completion"));
            Check(!UI()->ActivateRow(2,OldRevision),TEXT("late duplicate start cannot reinitialize or store twice"));
            FRemasterPlayerStepResult Step;Check(!Gameplay->StepPlayer(1,Step),TEXT("world API rejects movement during intro"));
            BeforeGeneration=Session()->GetGeneration();Next();return;
        }
        if(Phase==4) {
            if(!Session()->IsPlaying())return;
            Check(Session()->GetGeneration()==BeforeGeneration,TEXT("intro completed in its current session generation"));
            Check(Gameplay->GetCurrentMapId()==TEXT("InsideOfTruck"),TEXT("canonical truck destination retained"));
            FRemasterLegacyOverworldSnapshot Snapshot;
            Check(Save()->GetOverworldSnapshot(Snapshot)&&Snapshot.PlayerX==2&&Snapshot.PlayerY==2&&Snapshot.Money==3000&&Snapshot.PartyCount==0,TEXT("production core initial player/state snapshot"));
            FRemasterLegacyRtcSnapshot Clock;Check(Save()->GetLocalRtcNow(Clock)&&Clock.Days==1,TEXT("game-local RTC reset survives source clock read"));
            Screenshot(TEXT("truck-open.png"));Next();return;
        }
        if(Phase==5 && Ticks-PhaseTick>30) {
            UI()->SubmitAction(ERemasterUiAction::Menu);Check(Click(TEXT("Kayıt / Yükle")),TEXT("production save/load screen"));
            Check(Click(TEXT("Kaydet")),TEXT("production save confirmation"));Check(Click(TEXT("Onayla")),TEXT("production store delegate"));
            BeforePath=Save()->GetLegacySavePath();
            Check(FFileHelper::LoadFileToArray(BeforeFile,*BeforePath),TEXT("actual stored checkpoint captured"));
            Next();return;
        }
        if(Phase==6 && Ticks-PhaseTick>15) {
            UI()->SubmitAction(ERemasterUiAction::Cancel);UI()->SubmitAction(ERemasterUiAction::Cancel);UI()->SubmitAction(ERemasterUiAction::Cancel);
            UI()->SubmitAction(ERemasterUiAction::Menu);Check(Click(TEXT("Başlangıç menüsü")),TEXT("production return-title listener"));
            Check(!Session()->IsPlaying()&&UI()->BlocksWorldInput(),TEXT("title return fences gameplay"));
            Check(Click(TEXT("Devam Et")),TEXT("production Continue after save"));
            Check(Session()->IsPlaying()&&Session()->GetGeneration()>BeforeGeneration,TEXT("reopen owns a new session generation"));
            Check(SameFile(),TEXT("save-to-Continue roundtrip retains all actual image bytes"));Next();return;
        }
        if(Phase==7 && Ticks-PhaseTick>15) {
            Session()->BackToTitle();UI()->SubmitAction(ERemasterUiAction::Up);
            Check(Click(TEXT("Kayıt Aktar")),TEXT("production import form"));
            Check(Edit(Save()->GetLegacySavePath(),false),TEXT("native import path event"));
            Check(Click(TEXT("Vanilla+")),TEXT("explicit import format selection"));
            Check(Click(TEXT("Kopyayı Aktar")),TEXT("production copy-first import delegate"));
            Check(Session()->IsPlaying()&&Save()->GetLegacySavePath().Contains(TEXT("import-")),TEXT("import selects separate working copy"));
            Check(SameFile(),TEXT("imported working copy keeps byte-exact source image"));
            TArray<uint8> Original,Quarantine;
            Check(FFileHelper::LoadFileToArray(Original,*BeforePath)&&Original==BeforeFile,TEXT("actual import source file remains unchanged"));
            const FString RawCopy=FPaths::Combine(FPaths::GetPath(Save()->GetLegacySavePath()),TEXT("quarantine-")+FPaths::GetCleanFilename(Save()->GetLegacySavePath()));
            Check(FFileHelper::LoadFileToArray(Quarantine,*RawCopy)&&Quarantine==BeforeFile,TEXT("raw quarantined copy remains byte-exact"));
            Screenshot(TEXT("imported.png"));Phase=90;PhaseTick=Ticks;return;
        }
        if(Phase==90 && Ticks-PhaseTick>30)Finish();
    }
};
}
void StartRemasterSessionSmoke(UWorld* World) {
    FString Case;if(!World || !FParse::Value(FCommandLine::Get(),TEXT("AP04SessionSmoke="),Case))return;
    if(Case!=TEXT("new")&&Case!=TEXT("continue")&&Case!=TEXT("corrupt"))return;
    const auto Runner=MakeShared<FSessionSmoke>();Runner->World=World;Runner->Case=Case;
    Runner->Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SessionSmoke"));
    IFileManager::Get().MakeDirectory(*Runner->Directory,true);
    World->GetTimerManager().SetTimer(Runner->Timer,FTimerDelegate::CreateLambda([Runner](){Runner->Tick();}),1.0f/60.0f,true);
}
#else
void StartRemasterSessionSmoke(UWorld*) {}
#endif
