#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "RemasterWorldGameplaySubsystem.h"
#include "RemasterPlayerController.generated.h"

class URemasterInputConfig;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterPlayerController
    : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

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
    void HandleInteract(const FInputActionValue& Value);
    void HandleCancel(const FInputActionValue& Value);
    void HandleMenu(const FInputActionValue& Value);
    void HandleMap(const FInputActionValue& Value);
    void HandleQuest(const FInputActionValue& Value);
    void HandleQuickItem(const FInputActionValue& Value);

    void StepDirection(int32 Direction);
    void StepUp();
    void StepDown();
    void StepLeft();
    void StepRight();
    void InteractFallback();
};
