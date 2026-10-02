#include "RemasterUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"

UUserWidget* URemasterUISubsystem::ShowScreen(
    TSubclassOf<UUserWidget> ScreenClass,
    int32 ZOrder)
{
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
    if (CurrentScreen)
    {
        CurrentScreen->RemoveFromParent();
        CurrentScreen = nullptr;
    }
}
