#include "RemasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "RemasterInputConfig.h"
#include "RemasterUISubsystem.h"
#include "RemasterOverworldPawn.h"

extern "C"
{
#include "remaster/emerald_map.h"
}

void ARemasterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (InputConfig)
    {
        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                    LocalPlayer->GetSubsystem<
                        UEnhancedInputLocalPlayerSubsystem>())
            {
                if (UInputMappingContext* Context =
                        InputConfig->MappingContext.LoadSynchronous())
                {
                    Subsystem->AddMappingContext(Context, 0);
                }
            }
        }
    }

    if (ARemasterOverworldPawn* OverworldPawn =
            Cast<ARemasterOverworldPawn>(GetPawn()))
    {
        OverworldPawn->SyncFromAuthoritativeState();
    }
    if (auto* UI = GetGameInstance()->GetSubsystem<URemasterUISubsystem>())
        UI->StartNativePresentation();
}

void ARemasterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent)
        return;

    bool MoveBound = false, InteractBound = false, CancelBound = false;
    bool MenuBound = false, MapBound = false, QuestBound = false, QuickBound = false;
    auto* Enhanced = Cast<UEnhancedInputComponent>(InputComponent);
    if (Enhanced && InputConfig && InputConfig->MappingContext.LoadSynchronous())
    {
        if (UInputAction* Action = InputConfig->Move.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMove);
            MoveBound = true;
        }
        if (UInputAction* Action = InputConfig->Interact.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleInteract);
            InteractBound = true;
        }
        if (UInputAction* Action = InputConfig->Cancel.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleCancel);
            CancelBound = true;
        }
        if (UInputAction* Action = InputConfig->Menu.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMenu);
            MenuBound = true;
        }
        if (UInputAction* Action = InputConfig->Map.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMap);
            MapBound = true;
        }
        if (UInputAction* Action = InputConfig->Quest.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleQuest);
            QuestBound = true;
        }
        if (UInputAction* Action = InputConfig->QuickItem.LoadSynchronous())
        {
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleQuickItem);
            QuickBound = true;
        }
    }
    if (!MoveBound)
    {
        InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ARemasterPlayerController::StepUp);
        InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ARemasterPlayerController::StepUp);
        InputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &ARemasterPlayerController::StepUp);
        InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ARemasterPlayerController::StepDown);
        InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ARemasterPlayerController::StepDown);
        InputComponent->BindKey(EKeys::Gamepad_DPad_Down, IE_Pressed, this, &ARemasterPlayerController::StepDown);
        InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ARemasterPlayerController::StepLeft);
        InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ARemasterPlayerController::StepLeft);
        InputComponent->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &ARemasterPlayerController::StepLeft);
        InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ARemasterPlayerController::StepRight);
        InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ARemasterPlayerController::StepRight);
        InputComponent->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed, this, &ARemasterPlayerController::StepRight);
    }
    if (!InteractBound)
    {
        InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ARemasterPlayerController::InteractFallback);
        InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ARemasterPlayerController::InteractFallback);
        InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &ARemasterPlayerController::InteractFallback);
    }
    if (!CancelBound)
    {
        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARemasterPlayerController::CancelFallback);
        InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &ARemasterPlayerController::CancelFallback);
    }
    if (!MenuBound)
    {
        InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ARemasterPlayerController::MenuFallback);
        InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &ARemasterPlayerController::MenuFallback);
    }
    if (!MapBound)
    {
        InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ARemasterPlayerController::MapFallback);
    }
    if (!QuestBound)
    {
        InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ARemasterPlayerController::QuestFallback);
    }
    if (!QuickBound)
    {
        InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ARemasterPlayerController::QuickItemFallback);
        InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &ARemasterPlayerController::QuickItemFallback);
    }
}

