#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterCoreAdapter.h"
#include "RemasterCoreSubsystem.generated.h"

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterCoreSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void ResetCore();
    uint32 Step(ERemasterAction Action);
    FRemasterSnapshot Snapshot() const;
    uint64 StateHash() const;

    bool SavePersistentState();
    bool LoadPersistentState();

private:
    void HandleWillEnterBackground();
    void HandleEnteredForeground();

    FString SavePath() const;

    FRemasterCoreAdapter Core;
};
