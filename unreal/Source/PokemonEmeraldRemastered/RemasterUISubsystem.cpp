#include "RemasterUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"
#include "RemasterUiWidget.h"
#include "RemasterOverworldPawn.h"
#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterInputSubsystem.h"

void URemasterUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<URemasterVanillaPlusSaveSubsystem>();
    Collection.InitializeDependency<URemasterWorldGameplaySubsystem>();
    Collection.InitializeDependency<URemasterInputSubsystem>();
    Super::Initialize(Collection);
    if (GConfig)
    {
        GConfig->GetBool(TEXT("Remaster.UI"), TEXT("FastText"), Model.fastText, GGameUserSettingsIni);
        GConfig->GetBool(TEXT("Remaster.UI"), TEXT("MenuRepeat"), Model.repeatEnabled, GGameUserSettingsIni);
    }
}

void URemasterUISubsystem::Deinitialize()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PresentationTimer);
    DetachCoreDialogue();
    HideCurrentScreen();
    Super::Deinitialize();
}

void URemasterUISubsystem::StartNativePresentation()
{
    if (!GetGameInstance() || !GetWorld()) return;
    ShowScreen(URemasterUiWidget::StaticClass(), 10);
    // Gamepad/keyboard focus belongs to one abstract input route. UMG receives
    // pointer/touch clicks, preventing two independent confirm handlers.
    if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
    {
        PC->SetInputMode(FInputModeGameAndUI());
        PC->bShowMouseCursor = true;
    }
    LastTickSeconds = FPlatformTime::Seconds();
    GetWorld()->GetTimerManager().SetTimer(PresentationTimer, this,
        &URemasterUISubsystem::PresentationTick, 1.0f / 60.0f, true);
    Refresh();
}

RemasterEmeraldSave* URemasterUISubsystem::MutableSave() const
{
    auto* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>() : nullptr;
    return Save ? static_cast<RemasterEmeraldSave*>(Save->GetMutableNativeSaveHandle()) : nullptr;
}

RemasterUi::Context URemasterUISubsystem::ReadContext() const
{
    RemasterUi::Context Context;
    Context.scriptBusy = bScriptBusy || bIoBusy || DialogueRuntime != nullptr;
    Context.battleBusy = bBattleBusy;
    if (auto* Gameplay = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>() : nullptr)
    {
        Context.mapReady = Gameplay->IsMapReady();
        Context.mapGroup = Gameplay->GetCurrentMapGroup();
        Context.mapNum = Gameplay->GetCurrentMapNumber();
        Context.mapName = TCHAR_TO_UTF8(*Gameplay->GetCurrentMapId());
        if (const auto* Map = Gameplay->GetCurrentMapForPresentation())
            if (const char* Name = RemasterUi::RegionName(Map->RegionMapSectionId)) Context.mapName = Name;
    }
    return Context;
}

