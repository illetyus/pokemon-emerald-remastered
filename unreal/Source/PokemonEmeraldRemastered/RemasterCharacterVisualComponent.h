#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RemasterCharacterPresentationRead.h"
#include "RemasterCharacterVisualComponent.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class URemasterCharacterAssetSet;
class FStreamableHandle;

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterCharacterVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void SetupVisuals(UStaticMeshComponent* Placeholder, USkeletalMeshComponent* Skeletal);
    void SetGraphicsId(int32 GraphicsId);
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void CancelPendingLoad();
    void ShowPlaceholder();

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
    UPROPERTY()
    TObjectPtr<URemasterCharacterAssetSet> LocalAssetSet;
    TSharedPtr<FStreamableHandle> PendingLoad;
    RemasterCharacterPresentation::LoadEpoch LoadGeneration;
    int32 CurrentGraphicsId = -1;
    uint32 CurrentCatalogRevision = 0;
    bool bIdentityApplied = false;
};
