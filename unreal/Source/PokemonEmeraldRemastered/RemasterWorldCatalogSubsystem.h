#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterWorldCatalog.h"
#include "RemasterWorldData.h"
#include "RemasterWorldCatalogSubsystem.generated.h"

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterWorldCatalogSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(
        FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    bool ReloadCatalog();

    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    bool ResolveMapJson(
        int32 GroupNum,
        int32 MapNum,
        FString& OutAbsolutePath) const;

    bool LoadMap(
        int32 GroupNum,
        int32 MapNum,
        FRemasterMapIR& OutMap,
        FString& OutError) const;

    UFUNCTION(BlueprintPure, Category="Remaster|World")
    FString GetManifestPath() const;

private:
    FRemasterWorldCatalog Catalog;
    bool bCatalogReady = false;
};
