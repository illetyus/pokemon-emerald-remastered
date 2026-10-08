#include "RemasterBattleStage.h"
#include "RemasterPokemonAssetSet.h"
#include "RemasterUiModel.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace {
void RenderOnly(UPrimitiveComponent* Component)
{
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false); Component->SetCanEverAffectNavigation(false);
    Component->SetCastShadow(false);
}
bool Hash(const FString& Value)
{
    if (Value.Len() != 64) return false;
    for (TCHAR C : Value) if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f'))) return false;
    return true;
}
FName Semantic(int32 Motion)
{
    const FName Names[] = {TEXT("idle"), TEXT("entry"), TEXT("attack"), TEXT("hit"), TEXT("faint")};
    return Motion >= 0 && Motion < 5 ? Names[Motion] : Names[0];
}
FText EventText(const FRemasterBattlePresentationEvent& Event)
{
    if (Event.RawKind == REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED)
        return FText::FromString(UTF8_TO_TCHAR(RemasterUi::Name(RemasterUi::MoveName(Event.MoveId), Event.MoveId).c_str()));
    if (Event.RawKind == REMASTER_EMERALD_BATTLE_EVENT_DAMAGE)
        return FText::FromString(FString::Printf(TEXT("Hasar: %d"), Event.Value));
    const TCHAR* Names[] = {TEXT(""),TEXT("Savaş başladı"),TEXT("Yeni tur"),TEXT("Hareket"),TEXT("Iskaladı"),TEXT("Hasar"),
        TEXT("Kritik vuruş"),TEXT("Etkililik"),TEXT("Durum etkisi"),TEXT("İstatistik değişimi"),TEXT("Hava durumu"),TEXT("Özel yetenek"),
        TEXT("Eşya"),TEXT("Pokémon değişimi"),TEXT("Pokémon bayıldı"),TEXT("Deneyim"),TEXT("Seviye yükseldi"),TEXT("Yakalama"),
        TEXT("Pokémon yakalandı"),TEXT("Kaçış"),TEXT("Savaş sona erdi"),TEXT("İyileşme"),TEXT("PP"),TEXT("Savaş mesajı"),TEXT("Hareket öğrenme"),TEXT("Evrim kontrolü")};
    return FText::FromString(Event.RawKind >= 0 && Event.RawKind < static_cast<int32>(UE_ARRAY_COUNT(Names)) ? Names[Event.RawKind] : TEXT("Savaş olayı"));
}
}

ARemasterBattleStage::ARemasterBattleStage()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")); RootComponent = SceneRoot;
    PlayerAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerAnchor")); PlayerAnchor->SetupAttachment(SceneRoot);
    PlayerAnchor->SetRelativeLocation(FVector(-180, 0, 0));
    OpponentAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("OpponentAnchor")); OpponentAnchor->SetupAttachment(SceneRoot);
    OpponentAnchor->SetRelativeLocation(FVector(180, 0, 0));
    CameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CameraAnchor")); CameraAnchor->SetupAttachment(SceneRoot);
    BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera")); BattleCamera->SetupAttachment(CameraAnchor);
    BattleCamera->SetFieldOfView(45);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor")); Floor->SetupAttachment(SceneRoot);
    if (Cube.Succeeded()) Floor->SetStaticMesh(Cube.Object);
    Floor->SetRelativeScale3D(FVector(9, 6, 0.05f)); Floor->SetRelativeLocation(FVector(0, 0, -3)); RenderOnly(Floor);
    for (int32 i = 0; i < 6; ++i)
    {
        auto* Root = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Slot%d"), i)); Root->SetupAttachment(SceneRoot); SlotRoots.Add(Root);
        auto* Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Fallback%d"), i)); Placeholder->SetupAttachment(Root);
        if (Sphere.Succeeded()) Placeholder->SetStaticMesh(Sphere.Object);
        RenderOnly(Placeholder); Placeholders.Add(Placeholder);
        auto* Static = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Static%d"), i)); Static->SetupAttachment(Root); Static->SetVisibility(false);
        RenderOnly(Static); StaticVisuals.Add(Static);
        auto* Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(*FString::Printf(TEXT("Skeletal%d"), i)); Skeletal->SetupAttachment(Root); Skeletal->SetVisibility(false);
        RenderOnly(Skeletal); SkeletalVisuals.Add(Skeletal);
        auto* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Label%d"), i)); Label->SetupAttachment(Root);
        Label->SetWorldSize(18); Label->SetHorizontalAlignment(EHTA_Center); RenderOnly(Label); Labels.Add(Label);
    }
    EffectPulse = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EffectPulse")); EffectPulse->SetupAttachment(SceneRoot);
    if (Sphere.Succeeded()) EffectPulse->SetStaticMesh(Sphere.Object);
    RenderOnly(EffectPulse); EffectPulse->SetVisibility(false);
    EventLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EventLabel")); EventLabel->SetupAttachment(SceneRoot);
    EventLabel->SetRelativeLocation(FVector(0, 0, 350)); EventLabel->SetWorldSize(22); EventLabel->SetHorizontalAlignment(EHTA_Center);
    RenderOnly(EventLabel); SetActorHiddenInGame(true);
}

