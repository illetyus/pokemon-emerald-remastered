#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterObjectiveMarker.generated.h"

class USceneComponent;
class UTextRenderComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterObjectiveMarker : public AActor
{
    GENERATED_BODY()

public:
    ARemasterObjectiveMarker();

    UFUNCTION(BlueprintCallable)
    void SetMarkerVisible(bool bVisible);

    UFUNCTION(BlueprintCallable)
    void SetMarkerText(const FText& Text);

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Text;
};
