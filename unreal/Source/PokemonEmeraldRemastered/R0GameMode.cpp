#include "R0GameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "RemasterCameraRig.h"
#include "RemasterOverworldPawn.h"
#include "RemasterPlayerController.h"
#include "RemasterWorldActor.h"
#include "RemasterNpcPresentationWorld.h"
#include "RemasterEnvironmentController.h"
#include "RemasterBattleStage.h"

AR0GameMode::AR0GameMode()
{
    HUDClass = nullptr;
    PlayerControllerClass = ARemasterPlayerController::StaticClass();
    DefaultPawnClass = ARemasterOverworldPawn::StaticClass();
}

void AR0GameMode::StartPlay()
{
    Super::StartPlay();

    UWorld* World = GetWorld();
    if (!World)
        return;

    bool bHasBattleStage = false;
    if (TActorIterator<ARemasterBattleStage> It(World); It) { bHasBattleStage = true; }
    if (!bHasBattleStage)
        World->SpawnActor<ARemasterBattleStage>(ARemasterBattleStage::StaticClass(), FVector(0,0,10000), FRotator::ZeroRotator);

    ARemasterWorldActor* WorldRenderer = nullptr;
    if (TActorIterator<ARemasterWorldActor> It(World); It)
    {
        WorldRenderer = *It;
    }

    if (!WorldRenderer)
    {
        WorldRenderer = World->SpawnActor<ARemasterWorldActor>(
            ARemasterWorldActor::StaticClass(),
            FTransform::Identity);
    }

    APlayerController* PC = World->GetFirstPlayerController();
    ARemasterOverworldPawn* OverworldPawn =
        PC ? Cast<ARemasterOverworldPawn>(PC->GetPawn()) : nullptr;

    if (!OverworldPawn && PC)
    {
        OverworldPawn = World->SpawnActor<ARemasterOverworldPawn>(
            ARemasterOverworldPawn::StaticClass(),
            FTransform::Identity);

        if (OverworldPawn)
        {
            PC->Possess(OverworldPawn);
        }
    }

    if (WorldRenderer
        && !WorldRenderer->GetLoadedMap().IsValid())
    {
        WorldRenderer->LoadAuthoritativeMap();
    }

    if (OverworldPawn)
    {
        OverworldPawn->SyncFromAuthoritativeState();
    }

    if (!TActorIterator<ARemasterNpcPresentationWorld>(World))
    {
        World->SpawnActor<ARemasterNpcPresentationWorld>(
            ARemasterNpcPresentationWorld::StaticClass(), FTransform::Identity);
    }

    if (!TActorIterator<ARemasterEnvironmentController>(World))
    {
        World->SpawnActor<ARemasterEnvironmentController>(
            ARemasterEnvironmentController::StaticClass(), FTransform::Identity);
    }

    ARemasterCameraRig* CameraRig = nullptr;
    if (TActorIterator<ARemasterCameraRig> It(World); It)
    {
        CameraRig = *It;
    }

    if (!CameraRig)
    {
        CameraRig = World->SpawnActor<ARemasterCameraRig>(
            ARemasterCameraRig::StaticClass(),
            FTransform::Identity);
    }

    if (CameraRig && OverworldPawn)
    {
        CameraRig->SetFollowTarget(OverworldPawn);

        if (PC)
        {
            PC->SetViewTarget(CameraRig);
        }
    }
}
