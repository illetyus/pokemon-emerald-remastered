#include "RemasterBattlePresentationSubsystem.h"
#include "Engine/GameInstance.h"
#include "RemasterUISubsystem.h"

namespace {
FVector Vector(const RemasterBattlePresentation::Position& P) { return FVector(P.x, P.y, P.z); }
FRemasterBattleSnapshot ToFrame(const RemasterBattlePresentation::Snapshot& Snapshot)
{
    FRemasterBattleSnapshot Out;
    Out.bTrainerBattle = Snapshot.trainerBattle; Out.bEnded = Snapshot.ended;
    Out.Outcome = Snapshot.outcome; Out.Weather = Snapshot.weather; Out.TrainerId = Snapshot.trainerId;
    Out.TrainerIdentity = UTF8_TO_TCHAR(Snapshot.trainerIdentity.c_str());
    Out.TrainerName = FText::FromString(UTF8_TO_TCHAR(Snapshot.trainerName.c_str()));
    for (const auto& View : Snapshot.battlers)
    {
        auto& Row = Out.Battlers.AddDefaulted_GetRef();
        Row.bActive = View.active; Row.bFainted = View.fainted;
        Row.CoreSpecies = View.coreSpecies; Row.NationalDex = View.nationalDex;
        Row.Name = FText::FromString(UTF8_TO_TCHAR(View.name.c_str()));
        Row.Form = UTF8_TO_TCHAR(View.form.c_str()); Row.AssetKey = UTF8_TO_TCHAR(View.assetKey.c_str());
        Row.Hp = View.hp; Row.MaxHp = View.maxHp; Row.Level = View.level; Row.Side = View.side;
        Row.DisplayHeightCm = View.displayHeightCm; Row.Position = Vector(View.position);
    }
    return Out;
}
ERemasterBattlePresentationEventType Type(unsigned Kind)
{
    using T = ERemasterBattlePresentationEventType;
    switch (Kind) {
    case REMASTER_EMERALD_BATTLE_EVENT_STARTED: return T::BattleStarted;
    case REMASTER_EMERALD_BATTLE_EVENT_SWITCH: return T::BattlerEntered;
    case REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED: return T::MoveStarted;
    case REMASTER_EMERALD_BATTLE_EVENT_DAMAGE: return T::Hit;
    case REMASTER_EMERALD_BATTLE_EVENT_CRITICAL: return T::CriticalHit;
    case REMASTER_EMERALD_BATTLE_EVENT_STATUS: return T::StatusChanged;
    case REMASTER_EMERALD_BATTLE_EVENT_HEAL: return T::HpChanged;
    case REMASTER_EMERALD_BATTLE_EVENT_WEATHER: return T::WeatherChanged;
    case REMASTER_EMERALD_BATTLE_EVENT_FAINT: return T::BattlerFainted;
    case REMASTER_EMERALD_BATTLE_EVENT_ENDED: return T::BattleEnded;
    default: return T::OtherCoreEvent;
    }
}
}

bool URemasterBattlePresentationSubsystem::BeginCoreBattle(uint64 Epoch)
{
    if (Epoch > static_cast<uint64>(MAX_int64) || !Feed.Begin(Epoch)) return false;
    History.Reset(); bHasCue = false; Latest = {};
    OnPresentationReset.Broadcast();
    if (auto* UI = GetGameInstance()->GetSubsystem<URemasterUISubsystem>()) UI->SetBattleHostBusy(true);
    return true;
}
bool URemasterBattlePresentationSubsystem::PublishCoreBatch(uint64 Epoch, uint64 BatchSerial,
    const RemasterEmeraldBattleState& Core, SIZE_T FirstEvent)
{
    if (!Feed.Submit(Epoch, BatchSerial, Core, FirstEvent)) return false;
    Latest = ToFrame(Feed.Latest()); OnCoreSnapshot.Broadcast(Latest);
    Pump(); return true;
}
bool URemasterBattlePresentationSubsystem::EndCoreBattle(uint64 Epoch, const RemasterEmeraldBattleState& Core)
{
    if (!Epoch || Epoch != Feed.Epoch() || !Core.ended) return false;
    // Core host completion releases input; visual timing cannot release it.
    if (auto* UI = GetGameInstance()->GetSubsystem<URemasterUISubsystem>()) UI->SetBattleHostBusy(false);
    return true;
}
bool URemasterBattlePresentationSubsystem::ResyncCoreSnapshot(uint64 Epoch, uint64 BatchSerial, const RemasterEmeraldBattleState& Core)
{
    if (!Feed.Resync(Epoch, BatchSerial, Core)) return false;
    bHasCue = false; OnPresentationReset.Broadcast(); Latest = ToFrame(Feed.Latest()); OnCoreSnapshot.Broadcast(Latest);
    UE_LOG(LogTemp, Warning, TEXT("Battle visual resync; skipped queued cues=%llu"), static_cast<unsigned long long>(Feed.DroppedVisualCues()));
    return true;
}
bool URemasterBattlePresentationSubsystem::CompletePresentation(int64 Epoch, int64 Token)
{
    if (Epoch <= 0 || Token <= 0 || !Feed.Complete(static_cast<uint64>(Epoch), static_cast<uint64>(Token))) return false;
    bHasCue = false; Pump(); return true;
}
void URemasterBattlePresentationSubsystem::Clear()
{
    Feed.ClearPresentation(); History.Reset(); bHasCue = false; Latest = {};
    OnPresentationReset.Broadcast();
    // Clearing visuals cannot end, advance or commit a battle.
}
bool URemasterBattlePresentationSubsystem::GetActiveCue(FRemasterBattlePresentationEvent& OutEvent) const
{
    if (!bHasCue) return false;
    OutEvent = ActiveCue; return true;
}
void URemasterBattlePresentationSubsystem::Pump()
{
    RemasterBattlePresentation::Cue Cue;
    if (!Feed.Next(Cue)) return;
    FRemasterBattlePresentationEvent Event;
    Event.Type = Type(Cue.event.kind); Event.RawKind = Cue.event.kind;
    Event.SourceBattler = Cue.event.battler; Event.TargetBattler = Cue.event.target;
    Event.MoveId = Cue.event.move_id; Event.Value = Cue.event.value; Event.Aux = Cue.event.aux;
    Event.Tag = FName(UTF8_TO_TCHAR(Cue.identity.c_str())); Event.VfxIdentity = FName(UTF8_TO_TCHAR(Cue.vfxIdentity.c_str()));
    Event.Epoch = static_cast<int64>(Cue.epoch); Event.Token = static_cast<int64>(Cue.token);
    Event.MotionId = static_cast<int32>(Cue.motion); Event.ShotId = static_cast<int32>(Cue.shot);
    Event.Actor = Cue.actor; Event.Duration = static_cast<float>(Cue.seconds); Event.Snapshot = ToFrame(Cue.snapshot);
    const auto Camera = RemasterBattlePresentation::Camera(Cue);
    Event.CameraPosition = Vector(Camera.location); Event.CameraTarget = Vector(Camera.target); Event.CameraFov = static_cast<float>(Camera.fov);
    ActiveCue = Event; bHasCue = true;
    if (History.Num() >= 256) History.RemoveAt(0);
    History.Add(Event); OnBattlePresentationEvent.Broadcast(Event);
}
