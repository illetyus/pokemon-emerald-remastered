#include "RemasterNavigationSubsystem.h"

#include "RemasterVanillaPlusSaveSubsystem.h"
#include "RemasterWorldGameplaySubsystem.h"

extern "C"
{
#include "remaster/emerald_quest.h"
#include "remaster/emerald_save.h"
}

namespace
{
ERemasterObjectiveTargetType ToUnrealTargetType(uint8 Type)
{
    switch (static_cast<RemasterEmeraldQuestTargetType>(Type))
    {
    case REMASTER_EMERALD_QUEST_TARGET_REGION:
        return ERemasterObjectiveTargetType::Region;
    case REMASTER_EMERALD_QUEST_TARGET_MAP:
        return ERemasterObjectiveTargetType::Map;
    case REMASTER_EMERALD_QUEST_TARGET_OBJECT_EVENT:
        return ERemasterObjectiveTargetType::ObjectEvent;
    case REMASTER_EMERALD_QUEST_TARGET_COORDINATE:
        return ERemasterObjectiveTargetType::Coordinate;
    case REMASTER_EMERALD_QUEST_TARGET_NONE:
    default:
        return ERemasterObjectiveTargetType::None;
    }
}
}

void URemasterNavigationSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<URemasterVanillaPlusSaveSubsystem>();
    Collection.InitializeDependency<URemasterWorldGameplaySubsystem>();

    Super::Initialize(Collection);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterWorldGameplaySubsystem* World =
                GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
        {
            World->OnGameplayMapChanged.AddDynamic(
                this,
                &URemasterNavigationSubsystem::HandleGameplayMapChanged);
        }
    }

    RefreshFromCore();
}

void URemasterNavigationSubsystem::Deinitialize()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (URemasterWorldGameplaySubsystem* World =
                GI->GetSubsystem<URemasterWorldGameplaySubsystem>())
        {
            World->OnGameplayMapChanged.RemoveDynamic(
                this,
                &URemasterNavigationSubsystem::HandleGameplayMapChanged);
        }
    }

    ClearDerivedObjective();
    Super::Deinitialize();
}

bool URemasterNavigationSubsystem::RefreshFromCore()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        ClearDerivedObjective();
        return false;
    }

    const URemasterVanillaPlusSaveSubsystem* SaveSubsystem =
        GI->GetSubsystem<URemasterVanillaPlusSaveSubsystem>();
    const URemasterWorldGameplaySubsystem* World =
        GI->GetSubsystem<URemasterWorldGameplaySubsystem>();

    if (!SaveSubsystem
        || !SaveSubsystem->HasUsableSave()
        || !SaveSubsystem->GetNativeSaveHandle())
    {
        ClearDerivedObjective();
        return false;
    }

    const RemasterEmeraldSave* Save =
        static_cast<const RemasterEmeraldSave*>(
            SaveSubsystem->GetNativeSaveHandle());

    const RemasterEmeraldQuestObjective* Native =
        remaster_emerald_quest_active(Save);

    if (!Native)
    {
        ClearDerivedObjective();
        return true;
    }

    FRemasterQuestObjective Derived;
    Derived.ObjectiveId = static_cast<int32>(Native->id);
    Derived.QuestId = FName(UTF8_TO_TCHAR(Native->key));
    Derived.Title = FText::FromString(UTF8_TO_TCHAR(Native->title));
    Derived.Description =
        FText::FromString(UTF8_TO_TCHAR(Native->description));
    Derived.MapSectionId = static_cast<int32>(Native->map_section_id);
    Derived.RegionMarkerX =
        static_cast<int32>(Native->region_marker.x);
    Derived.RegionMarkerY =
        static_cast<int32>(Native->region_marker.y);
    Derived.RegionMarkerWidth =
        static_cast<int32>(Native->region_marker.width);
    Derived.RegionMarkerHeight =
        static_cast<int32>(Native->region_marker.height);
    Derived.MapGroup = static_cast<int32>(Native->map_group);
    Derived.MapNum = static_cast<int32>(Native->map_num);
    Derived.LocalId = static_cast<int32>(Native->local_id);
    Derived.TileX = static_cast<int32>(Native->x);
    Derived.TileY = static_cast<int32>(Native->y);
    Derived.TargetType = ToUnrealTargetType(Native->target_type);

    if (World && World->IsMapReady())
    {
        Derived.bTargetMatchesCurrentMap =
            remaster_emerald_quest_target_matches_map(
                Native,
                static_cast<uint8>(World->GetCurrentMapGroup()),
                static_cast<uint8>(World->GetCurrentMapNumber())) != 0;

        if (Derived.bTargetMatchesCurrentMap
            && Derived.TargetType
                == ERemasterObjectiveTargetType::ObjectEvent
            && Derived.LocalId > 0)
        {
            bool bVisible = false;
            if (World->IsObjectVisible(Derived.LocalId, bVisible))
            {
                Derived.bTargetObjectVisible = bVisible;
            }
        }
    }

    ActiveObjective = MoveTemp(Derived);
    bHasObjective = true;
    OnActiveObjectiveChanged.Broadcast(ActiveObjective);
    return true;
}

void URemasterNavigationSubsystem::HandleGameplayMapChanged(
    int32 MapGroup,
    int32 MapNum,
    FString MapId)
{
    (void)MapGroup;
    (void)MapNum;
    (void)MapId;
    RefreshFromCore();
}

void URemasterNavigationSubsystem::ClearDerivedObjective()
{
    const bool bHadObjective = bHasObjective;

    ActiveObjective = FRemasterQuestObjective{};
    bHasObjective = false;

    if (bHadObjective)
    {
        OnActiveObjectiveChanged.Broadcast(ActiveObjective);
    }
}
