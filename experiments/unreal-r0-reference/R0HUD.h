#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "R0HUD.generated.h"

UCLASS()
class POKEMONEMERALDREMASTERED_API AR0HUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawFilledRect(
        const FVector2D& Position,
        const FVector2D& Size,
        const FLinearColor& Color);
};
