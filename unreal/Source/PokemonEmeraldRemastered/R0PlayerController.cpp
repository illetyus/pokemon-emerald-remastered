#include "R0PlayerController.h"

#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "RemasterCoreSubsystem.h"

void AR0PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    check(InputComponent);

    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AR0PlayerController::StepUp);
    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AR0PlayerController::StepUp);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &AR0PlayerController::StepUp);

    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AR0PlayerController::StepDown);
    InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AR0PlayerController::StepDown);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Down, IE_Pressed, this, &AR0PlayerController::StepDown);

    InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AR0PlayerController::StepLeft);
    InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AR0PlayerController::StepLeft);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &AR0PlayerController::StepLeft);

    InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AR0PlayerController::StepRight);
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AR0PlayerController::StepRight);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed, this, &AR0PlayerController::StepRight);

    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AR0PlayerController::Interact);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AR0PlayerController::Interact);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &AR0PlayerController::Interact);

    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AR0PlayerController::ResetCore);
    InputComponent->BindTouch(IE_Pressed, this, &AR0PlayerController::HandleTouchPressed);

    bEnableTouchEvents = true;
}

void AR0PlayerController::Step(ERemasterAction Action)
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URemasterCoreSubsystem* Core =
                GameInstance->GetSubsystem<URemasterCoreSubsystem>())
        {
            Core->Step(Action);
        }
    }
}

void AR0PlayerController::StepUp()
{
    Step(ERemasterAction::MoveUp);
}

void AR0PlayerController::StepDown()
{
    Step(ERemasterAction::MoveDown);
}

void AR0PlayerController::StepLeft()
{
    Step(ERemasterAction::MoveLeft);
}

void AR0PlayerController::StepRight()
{
    Step(ERemasterAction::MoveRight);
}

void AR0PlayerController::Interact()
{
    Step(ERemasterAction::Interact);
}

void AR0PlayerController::ResetCore()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URemasterCoreSubsystem* Core =
                GameInstance->GetSubsystem<URemasterCoreSubsystem>())
        {
            Core->ResetCore();
        }
    }
}

void AR0PlayerController::HandleTouchPressed(
    ETouchIndex::Type FingerIndex,
    FVector Location)
{
    int32 Width = 0;
    int32 Height = 0;
    GetViewportSize(Width, Height);

    if (Width <= 0 || Height <= 0)
    {
        return;
    }

    const float X = Location.X / static_cast<float>(Width);
    const float Y = Location.Y / static_cast<float>(Height);

    if (X > 0.70f)
    {
        Interact();
        return;
    }

    if (X < 0.35f)
    {
        if (Y < 0.35f)
        {
            StepUp();
        }
        else if (Y > 0.65f)
        {
            StepDown();
        }
        else
        {
            StepLeft();
        }
        return;
    }

    if (X < 0.70f)
    {
        StepRight();
    }
}