void ARemasterBattleStage::BeginPlay()
{
    Super::BeginPlay();
    LocalAssets = GetDefault<URemasterBattlePresentationSettings>()->AssetSet.LoadSynchronous();
    if (auto* Presentation = GetGameInstance()->GetSubsystem<URemasterBattlePresentationSubsystem>())
    {
        Presentation->OnBattlePresentationEvent.AddDynamic(this, &ARemasterBattleStage::HandlePresentationEvent);
        Presentation->OnCoreSnapshot.AddDynamic(this, &ARemasterBattleStage::HandleCoreSnapshot);
        Presentation->OnPresentationReset.AddDynamic(this, &ARemasterBattleStage::ResetPresentation);
        const auto Latest = Presentation->GetLatestSnapshot();
        if (!Latest.Battlers.IsEmpty()) HandleCoreSnapshot(Latest);
        FRemasterBattlePresentationEvent Cue;
        if (Presentation->GetActiveCue(Cue)) HandlePresentationEvent(Cue);
    }
}
void ARemasterBattleStage::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Presentation = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterBattlePresentationSubsystem>() : nullptr)
    {
        Presentation->OnBattlePresentationEvent.RemoveDynamic(this, &ARemasterBattleStage::HandlePresentationEvent);
        Presentation->OnCoreSnapshot.RemoveDynamic(this, &ARemasterBattleStage::HandleCoreSnapshot);
        Presentation->OnPresentationReset.RemoveDynamic(this, &ARemasterBattleStage::ResetPresentation);
    }
    for (int32 i = 0; i < 6; ++i) { ++LoadGeneration[i]; if (PendingLoads[i]) PendingLoads[i]->CancelHandle(); }
    RestoreCamera(); Super::EndPlay(Reason);
}
void ARemasterBattleStage::ApplyIdentity(int32 Slot, const FString& Key, int32 Height)
{
    if (ModelKeys[Slot] == Key) return;
    ModelKeys[Slot] = Key; const uint64 Generation = ++LoadGeneration[Slot];
    if (PendingLoads[Slot]) PendingLoads[Slot]->CancelHandle(); PendingLoads[Slot].Reset();
    Placeholders[Slot]->SetVisibility(true); StaticVisuals[Slot]->SetVisibility(false); SkeletalVisuals[Slot]->SetVisibility(false);
    const float Size = FMath::Clamp(Height, 25, 300) / 100.0f;
    Placeholders[Slot]->SetRelativeScale3D(FVector(Size)); Placeholders[Slot]->SetRelativeLocation(FVector(0,0,Size * 50));
    if (!LocalAssets) return;
    const auto* Binding = LocalAssets->Models.Find(Key);
    // Spinda personality spots have no material host yet. Never import one generic skin as all personalities.
    if (!Binding || Key.Contains(TEXT(".spinda.")) || !Binding->bUnrealImportValidated || !Binding->bSpecialChannelsInspected
        || !Binding->RequiredSpecialChannels.IsEmpty() || !Hash(Binding->SourceSha256)
        || !Hash(Binding->NormalizedSha256) || !Hash(Binding->ProvenanceSha256)
        || !FMath::IsFinite(Binding->UniformScale) || Binding->UniformScale < 0.01f || Binding->UniformScale > 100
        || !FMath::IsFinite(Binding->GroundOffsetCm) || FMath::Abs(Binding->GroundOffsetCm) > 300
        || Binding->SkeletalMesh.IsNull() == Binding->StaticMesh.IsNull()) return;
    TArray<FSoftObjectPath> Paths;
    if (!Binding->SkeletalMesh.IsNull()) Paths.Add(Binding->SkeletalMesh.ToSoftObjectPath());
    if (!Binding->StaticMesh.IsNull()) Paths.Add(Binding->StaticMesh.ToSoftObjectPath());
    for (const auto& Pair : Binding->Animations) if (!Pair.Value.IsNull()) Paths.Add(Pair.Value.ToSoftObjectPath());
    PendingLoads[Slot] = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,
        FStreamableDelegate::CreateWeakLambda(this, [this, Slot, Generation, Key]() { FinishAssetLoad(Slot, Generation, Key); }));
}
void ARemasterBattleStage::FinishAssetLoad(int32 Slot, uint64 Generation, const FString& Key)
{
    if (Generation != LoadGeneration[Slot] || Key != ModelKeys[Slot] || !LocalAssets) return;
    const auto* Binding = LocalAssets->Models.Find(Key); if (!Binding) return;
    const FVector Offset(0, 0, Binding->GroundOffsetCm), Scale(Binding->UniformScale);
    bool Loaded = false;
    if (auto* Mesh = Binding->SkeletalMesh.Get())
    {
        SkeletalVisuals[Slot]->SetSkeletalMesh(Mesh); SkeletalVisuals[Slot]->SetRelativeLocation(Offset);
        SkeletalVisuals[Slot]->SetRelativeScale3D(Scale); SkeletalVisuals[Slot]->SetVisibility(true); Loaded = true;
        PlaySemantic(Slot, TEXT("idle"), true);
    }
    else if (auto* Mesh = Binding->StaticMesh.Get())
    {
        StaticVisuals[Slot]->SetStaticMesh(Mesh); StaticVisuals[Slot]->SetRelativeLocation(Offset);
        StaticVisuals[Slot]->SetRelativeScale3D(Scale); StaticVisuals[Slot]->SetVisibility(true); Loaded = true;
    }
    Placeholders[Slot]->SetVisibility(!Loaded);
}
bool ARemasterBattleStage::PlaySemantic(int32 Slot, FName Name, bool Loop)
{
    if (!LocalAssets || !SkeletalVisuals[Slot]->IsVisible() || !SkeletalVisuals[Slot]->GetSkeletalMeshAsset()) return false;
    const auto* Binding = LocalAssets->Models.Find(ModelKeys[Slot]); if (!Binding) return false;
    const auto* Soft = Binding->Animations.Find(Name); UAnimSequence* Clip = Soft ? Soft->Get() : nullptr;
    if (!Clip || Clip->GetSkeleton() != SkeletalVisuals[Slot]->GetSkeletalMeshAsset()->GetSkeleton()) return false;
    SkeletalVisuals[Slot]->PlayAnimation(Clip, Loop); return true;
}
void ARemasterBattleStage::HandleCoreSnapshot(const FRemasterBattleSnapshot& InSnapshot)
{
    Snapshot = InSnapshot;
    SetActorHiddenInGame(Snapshot.bEnded && !bHasCue);
    if (!Snapshot.bEnded && !bCameraOwned)
    {
        CameraAnchor->SetRelativeLocation(FVector(-600, -650, 400));
        CameraAnchor->SetRelativeRotation((FVector(0, 0, 70) - CameraAnchor->GetRelativeLocation()).Rotation());
        BattleCamera->SetFieldOfView(45);
        if (auto* PC = GetGameInstance()->GetFirstLocalPlayerController())
        { PreviousViewTarget = PC->GetViewTarget(); bCameraOwned = true; PC->SetViewTarget(this); }
    }
    if (Snapshot.bEnded && !bHasCue) RestoreCamera();
    for (int32 i = 0; i < 4; ++i)
    {
        if (!Snapshot.Battlers.IsValidIndex(i)) continue;
        const auto& View = Snapshot.Battlers[i];
        if (ModelKeys[i] != View.AssetKey || View.Hp > 0) bFaintPresented[i] = false;
        SlotRoots[i]->SetHiddenInGame(!View.bActive || (bFaintPresented[i] && View.bFainted), true);
        if (!View.bActive) { ++LoadGeneration[i]; if (PendingLoads[i]) PendingLoads[i]->CancelHandle(); ModelKeys[i].Reset(); continue; }
        SlotRoots[i]->SetRelativeLocation(View.Position);
        SlotRoots[i]->SetRelativeRotation(FRotator(0, View.Side ? 180 : 0, 0));
        ApplyIdentity(i, View.AssetKey, View.DisplayHeightCm);
        Labels[i]->SetRelativeLocation(FVector(0, 0, View.DisplayHeightCm + 30));
        Labels[i]->SetText(FText::FromString(FString::Printf(TEXT("%s Sv %d · HP %d/%d"), *View.Name.ToString(), View.Level, View.Hp, View.MaxHp)));
    }
    for (int32 i = 4; i < 6; ++i)
    {
        SlotRoots[i]->SetHiddenInGame(!Snapshot.bTrainerBattle, true);
        SlotRoots[i]->SetRelativeLocation(FVector(i == 4 ? -320 : 320, 180, 0));
        SlotRoots[i]->SetRelativeRotation(FRotator(0, i == 4 ? 0 : 180, 0));
        ApplyIdentity(i, i == 4 ? TEXT("trainer.player.unresolved") : Snapshot.TrainerIdentity, 170);
        Labels[i]->SetRelativeLocation(FVector(0,0,200)); Labels[i]->SetText(i == 4 ? FText::FromString(TEXT("Oyuncu")) : Snapshot.TrainerName);
    }
}
void ARemasterBattleStage::HandlePresentationEvent(const FRemasterBattlePresentationEvent& Event)
{
    CurrentCue = Event; Elapsed = 0; bHasCue = true; HandleCoreSnapshot(Event.Snapshot);
    for (int32 i = 0; i < 6; ++i) bClipActive[i] = false;
    if (Event.Actor >= 0 && Event.Actor < 4) bClipActive[Event.Actor] = PlaySemantic(Event.Actor, Semantic(Event.MotionId), false);
    CameraAnchor->SetRelativeLocation(Event.CameraPosition);
    CameraAnchor->SetRelativeRotation((Event.CameraTarget - Event.CameraPosition).Rotation()); BattleCamera->SetFieldOfView(Event.CameraFov);
    if (auto* PC = GetGameInstance()->GetFirstLocalPlayerController())
    {
        if (!bCameraOwned) { PreviousViewTarget = PC->GetViewTarget(); bCameraOwned = true; }
        PC->SetViewTarget(this);
    }
    EventLabel->SetText(EventText(Event));
    BP_HandlePresentationEvent(Event);
}
void ARemasterBattleStage::RestoreCamera()
{
    if (bCameraOwned) if (auto* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
        if (PC->GetViewTarget() == this && PreviousViewTarget.IsValid()) PC->SetViewTarget(PreviousViewTarget.Get());
    bCameraOwned = false; PreviousViewTarget.Reset();
}
void ARemasterBattleStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); if (!bHasCue) return;
    Elapsed += FMath::Max(0.0f, DeltaSeconds);
    const double Progress = FMath::Clamp(Elapsed / FMath::Max(0.01f, CurrentCue.Duration), 0.0, 1.0);
    if (CurrentCue.Actor >= 0 && CurrentCue.Actor < 4 && Snapshot.Battlers.IsValidIndex(CurrentCue.Actor))
    {
        const auto& View = Snapshot.Battlers[CurrentCue.Actor];
        const auto Offset = bClipActive[CurrentCue.Actor] ? RemasterBattlePresentation::Position{}
            : RemasterBattlePresentation::FallbackOffset(static_cast<RemasterBattlePresentation::Motion>(CurrentCue.MotionId), Progress, View.Side);
        SlotRoots[CurrentCue.Actor]->SetRelativeLocation(View.Position + FVector(Offset.x, Offset.y, Offset.z));
    }
    const int32 Target = CurrentCue.TargetBattler;
    const bool Pulse = Snapshot.Battlers.IsValidIndex(Target) && Snapshot.Battlers[Target].bActive
        && CurrentCue.RawKind != REMASTER_EMERALD_BATTLE_EVENT_STARTED && CurrentCue.RawKind != REMASTER_EMERALD_BATTLE_EVENT_ENDED;
    EffectPulse->SetVisibility(Pulse);
    if (Pulse) { EffectPulse->SetRelativeLocation(Snapshot.Battlers[Target].Position + FVector(0,0,50)); EffectPulse->SetRelativeScale3D(FVector(0.15f + 0.4f * FMath::Sin(Progress * PI))); }
    if (Progress < 1) return;
    if (CurrentCue.Actor >= 0 && CurrentCue.Actor < 4 && Snapshot.Battlers.IsValidIndex(CurrentCue.Actor))
    {
        const auto& View = Snapshot.Battlers[CurrentCue.Actor]; SlotRoots[CurrentCue.Actor]->SetRelativeLocation(View.Position);
        if (CurrentCue.MotionId == static_cast<int32>(RemasterBattlePresentation::Motion::Faint))
        { bFaintPresented[CurrentCue.Actor] = true; SlotRoots[CurrentCue.Actor]->SetHiddenInGame(true, true); }
        else PlaySemantic(CurrentCue.Actor, TEXT("idle"), true);
    }
    const auto Finished = CurrentCue; bHasCue = false; EffectPulse->SetVisibility(false);
    if (Finished.RawKind == REMASTER_EMERALD_BATTLE_EVENT_ENDED) { RestoreCamera(); SetActorHiddenInGame(true); }
    if (auto* Presentation = GetGameInstance()->GetSubsystem<URemasterBattlePresentationSubsystem>())
        Presentation->CompletePresentation(Finished.Epoch, Finished.Token);
}
void ARemasterBattleStage::ResetPresentation()
{
    bHasCue = false; EffectPulse->SetVisibility(false); RestoreCamera(); SetActorHiddenInGame(true);
    for (int32 i = 0; i < 6; ++i)
    {
        ++LoadGeneration[i]; if (PendingLoads[i]) PendingLoads[i]->CancelHandle(); PendingLoads[i].Reset(); ModelKeys[i].Reset();
    }
    for (int32 i = 0; i < 4; ++i) bFaintPresented[i] = false;
}
