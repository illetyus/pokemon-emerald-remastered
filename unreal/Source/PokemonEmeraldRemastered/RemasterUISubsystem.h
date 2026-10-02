#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterUISubsystem.generated.h"

class UUserWidget;

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterUISubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    UUserWidget* ShowScreen(TSubclassOf<UUserWidget> ScreenClass, int32 ZOrder = 0);

    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    void HideCurrentScreen();

    UFUNCTION(BlueprintPure, Category="Remaster|UI")
    UUserWidget* GetCurrentScreen() const
    {
        return CurrentScreen;
    }

private:
    UPROPERTY()
    TObjectPtr<UUserWidget> CurrentScreen;
};
