#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RemasterCoreAdapter.h"
#include "R0PlayerController.generated.h"

UCLASS()
class POKEMONEMERALDREMASTERED_API AR0PlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void SetupInputComponent() override;

private:
    void StepUp();
    void StepDown();
    void StepLeft();
    void StepRight();
    void Interact();
    void ResetCore();

    void HandleTouchPressed(
        ETouchIndex::Type FingerIndex,
        FVector Location);

    void Step(ERemasterAction Action);
};
