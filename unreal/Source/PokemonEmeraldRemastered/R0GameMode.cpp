#include "R0GameMode.h"

#include "R0HUD.h"
#include "R0PlayerController.h"

AR0GameMode::AR0GameMode()
{
    HUDClass = AR0HUD::StaticClass();
    PlayerControllerClass = AR0PlayerController::StaticClass();
    DefaultPawnClass = nullptr;
}
