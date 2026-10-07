#include "RemasterCharacterVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/StreamableManager.h"
#include "RemasterCharacterAssetSet.h"
#include "RemasterCharacterCatalogSubsystem.h"

void URemasterCharacterVisualComponent::SetupVisuals(
    UStaticMeshComponent* Placeholder, USkeletalMeshComponent* Skeletal)
{
    PlaceholderMesh = Placeholder;
    SkeletalMesh = Skeletal;
    if (PlaceholderMesh)
        PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (SkeletalMesh)
    {
        SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkeletalMesh->SetGenerateOverlapEvents(false);
        SkeletalMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        SkeletalMesh->bEnableUpdateRateOptimizations = true;
        SkeletalMesh->SetVisibility(false);
    }
    ShowPlaceholder();
}

void URemasterCharacterVisualComponent::CancelPendingLoad()
{
    LoadGeneration.Cancel();
    if (PendingLoad.IsValid())
        PendingLoad->CancelHandle();
    PendingLoad.Reset();
}

void URemasterCharacterVisualComponent::ShowPlaceholder()
{
    if (SkeletalMesh)
    {
        SkeletalMesh->SetVisibility(false);
        SkeletalMesh->SetSkeletalMesh(nullptr);
    }
    if (PlaceholderMesh)
        PlaceholderMesh->SetVisibility(true);
}

void URemasterCharacterVisualComponent::SetGraphicsId(int32 GraphicsId)
{
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const URemasterCharacterCatalogSubsystem* Catalog = GI
        ? GI->GetSubsystem<URemasterCharacterCatalogSubsystem>() : nullptr;
    const uint32 Revision = Catalog ? Catalog->GetCatalogRevision() : 0;
    if (bIdentityApplied && CurrentGraphicsId == GraphicsId && CurrentCatalogRevision == Revision)
        return;
    bIdentityApplied = true;
    CurrentGraphicsId = GraphicsId;
    CurrentCatalogRevision = Revision;
    CancelPendingLoad();
    ShowPlaceholder();

    FRemasterCharacterPresentationEntry Entry;
    if (!Catalog || !Catalog->ResolveGraphicsId(GraphicsId, Entry))
        return;
    if (PlaceholderMesh)
    {
        PlaceholderMesh->ComponentTags.Reset();
        PlaceholderMesh->ComponentTags.Add(FName(*Entry.FallbackId));
        const TCHAR* Shape = Entry.PresentationKind == TEXT("pokemon_overworld")
            ? TEXT("/Engine/BasicShapes/Sphere.Sphere")
            : Entry.PresentationKind == TEXT("special_object")
                ? TEXT("/Engine/BasicShapes/Cube.Cube")
                : TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Shape))
            PlaceholderMesh->SetStaticMesh(Mesh);
    }
    if (Entry.NormalizedSha256.IsEmpty() || !SkeletalMesh)
        return;

    // Exact ID only. No guessed family, named-character or state substitution.
    const URemasterCharacterPresentationSettings* Settings =
        GetDefault<URemasterCharacterPresentationSettings>();
    LocalAssetSet = Settings->AssetSet.LoadSynchronous();
    const FRemasterCharacterAssetBinding* Binding = LocalAssetSet
        ? LocalAssetSet->Models.Find(Entry.ModelId) : nullptr;
    if (!Binding || !Binding->bUnrealImportValidated || Binding->Mesh.IsNull()
        || !Entry.NormalizedSha256.Contains(Binding->NormalizedSha256))
        return;

    const uint64 Generation = LoadGeneration.Value;
    const TSoftObjectPtr<USkeletalMesh> SoftMesh = Binding->Mesh;
    const TWeakObjectPtr<URemasterCharacterVisualComponent> WeakThis(this);
    PendingLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        SoftMesh.ToSoftObjectPath(), FStreamableDelegate::CreateLambda(
            [WeakThis, Generation, SoftMesh, Entry]()
            {
                URemasterCharacterVisualComponent* Self = WeakThis.Get();
                if (!Self || !Self->LoadGeneration.Accept(Generation) || !Self->SkeletalMesh)
                    return;
                USkeletalMesh* Mesh = SoftMesh.Get();
                if (!Mesh)
                    return; // Visible placeholder stays in place on load failure.
                Self->SkeletalMesh->SetSkeletalMesh(Mesh);
                Self->SkeletalMesh->SetRelativeScale3D(FVector(Entry.Scale));
                Self->SkeletalMesh->SetRelativeLocation(FVector(0, 0, Entry.GroundOffsetCm));
                Self->SkeletalMesh->SetRelativeRotation(FRotator(0, Entry.YawOffsetDeg, 0));
                // Reference pose until exact clips and authority semantics are validated.
                Self->SkeletalMesh->SetVisibility(true);
                if (Self->PlaceholderMesh)
                    Self->PlaceholderMesh->SetVisibility(false);
            }));
}

void URemasterCharacterVisualComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelPendingLoad();
    Super::EndPlay(Reason);
}
