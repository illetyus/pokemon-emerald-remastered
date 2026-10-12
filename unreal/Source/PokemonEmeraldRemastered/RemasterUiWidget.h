#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "RemasterUISubsystem.h"
#include "RemasterUiWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UButton;
class USizeBox;
class UBorder;
class UCanvasPanelSlot;
class UScrollBox;

// UButton's focus initializer is protected and must run before Slate construction.
UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterUiButton : public UButton
{
    GENERATED_BODY()
public:
    URemasterUiButton(const FObjectInitializer& ObjectInitializer);
};

// Each button retains its own frame revision; a late pointer click is rejected.
UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterUiRowWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(URemasterUISubsystem* InUI, int32 InIndex, int64 InRevision,
        const FText& Label, bool bEnabled, bool bSelected);
private:
    UFUNCTION() void Click();
    UPROPERTY() TObjectPtr<URemasterUISubsystem> UI;
    int32 Index = 0;
    int64 Revision = 0;
};

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterUiWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UFUNCTION() void Present(const FRemasterUiFrame& Frame);
    UFUNCTION() void Back();
    UPROPERTY() TObjectPtr<URemasterUISubsystem> UI;
    UPROPERTY() TObjectPtr<UTextBlock> Title;
    UPROPERTY() TObjectPtr<UTextBlock> Body;
    UPROPERTY() TObjectPtr<UVerticalBox> Rows;
    UPROPERTY() TObjectPtr<UButton> BackButton;
    UPROPERTY() TObjectPtr<USizeBox> MapPanel;
    UPROPERTY() TObjectPtr<UBorder> Marker;
    UPROPERTY() TObjectPtr<UCanvasPanelSlot> MarkerSlot;
    UPROPERTY() TObjectPtr<UScrollBox> Scroll;
    int64 RowsRevision = -1;
};
