#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterUiModel.h"
#include "RemasterInputRouter.h"
#include "RemasterInputRepeat.h"
#include "RemasterUISubsystem.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class ERemasterUiAction : uint8
{
    Up, Down, Left, Right, Confirm, Cancel, Menu, Map, Quest, QuickItem
};

USTRUCT(BlueprintType)
struct FRemasterUiRow
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FText Label;
    UPROPERTY(BlueprintReadOnly) bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct FRemasterUiFrame
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FText Title;
    UPROPERTY(BlueprintReadOnly) FText Body;
    UPROPERTY(BlueprintReadOnly) TArray<FRemasterUiRow> Rows;
    UPROPERTY(BlueprintReadOnly) int32 ScreenId = 0;
    UPROPERTY(BlueprintReadOnly) int32 Selected = 0;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
    UPROPERTY(BlueprintReadOnly) bool bModal = false;
    UPROPERTY(BlueprintReadOnly) bool bHasMarker = false;
    UPROPERTY(BlueprintReadOnly) FVector2D MarkerPosition = FVector2D::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector2D MarkerSize = FVector2D::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRemasterUiFrameChanged, const FRemasterUiFrame&, Frame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRemasterUiHostEntry, int32, Value, int64, Revision);

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterUISubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    void StartNativePresentation();

    // Touch, keyboard and gamepad use the same route. True means consumed.
    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    bool SubmitAction(ERemasterUiAction Action);

    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    bool ActivateRow(int32 Index, int64 ExpectedRevision);

    UFUNCTION(BlueprintPure, Category="Remaster|UI")
    FRemasterUiFrame GetFrame() const { return PresentedFrame; }

    UFUNCTION(BlueprintPure, Category="Remaster|UI")
    bool BlocksWorldInput() const;
    RemasterControls::Context ReadInputContext();

    // The gameplay/script/battle host supplies busy lifetimes, not the widget.
    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    void SetHostBusy(bool bScript, bool bBattle);

    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    void SetBattleHostBusy(bool bBattle) { SetHostBusy(bScriptBusy, bBattle); }

    UFUNCTION(BlueprintCallable, Category="Remaster|UI")
    void NotifyCoreStep(bool bScriptPending, bool bEncounterPending, bool bRepelWoreOff);

    UPROPERTY(BlueprintAssignable) FRemasterUiFrameChanged OnFrameChanged;
    UPROPERTY(BlueprintAssignable) FRemasterUiHostEntry OnNicknameEntry;
    UPROPERTY(BlueprintAssignable) FRemasterUiHostEntry OnFieldItemEntry;

    // Runtime owner must detach before destroying/restarting its VM. Text/choice
    // resources must have already been resolved by that owner from core request IDs.
    bool PresentCoreDialogue(RemasterEmeraldScriptRuntime* Runtime,
        const FString& ResolvedText, const TArray<FText>& ChoiceLabels,
        const TArray<int32>& ChoiceValues);
    void DetachCoreDialogue();
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
    void AdvanceInputBoundary();
    uint64 InputHostGeneration=1,LastInputRequest=0;
    int32 LastInputScreen=-1;
    FString LastInputMap;
    void Refresh();
    void PresentationTick();
    RemasterUi::Context ReadContext() const;
    RemasterEmeraldSave* MutableSave() const;
    RemasterUi::Model Model;
    RemasterEmeraldScriptRuntime* DialogueRuntime = nullptr;
    UPROPERTY() FRemasterUiFrame PresentedFrame;
    FTimerHandle PresentationTimer;
    bool bScriptBusy = false, bBattleBusy = false, bIoBusy = false;
    bool bRepelPromptPending = false;
    double LastTickSeconds = 0.0, TickAccumulator = 0.0;
    double PollAccumulator = 0.0;
    RemasterControls::UiRepeat CommonRepeat;
    FString LastFrameKey;
    UPROPERTY()
    TObjectPtr<UUserWidget> CurrentScreen;
};