void ARemasterPlayerController::StepDirection(int32 Direction)
{
    const ERemasterUiAction UiDirection = Direction == REMASTER_EMERALD_DIR_NORTH ? ERemasterUiAction::Up
        : Direction == REMASTER_EMERALD_DIR_SOUTH ? ERemasterUiAction::Down
        : Direction == REMASTER_EMERALD_DIR_WEST ? ERemasterUiAction::Left : ERemasterUiAction::Right;
    if (RouteUI(UiDirection)) return;
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    URemasterWorldGameplaySubsystem* Gameplay =
        GI->GetSubsystem<URemasterWorldGameplaySubsystem>();

    if (!Gameplay || !Gameplay->IsMapReady())
        return;

    FRemasterPlayerStepResult Result;
    if (!Gameplay->StepPlayer(Direction, Result))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Authoritative overworld step failed: direction=%d"),
            Direction);
        return;
    }

    if (ARemasterOverworldPawn* OverworldPawn =
            Cast<ARemasterOverworldPawn>(GetPawn()))
    {
        OverworldPawn->ApplyAuthoritativeStep(Result);
    }

    if (auto* UI = GI->GetSubsystem<URemasterUISubsystem>())
        UI->NotifyCoreStep(!Result.ScriptId.IsEmpty(), Result.bEncounterPending, Result.bRepelWoreOff);
    BP_OnWorldStep(Result);
}

void ARemasterPlayerController::HandleMove(
    const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();

    if (FMath::Abs(Axis.X) > FMath::Abs(Axis.Y))
    {
        if (Axis.X > 0.0f)
            StepDirection(REMASTER_EMERALD_DIR_EAST);
        else if (Axis.X < 0.0f)
            StepDirection(REMASTER_EMERALD_DIR_WEST);
    }
    else
    {
        if (Axis.Y > 0.0f)
            StepDirection(REMASTER_EMERALD_DIR_NORTH);
        else if (Axis.Y < 0.0f)
            StepDirection(REMASTER_EMERALD_DIR_SOUTH);
    }
}

void ARemasterPlayerController::HandleInteract(
    const FInputActionValue&)
{
    if (!RouteUI(ERemasterUiAction::Confirm)) BP_OnInteract();
}

void ARemasterPlayerController::HandleCancel(
    const FInputActionValue&)
{
    if (!RouteUI(ERemasterUiAction::Cancel)) BP_OnCancel();
}

void ARemasterPlayerController::HandleMenu(
    const FInputActionValue&)
{
    if (!RouteUI(ERemasterUiAction::Menu)) BP_OnMenu();
}

void ARemasterPlayerController::HandleMap(
    const FInputActionValue&)
{
    if (!RouteUI(ERemasterUiAction::Map)) BP_OnMap();
}

void ARemasterPlayerController::HandleQuest(
    const FInputActionValue&)
{
    if (!RouteUI(ERemasterUiAction::Quest)) BP_OnQuest();
}

void ARemasterPlayerController::HandleQuickItem(
    const FInputActionValue&)
{
    if (!RouteUI(ERemasterUiAction::QuickItem)) BP_OnQuickItem();
}

void ARemasterPlayerController::StepUp()
{
    StepDirection(REMASTER_EMERALD_DIR_NORTH);
}

void ARemasterPlayerController::StepDown()
{
    StepDirection(REMASTER_EMERALD_DIR_SOUTH);
}

void ARemasterPlayerController::StepLeft()
{
    StepDirection(REMASTER_EMERALD_DIR_WEST);
}

void ARemasterPlayerController::StepRight()
{
    StepDirection(REMASTER_EMERALD_DIR_EAST);
}

void ARemasterPlayerController::InteractFallback()
{
    if (!RouteUI(ERemasterUiAction::Confirm)) BP_OnInteract();
}

bool ARemasterPlayerController::RouteUI(ERemasterUiAction Action)
{
    if (auto* UI = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterUISubsystem>() : nullptr)
        return UI->SubmitAction(Action);
    return false;
}
void ARemasterPlayerController::CancelFallback() { if (!RouteUI(ERemasterUiAction::Cancel)) RouteUI(ERemasterUiAction::Menu); }
void ARemasterPlayerController::MenuFallback() { RouteUI(ERemasterUiAction::Menu); }
void ARemasterPlayerController::MapFallback() { RouteUI(ERemasterUiAction::Map); }
void ARemasterPlayerController::QuestFallback() { RouteUI(ERemasterUiAction::Quest); }
void ARemasterPlayerController::QuickItemFallback() { RouteUI(ERemasterUiAction::QuickItem); }
