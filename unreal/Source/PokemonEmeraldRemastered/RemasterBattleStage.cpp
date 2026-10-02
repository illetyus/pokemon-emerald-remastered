#include "RemasterBattleStage.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"

ARemasterBattleStage::ARemasterBattleStage()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    PlayerAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerAnchor"));
    PlayerAnchor->SetupAttachment(SceneRoot);
    PlayerAnchor->SetRelativeLocation(FVector(-180.0f, 80.0f, 0.0f));

    OpponentAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("OpponentAnchor"));
    OpponentAnchor->SetupAttachment(SceneRoot);
    OpponentAnchor->SetRelativeLocation(FVector(180.0f, -80.0f, 0.0f));

    CameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CameraAnchor"));
    CameraAnchor->SetupAttachment(SceneRoot);
    CameraAnchor->SetRelativeLocation(FVector(-450.0f, 0.0f, 280.0f));
}

void ARemasterBattleStage::BeginPlay()
{
    Super::BeginPlay();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterBattlePresentationSubsystem* Presentation =
                GI->GetSubsystem<URemasterBattlePresentationSubsystem>())
        {
            Presentation->OnBattlePresentationEvent.AddDynamic(
                this,
                &ARemasterBattleStage::HandlePresentationEvent);
        }
    }
}

void ARemasterBattleStage::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterBattlePresentationSubsystem* Presentation =
                GI->GetSubsystem<URemasterBattlePresentationSubsystem>())
        {
            Presentation->OnBattlePresentationEvent.RemoveDynamic(
                this,
                &ARemasterBattleStage::HandlePresentationEvent);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void ARemasterBattleStage::HandlePresentationEvent(
    const FRemasterBattlePresentationEvent& Event)
{
    BP_HandlePresentationEvent(Event);
}
