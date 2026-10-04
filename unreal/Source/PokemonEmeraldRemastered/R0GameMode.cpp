#include "R0GameMode.h"

#include "R0HUD.h"
#include "RemasterOverworldPawn.h"
#include "RemasterPlayerController.h"

AR0GameMode::AR0GameMode()
{
    HUDClass = AR0HUD::StaticClass();
    PlayerControllerClass = ARemasterPlayerController::StaticClass();
    DefaultPawnClass = ARemasterOverworldPawn::StaticClass();
}
