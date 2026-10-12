#include "RemasterCharacterImportPilot.h"

#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "RemasterCharacterVisualComponent.h"
#include "RemasterCoreSubsystem.h"

ARemasterCharacterImportPilotActor::ARemasterCharacterImportPilotActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
    Placeholder->SetupAttachment(RootComponent);
    Placeholder->SetRelativeScale3D(FVector(0.25, 0.25, 0.8));
    Placeholder->SetRelativeLocation(FVector(0, 0, 40));
    Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ImportedCharacter"));
    Skeletal->SetupAttachment(RootComponent);
    Skeletal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Skeletal->SetGenerateOverlapEvents(false);
    Visual = CreateDefaultSubobject<URemasterCharacterVisualComponent>(TEXT("ProductionVisualBinding"));
}

void ARemasterCharacterImportPilotActor::BeginPlay()
{
    Super::BeginPlay();
#if UE_BUILD_SHIPPING
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
#else
    InitialLocation = GetActorLocation();
    if (const URemasterCoreSubsystem* Core = GetGameInstance()->GetSubsystem<URemasterCoreSubsystem>())
        InitialCoreHash = Core->StateHash();
    Visual->SetupVisuals(Placeholder, Skeletal);
    Visual->SetGraphicsId(GraphicsId);
    Skeletal->bEnableUpdateRateOptimizations = false;
    Skeletal->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Skeletal->SetComponentTickEnabled(true);
#endif
}

void ARemasterCharacterImportPilotActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (bFailed) return;
    Elapsed += DeltaSeconds;
    const URemasterCoreSubsystem* Core = GetGameInstance()->GetSubsystem<URemasterCoreSubsystem>();
    if (!Core || Core->StateHash() != InitialCoreHash || !GetActorLocation().Equals(InitialLocation, 0.001))
    {
        UE_LOG(LogTemp, Error, TEXT("AP02_AUTHORITY_BOUNDARY_FAILED actor=%s"), *GetName());
        bFailed = true;
        return;
    }
    if (bExpectFallback)
    {
        if (Elapsed >= 3 && !bVerifiedBoundary)
        {
            const bool bFallback = Placeholder->IsVisible() && !Skeletal->IsVisible()
                && Skeletal->GetSkeletalMeshAsset() == nullptr && Placeholder->GetStaticMesh() != nullptr;
            UE_LOG(LogTemp, Display, TEXT("AP02_FALLBACK_CHECK actor=%s graphics_id=%d pass=%d core_hash=%llu"),
                *GetName(), GraphicsId, bFallback, static_cast<unsigned long long>(InitialCoreHash));
            bFailed = !bFallback;
            bVerifiedBoundary = bFallback;
        }
        return;
    }
    if (!bStartedClip && Skeletal->IsVisible() && Skeletal->GetSkeletalMeshAsset())
    {
        LoadedClip = PilotClip.LoadSynchronous();
        if (!LoadedClip || LoadedClip->GetSkeleton() != Skeletal->GetSkeletalMeshAsset()->GetSkeleton()
            || LoadedClip->bEnableRootMotion || Placeholder->IsVisible())
        {
            UE_LOG(LogTemp, Error, TEXT("AP02_IMPORT_CLIP_BINDING_FAILED actor=%s"), *GetName());
            bFailed = true;
            return;
        }
        Skeletal->PlayAnimation(LoadedClip, true);
        if (!bExpectMotion) Skeletal->SetPlayRate(0.0f);
        bStartedClip = true;
        UE_LOG(LogTemp, Display, TEXT("AP02_IMPORT_BINDING_OK actor=%s mesh=%s clip=%s duration=%.3f"),
            *GetName(), *Skeletal->GetSkeletalMeshAsset()->GetPathName(), *LoadedClip->GetPathName(),
            LoadedClip->GetPlayLength());
    }
    if (bStartedClip) ClipElapsed += DeltaSeconds;
    if (bStartedClip && !bObservedMotion && ClipElapsed >= 1.0f
        && ClipElapsed - LastSampleTime >= 0.05f)
    {
        const FName Bone = LoadedClip->GetName().Contains(TEXT("Idle")) ? FName(TEXT("Head")) : FName(TEXT("RFoot"));
        const FTransform Pose = Skeletal->GetSocketTransform(Bone, RTS_Component);
        if (Skeletal->GetBoneIndex(Bone) == INDEX_NONE)
        {
            bFailed = true;
            UE_LOG(LogTemp, Error, TEXT("AP02_PILOT_BONE_MISSING actor=%s bone=%s"), *GetName(), *Bone.ToString());
            return;
        }
        LastSampleTime = ClipElapsed;
        if (!bSampledPose)
        {
            FirstBonePose = Pose;
            bSampledPose = true;
        }
        else if (!Pose.Equals(FirstBonePose, 0.001))
        {
            FirstBonePose = Pose;
            ++PoseChanges;
            if (PoseChanges >= 3)
            {
                bObservedMotion = true;
                UE_LOG(LogTemp, Display, TEXT("AP02_CLIP_MOTION_OBSERVED actor=%s bone=%s clip=%s samples=%d"),
                    *GetName(), *Bone.ToString(), *LoadedClip->GetName(), PoseChanges);
            }
        }
    }
    if (!bExpectMotion && ClipElapsed >= 4 && !bVerifiedBoundary)
    {
        const bool bStaticRejected = bSampledPose && PoseChanges == 0 && !bObservedMotion;
        UE_LOG(LogTemp, Display, TEXT("AP02_STATIC_NEGATIVE_CHECK actor=%s pass=%d"), *GetName(), bStaticRejected);
        bVerifiedBoundary = bStaticRejected;
        bFailed = !bStaticRejected;
    }
    if (bExpectMotion && bObservedMotion && Elapsed >= 5 && !bVerifiedBoundary)
    {
        bVerifiedBoundary = true;
        UE_LOG(LogTemp, Display, TEXT("AP02_CORE_UNCHANGED actor=%s hash=%llu"), *GetName(),
            static_cast<unsigned long long>(InitialCoreHash));
    }
    if (Elapsed > 15 && !bVerifiedBoundary)
    {
        bFailed = true;
        UE_LOG(LogTemp, Error, TEXT("AP02_PILOT_TIMEOUT actor=%s"), *GetName());
    }
#endif
}

ARemasterCharacterImportPilotGameMode::ARemasterCharacterImportPilotGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = APlayerController::StaticClass();
}
