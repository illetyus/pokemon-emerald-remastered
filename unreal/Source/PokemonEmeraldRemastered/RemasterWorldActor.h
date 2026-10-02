#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterWorldData.h"
#include "RemasterWorldActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class URemasterVisualStyle;
class USceneComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterWorldActor : public AActor
{
    GENERATED_BODY()

public:
    ARemasterWorldActor();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    bool LoadMapFromGeneratedData(const FString& RelativeJsonPath);

    UFUNCTION(BlueprintCallable, Category="Remaster|World")
    void ClearWorld();

    const FRemasterMapIR& GetLoadedMap() const
    {
        return LoadedMap;
    }

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BlockInstances;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    TObjectPtr<URemasterVisualStyle> VisualStyle;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    float TileWorldSize = 100.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    float PreviewThickness = 10.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|World")
    FString StartupMapJson;

private:
    void BuildPreviewInstances();
    UHierarchicalInstancedStaticMeshComponent* ComponentForBlock(uint16 RawBlock);

    FRemasterMapIR LoadedMap;
    TMap<int32, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> VisualComponents;
};
