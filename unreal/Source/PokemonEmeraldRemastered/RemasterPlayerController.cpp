#include "RemasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "RemasterInputConfig.h"
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
}

void ARemasterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent)
        return;

    if (InputConfig)
    {
        if (UEnhancedInputComponent* Enhanced =
                Cast<UEnhancedInputComponent>(InputComponent))
        {
            if (UInputAction* Action =
                    InputConfig->Move.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleMove);
            }

            if (UInputAction* Action =
                    InputConfig->Interact.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleInteract);
            }

            if (UInputAction* Action =
                    InputConfig->Cancel.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleCancel);
            }

            if (UInputAction* Action =
                    InputConfig->Menu.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleMenu);
            }

            if (UInputAction* Action =
                    InputConfig->Map.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleMap);
            }

            if (UInputAction* Action =
                    InputConfig->Quest.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleQuest);
            }

            if (UInputAction* Action =
                    InputConfig->QuickItem.LoadSynchronous())
            {
                Enhanced->BindAction(
                    Action,
                    ETriggerEvent::Started,
                    this,
                    &ARemasterPlayerController::HandleQuickItem);
            }

            return;
        }

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("RemasterPlayerController falling back to direct key bindings."));
    }

    InputComponent->BindKey(
        EKeys::Up,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepUp);
    InputComponent->BindKey(
        EKeys::W,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepUp);
    InputComponent->BindKey(
        EKeys::Gamepad_DPad_Up,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepUp);

    InputComponent->BindKey(
        EKeys::Down,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepDown);
    InputComponent->BindKey(
        EKeys::S,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepDown);
    InputComponent->BindKey(
        EKeys::Gamepad_DPad_Down,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepDown);

    InputComponent->BindKey(
        EKeys::Left,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepLeft);
    InputComponent->BindKey(
        EKeys::A,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepLeft);
    InputComponent->BindKey(
        EKeys::Gamepad_DPad_Left,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepLeft);

    InputComponent->BindKey(
        EKeys::Right,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepRight);
    InputComponent->BindKey(
        EKeys::D,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepRight);
    InputComponent->BindKey(
        EKeys::Gamepad_DPad_Right,
        IE_Pressed,
        this,
        &ARemasterPlayerController::StepRight);

    InputComponent->BindKey(
        EKeys::Enter,
        IE_Pressed,
        this,
        &ARemasterPlayerController::InteractFallback);
    InputComponent->BindKey(
        EKeys::SpaceBar,
        IE_Pressed,
        this,
        &ARemasterPlayerController::InteractFallback);
    InputComponent->BindKey(
        EKeys::Gamepad_FaceButton_Bottom,
        IE_Pressed,
        this,
        &ARemasterPlayerController::InteractFallback);
}

void ARemasterPlayerController::StepDirection(int32 Direction)
{
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
    BP_OnInteract();
}

void ARemasterPlayerController::HandleCancel(
    const FInputActionValue&)
{
    BP_OnCancel();
}

void ARemasterPlayerController::HandleMenu(
    const FInputActionValue&)
{
    BP_OnMenu();
}

void ARemasterPlayerController::HandleMap(
    const FInputActionValue&)
{
    BP_OnMap();
}

void ARemasterPlayerController::HandleQuest(
    const FInputActionValue&)
{
    BP_OnQuest();
}

void ARemasterPlayerController::HandleQuickItem(
    const FInputActionValue&)
{
    BP_OnQuickItem();
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
    BP_OnInteract();
}
