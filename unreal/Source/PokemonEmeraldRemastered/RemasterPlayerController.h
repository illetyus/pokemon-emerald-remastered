#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterPhysicalInput.h"
#include "RemasterPlayerController.generated.h"

class URemasterInputConfig;
class UInputMappingContext;
enum class ERemasterUiAction : uint8;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterPlayerController
    : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Remaster|Input")
    TObjectPtr<URemasterInputConfig> InputConfig;

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnWorldStep(const FRemasterPlayerStepResult& Result);

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnInteract();

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnCancel();

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnMenu();

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnMap();

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnQuest();

    UFUNCTION(BlueprintImplementableEvent, Category="Remaster|Input")
    void BP_OnQuickItem();

private:
    void HandleMove(const FInputActionValue& Value);
    void HandleMoveReleased(const FInputActionValue& Value);
    void HandleButton(const FInputActionValue& Value,RemasterControls::Action Action,
        RemasterControls::Phase Phase);
    void BindNativeAction(RemasterControls::Action Action);
    void PhysicalInput(RemasterControls::Source Source,uint16 Control,
        RemasterControls::Action Action,RemasterControls::Phase Phase);
    bool bNativeGamepadAxis = false;
    bool bInputRoutingAttached = false;
    TWeakObjectPtr<UInputMappingContext> OwnedMappingContext;
    RemasterControls::Context ReadInputContext();
    bool DispatchInput(RemasterControls::Action Action,RemasterControls::Target Target);
    bool StepDirection(int32 Direction);
    bool RouteUI(ERemasterUiAction Action);
    void TouchPressed(ETouchIndex::Type Finger, FVector Location);
    void TouchMoved(ETouchIndex::Type Finger, FVector Location);
    void TouchReleased(ETouchIndex::Type Finger, FVector Location);
    FVector2D NormalizeTouch(FVector Location) const;
};