bool URemasterUISubsystem::BlocksWorldInput() const
{
    return Model.Modal() || !Model.CanAct(MutableSave(), ReadContext());
}
void URemasterUISubsystem::AdvanceInputBoundary()
{
    if (InputHostGeneration != MAX_uint64) ++InputHostGeneration;
    if (auto* Input=GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->ResetInputs();
}
RemasterControls::Context URemasterUISubsystem::ReadInputContext()
{
    RemasterControls::Context Out;
    const auto World=ReadContext();
    Out.saveUsable=RemasterUi::Usable(MutableSave());Out.mapReady=World.mapReady;
    Out.uiAttached=CurrentScreen!=nullptr;Out.uiModal=Model.Modal();
    Out.ioBusy=bIoBusy;Out.scriptBusy=bScriptBusy;Out.battleBusy=bBattleBusy;
    RemasterEmeraldScriptRequest Pending{};
    Out.dialoguePending=DialogueRuntime && Model.frame.screen==RemasterUi::Screen::Dialogue
        && remaster_emerald_script_runtime_pending_request(DialogueRuntime,&Pending)
        && (Pending.type==REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE
            || Pending.type==REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE);
    const uint64 Request=Out.dialoguePending ? Pending.sequence : 0;
    const int32 Screen=static_cast<int32>(Model.frame.screen);
    const FString MapKey=FString::Printf(TEXT("%d/%d/"),World.mapGroup,World.mapNum)
        + UTF8_TO_TCHAR(World.mapName.c_str());
    if (Request!=LastInputRequest || Screen!=LastInputScreen || MapKey!=LastInputMap)
    {
        AdvanceInputBoundary();LastInputRequest=Request;LastInputScreen=Screen;
        LastInputMap=MapKey;
    }
    Out.hostGeneration=InputHostGeneration;
    if (InputHostGeneration==MAX_uint64) Out.suspensionReasons=128; // Never wrap an owner fence.
    return Out;
}

void URemasterUISubsystem::SetHostBusy(bool bScript, bool bBattle)
{
    if (bScriptBusy!=bScript || bBattleBusy!=bBattle) AdvanceInputBoundary();
    bScriptBusy = bScript; bBattleBusy = bBattle;
    if (!bScript) DialogueRuntime = nullptr;
    if (bScript || bBattle) Model.Reset();
    else if (bRepelPromptPending)
    {
        Model.Open(RemasterUi::Screen::Repel);
        bRepelPromptPending = false;
    }
    Refresh();
}

void URemasterUISubsystem::NotifyCoreStep(bool bScriptPending, bool bEncounterPending, bool bRepelWoreOff)
{
    bRepelPromptPending = bRepelPromptPending || bRepelWoreOff;
    if (bScriptPending || bEncounterPending) SetHostBusy(bScriptPending, bEncounterPending);
    if (bRepelPromptPending && !bScriptBusy && !bBattleBusy)
    {
        Model.Open(RemasterUi::Screen::Repel);
        bRepelPromptPending = false;
    }
    Refresh();
}

bool URemasterUISubsystem::SubmitAction(ERemasterUiAction Action)
{
    if (bIoBusy) return true;
    Refresh();
    if (Action == ERemasterUiAction::Confirm && Model.Modal())
    {
        ActivateRow(static_cast<int32>(Model.frame.selected), static_cast<int64>(Model.frame.revision));
        return true;
    }
    const bool bConsumed = Model.Input(static_cast<RemasterUi::Action>(Action), MutableSave(), ReadContext());
    Refresh();
    return bConsumed;
}

bool URemasterUISubsystem::ActivateRow(int32 Index, int64 ExpectedRevision)
{
    if (bIoBusy || Index < 0 || ExpectedRevision < 0) return false;
    const bool OldFast = Model.fastText, OldRepeat = Model.repeatEnabled;
    const auto Intent = Model.Activate(static_cast<unsigned>(Index), static_cast<uint64>(ExpectedRevision),
        MutableSave(), ReadContext());
    using RemasterUi::Effect;
    auto* Save = GetGameInstance()->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    if (Intent.effect == Effect::Save || Intent.effect == Effect::Load)
    {
        bIoBusy = true;
        AdvanceInputBoundary();
        Model.notice = Intent.effect == Effect::Save ? "Kaydediliyor…" : "Yükleniyor…";
        Refresh(); // Explicit busy state, no invented percentage or success.
        if (Intent.effect == Effect::Save)
            Model.notice = Save && Save->StoreLegacySave() ? "Kayıt tamamlandı." : "Kayıt yazılamadı. Tekrar deneyebilirsin.";
        else
        {
            DetachCoreDialogue();
            bRepelPromptPending = false;
            const auto Status = Save ? Save->LoadLegacySave() : ERemasterLegacySaveStatus::IoError;
            Model.Reset();
            if (Status == ERemasterLegacySaveStatus::Ok || Status == ERemasterLegacySaveStatus::Degraded)
            {
                auto* Gameplay = GetGameInstance()->GetSubsystem<URemasterWorldGameplaySubsystem>();
                const bool bLoadedMap = Gameplay && Gameplay->LoadCurrentMapFromSave(true);
                if (auto* PC = GetGameInstance()->GetFirstLocalPlayerController())
                    if (auto* Pawn = Cast<ARemasterOverworldPawn>(PC->GetPawn())) Pawn->SyncFromAuthoritativeState();
                Model.notice = bLoadedMap ? "Kayıt yüklendi." : "Kayıt yüklendi; harita açılamadı.";
            }
            else
            {
                switch (Status)
                {
                case ERemasterLegacySaveStatus::Empty: Model.notice = "Kayıt dosyası bulunamadı."; break;
                case ERemasterLegacySaveStatus::Corrupt: Model.notice = "Kayıt dosyası bozuk."; break;
                case ERemasterLegacySaveStatus::Unsupported: Model.notice = "Kayıt biçimi desteklenmiyor."; break;
                default: Model.notice = "Kayıt dosyası okunamadı."; break;
                }
            }
            Model.Open(RemasterUi::Screen::SaveLoad);
        }
        bIoBusy = false;
        AdvanceInputBoundary();
    }
    else if (Intent.effect == Effect::ApplyRtc)
    {
        FRemasterLegacyRtcSnapshot Time;
        Time.Days = Intent.time.days; Time.Hours = Intent.time.hours;
        Time.Minutes = Intent.time.minutes; Time.Seconds = Intent.time.seconds;
        Model.notice = Save && Save->RealignRtcNow(Time) ? "Saat ayarı uygulandı." : "Saat ayarı uygulanamadı.";
    }
    else if (Intent.effect == Effect::Nickname)
    {
        if (OnNicknameEntry.IsBound()) OnNicknameEntry.Broadcast(Intent.value, static_cast<int64>(Intent.revision));
        else Model.notice = "Ad girişi şu anda kullanılamıyor.";
    }
    else if (Intent.effect == Effect::FieldItem)
    {
        if (OnFieldItemEntry.IsBound()) OnFieldItemEntry.Broadcast(Intent.value, static_cast<int64>(Intent.revision));
        else Model.notice = "Bu eşya şu anda kullanılamıyor.";
    }
    else if (Intent.effect == Effect::DialogueComplete)
    {
        if (!Model.CompleteDialogue(DialogueRuntime, Intent)) Model.notice = "Diyalog bekliyor.";
    }
    if (GConfig && (OldFast != Model.fastText || OldRepeat != Model.repeatEnabled))
    {
        GConfig->SetBool(TEXT("Remaster.UI"), TEXT("FastText"), Model.fastText, GGameUserSettingsIni);
        GConfig->SetBool(TEXT("Remaster.UI"), TEXT("MenuRepeat"), Model.repeatEnabled, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
    Refresh();
    return true;
}

bool URemasterUISubsystem::PresentCoreDialogue(RemasterEmeraldScriptRuntime* Runtime,
    const FString& ResolvedText, const TArray<FText>& ChoiceLabels, const TArray<int32>& ChoiceValues)
{
    RemasterEmeraldScriptRequest Request{};
    if (!Runtime || Runtime->vm.save != MutableSave() || ChoiceLabels.Num() != ChoiceValues.Num()
        || !remaster_emerald_script_runtime_pending_request(Runtime, &Request)) return false;
    std::vector<RemasterUi::Row> Choices;
    for (int32 i = 0; i < ChoiceLabels.Num(); ++i)
    {
        if (ChoiceValues[i] < 0 || ChoiceValues[i] > 65535) return false;
        Choices.push_back({TCHAR_TO_UTF8(*ChoiceLabels[i].ToString()),
            RemasterUi::Command::DialogueComplete, ChoiceValues[i], true});
    }
    if (!Model.PresentDialogue(Request, TCHAR_TO_UTF8(*ResolvedText), Choices)) return false;
    AdvanceInputBoundary();DialogueRuntime = Runtime; bScriptBusy = true;
    Refresh();
    return true;
}

void URemasterUISubsystem::DetachCoreDialogue()
{
    AdvanceInputBoundary();
    DialogueRuntime = nullptr;
    Model.Reset();
}

void URemasterUISubsystem::PresentationTick()
{
    const double Now = FPlatformTime::Seconds();
    // Bound catch-up to presentation only; no gameplay steps or asynchronous completions.
    TickAccumulator += FMath::Clamp(Now - LastTickSeconds, 0.0, 0.1);
    PollAccumulator += FMath::Clamp(Now - LastTickSeconds, 0.0, 0.1);
    LastTickSeconds = Now;
    bool bChanged = Model.frame.screen == RemasterUi::Screen::Dialogue;
    APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
    while (TickAccumulator >= 1.0 / 60.0)
    {
        TickAccumulator -= 1.0 / 60.0;
        Model.TextTick();
        const bool Up = PC && (PC->IsInputKeyDown(EKeys::Up) || PC->IsInputKeyDown(EKeys::W)
            || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up));
        const bool Down = PC && (PC->IsInputKeyDown(EKeys::Down) || PC->IsInputKeyDown(EKeys::S)
            || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down));
        if (RepeatUp.Tick(Up && Model.Modal(), Model.repeatEnabled))
        {
            Model.Input(RemasterUi::Action::Up, MutableSave(), ReadContext());
            bChanged = true;
        }
        if (RepeatDown.Tick(Down && Model.Modal(), Model.repeatEnabled))
        {
            Model.Input(RemasterUi::Action::Down, MutableSave(), ReadContext());
            bChanged = true;
        }
    }
    if (bChanged || PollAccumulator >= 0.25) { PollAccumulator = 0.0; Refresh(); }
}

void URemasterUISubsystem::Refresh()
{
    Model.Build(MutableSave(), ReadContext());
    const auto& Frame = Model.frame;
    FString Key = FString::Printf(TEXT("%llu/%d/%u/%d/%d/%d/%d/%d/%d/"),
        static_cast<unsigned long long>(Frame.revision), static_cast<int>(Frame.screen), Frame.selected,
        Frame.modal, Frame.hasMarker, Frame.markerX, Frame.markerY, Frame.markerWidth, Frame.markerHeight);
    Key += UTF8_TO_TCHAR(Frame.title.c_str()); Key += UTF8_TO_TCHAR(Frame.body.c_str());
    for (const auto& Row : Frame.rows) { Key += UTF8_TO_TCHAR(Row.label.c_str()); Key += Row.enabled ? TEXT("1") : TEXT("0"); }
    if (Key == LastFrameKey) return;
    LastFrameKey = Key;
    PresentedFrame.Title = FText::FromString(UTF8_TO_TCHAR(Frame.title.c_str()));
    PresentedFrame.Body = FText::FromString(UTF8_TO_TCHAR(Frame.body.c_str()));
    PresentedFrame.ScreenId = static_cast<int32>(Frame.screen);
    PresentedFrame.Selected = static_cast<int32>(Frame.selected);
    PresentedFrame.Revision = static_cast<int64>(Frame.revision);
    PresentedFrame.bModal = Frame.modal;
    PresentedFrame.bHasMarker = Frame.hasMarker;
    PresentedFrame.MarkerPosition = FVector2D(Frame.markerX, Frame.markerY);
    PresentedFrame.MarkerSize = FVector2D(Frame.markerWidth, Frame.markerHeight);
    PresentedFrame.Rows.Reset();
    for (const auto& Row : Frame.rows)
    {
        FRemasterUiRow& Out = PresentedFrame.Rows.AddDefaulted_GetRef();
        Out.Label = FText::FromString(UTF8_TO_TCHAR(Row.label.c_str())); Out.bEnabled = Row.enabled;
    }
    OnFrameChanged.Broadcast(PresentedFrame);
}

UUserWidget* URemasterUISubsystem::ShowScreen(
    TSubclassOf<UUserWidget> ScreenClass,
    int32 ZOrder)
{
    AdvanceInputBoundary();
    HideCurrentScreen();

    if (!ScreenClass || !GetGameInstance())
    {
        return nullptr;
    }

    CurrentScreen = CreateWidget<UUserWidget>(
        GetGameInstance(),
        ScreenClass);

    if (CurrentScreen)
    {
        CurrentScreen->AddToViewport(ZOrder);
    }

    return CurrentScreen;
}

void URemasterUISubsystem::HideCurrentScreen()
{
    AdvanceInputBoundary();
    if (CurrentScreen)
    {
        CurrentScreen->RemoveFromParent();
        CurrentScreen = nullptr;
    }
}
