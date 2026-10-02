#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterFeedbackSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FRemasterFeedbackDelegate,
    FName, EventName,
    int64, Arg0,
    int64, Arg1);

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterFeedbackSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FRemasterFeedbackDelegate OnFeedback;

    UFUNCTION(BlueprintCallable)
    void Emit(FName EventName, int64 Arg0 = 0, int64 Arg1 = 0);
};
