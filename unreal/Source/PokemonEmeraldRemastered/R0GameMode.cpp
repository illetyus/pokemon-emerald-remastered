#include "R0GameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "R0HUD.h"
#include "RemasterCameraRig.h"
#include "RemasterOverworldPawn.h"
#include "RemasterPlayerController.h"
#include "RemasterWorldActor.h"

AR0GameMode::AR0GameMode()
{
    HUDClass = AR0HUD::StaticClass();
    PlayerControllerClass = ARemasterPlayerController::StaticClass();
    DefaultPawnClass = ARemasterOverworldPawn::StaticClass();
}

void AR0GameMode::StartPlay()
{
    Super::StartPlay();

    UWorld* World = GetWorld();
    if (!World)
        return;

    ARemasterWorldActor* WorldRenderer = nullptr;
    for (TActorIterator<ARemasterWorldActor> It(World); It; ++It)
    {
        WorldRenderer = *It;
        break;
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

    ARemasterCameraRig* CameraRig = nullptr;
    for (TActorIterator<ARemasterCameraRig> It(World); It; ++It)
    {
        CameraRig = *It;
        break;
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
