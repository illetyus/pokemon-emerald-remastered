#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "R0GameMode.generated.h"

UCLASS()
class POKEMONEMERALDREMASTERED_API AR0GameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AR0GameMode();
    virtual void StartPlay() override;
};
