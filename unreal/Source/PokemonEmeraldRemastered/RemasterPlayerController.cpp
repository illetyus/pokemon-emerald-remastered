#include "RemasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "RemasterCoreSubsystem.h"
#include "RemasterInputConfig.h"

void ARemasterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!InputConfig)
        return;

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer)
        return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

    if (!Subsystem)
        return;

    if (UInputMappingContext* Context =
            InputConfig->MappingContext.LoadSynchronous())
    {
        Subsystem->AddMappingContext(Context, 0);
    }
}

void ARemasterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputConfig)
        return;

    UEnhancedInputComponent* Enhanced =
        Cast<UEnhancedInputComponent>(InputComponent);

    if (!Enhanced)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("RemasterPlayerController requires EnhancedInputComponent."));
        return;
    }

    if (UInputAction* Action = InputConfig->Move.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMove);

    if (UInputAction* Action = InputConfig->Interact.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleInteract);

    if (UInputAction* Action = InputConfig->Cancel.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleCancel);

    if (UInputAction* Action = InputConfig->Menu.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMenu);

    if (UInputAction* Action = InputConfig->Map.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMap);

    if (UInputAction* Action = InputConfig->Quest.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleQuest);

    if (UInputAction* Action = InputConfig->QuickItem.LoadSynchronous())
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleQuickItem);
}

void ARemasterPlayerController::Step(ERemasterAction Action)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterCoreSubsystem* Core =
                GI->GetSubsystem<URemasterCoreSubsystem>())
        {
            Core->Step(Action);
        }
    }
}

void ARemasterPlayerController::HandleMove(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();

    if (FMath::Abs(Axis.X) > FMath::Abs(Axis.Y))
    {
        if (Axis.X > 0.0f)
            Step(ERemasterAction::MoveRight);
        else if (Axis.X < 0.0f)
            Step(ERemasterAction::MoveLeft);
    }
    else
    {
        if (Axis.Y > 0.0f)
            Step(ERemasterAction::MoveUp);
        else if (Axis.Y < 0.0f)
            Step(ERemasterAction::MoveDown);
    }
}

void ARemasterPlayerController::HandleInteract(const FInputActionValue&)
{
    Step(ERemasterAction::Interact);
}

void ARemasterPlayerController::HandleCancel(const FInputActionValue&)
{
    BP_OnCancel();
}

void ARemasterPlayerController::HandleMenu(const FInputActionValue&)
{
    BP_OnMenu();
}

void ARemasterPlayerController::HandleMap(const FInputActionValue&)
{
    BP_OnMap();
}

void ARemasterPlayerController::HandleQuest(const FInputActionValue&)
{
    BP_OnQuest();
}

void ARemasterPlayerController::HandleQuickItem(const FInputActionValue&)
{
    BP_OnQuickItem();
}
