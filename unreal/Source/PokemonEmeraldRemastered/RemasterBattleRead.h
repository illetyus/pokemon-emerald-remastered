#pragma once

// Engine-independent presentation consumer. Never retains a mutable core pointer.
extern "C" {
#include "remaster/emerald_battle.h"
}
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>

namespace RemasterBattlePresentation {
struct SpeciesEntry { unsigned coreSpecies, nationalDex, heightCm; const char* name; };
#include "RemasterBattleCatalog.inl"

inline const SpeciesEntry* ResolveSpecies(unsigned species) {
    // Extended Unown graphics IDs have one National Dex identity.
    if (species >= 413 && species <= 439) species = 201;
    for (const auto& entry : SpeciesCatalog) if (entry.coreSpecies == species) return &entry;
    return nullptr; // Egg, old Unown placeholders and invalid IDs are explicit gaps.
}
inline std::string Form(unsigned species, uint32_t personality, uint8_t currentType) {
    const auto* entry = ResolveSpecies(species);
    if (!entry) return "unsupported";
    if (entry->nationalDex == 201) {
        unsigned form = (((personality & 0x03000000u) >> 18u) | ((personality & 0x00030000u) >> 12u)
            | ((personality & 0x00000300u) >> 6u) | (personality & 3u)) % 28u;
        if (species >= 413 && species <= 439) form = species - 412;
        return "unown." + std::to_string(form);
    }
    if (entry->nationalDex == 351)
        return currentType == 10 ? "sunny" : currentType == 11 ? "rainy" : currentType == 15 ? "snowy" : "normal";
    if (entry->nationalDex == 386) return "speed"; // Source Emerald, never ORAS normal substitution.
    if (entry->nationalDex == 327) return "spots." + std::to_string(personality);
    return "base";
}
inline bool Shiny(uint32_t ot, uint32_t personality) {
    // Source visual selection only: GET_SHINY_VALUE / SHINY_ODDS, no RNG calls.
    return ((ot >> 16u) ^ (ot & 65535u) ^ (personality >> 16u) ^ (personality & 65535u)) < 8u;
}
inline std::string AssetKey(unsigned species, uint32_t personality, uint32_t ot, uint8_t currentType) {
    const auto* entry = ResolveSpecies(species);
    if (!entry) return "pokemon.unsupported";
    // Spinda appearance needs a verified material implementation, not one mesh per personality.
    const auto form = entry->nationalDex == 327 ? "spinda" : Form(species, personality, currentType);
    return "pokemon." + std::to_string(entry->nationalDex) + "." + form + (Shiny(ot, personality) ? ".shiny" : ".normal");
}
struct Position { double x = 0, y = 0, z = 0; };
inline Position SlotPosition(unsigned side, unsigned slot, bool doubles) {
    return {side == 0 ? -180.0 : 180.0, doubles ? (slot == 0 ? -90.0 : 90.0) : 0.0, 0.0};
}
struct BattlerView {
    bool active = false, fainted = false, shiny = false;
    uint16_t coreSpecies = 0, hp = 0, maxHp = 0;
    uint8_t side = 0, sideSlot = 0, level = 0, currentType = 0;
    uint32_t personality = 0, otId = 0, status = 0;
    unsigned nationalDex = 0, displayHeightCm = 80;
    std::string name, form, assetKey;
    Position position;
};
struct Snapshot {
    std::array<BattlerView, 4> battlers;
    bool doubles = false, trainerBattle = false, ended = false;
    uint8_t outcome = 0, weather = 0;
    uint32_t turn = 0;
    uint16_t trainerId = 0;
    uint8_t trainerPic = 255;
    std::string trainerIdentity = "trainer.unsupported", trainerName;
};
inline bool Read(const RemasterEmeraldBattleState& core, Snapshot& out) {
    if (core.event_count > REMASTER_EMERALD_BATTLE_EVENT_CAPACITY) return false;
    Snapshot snapshot;
    snapshot.doubles = (core.battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_DOUBLE) != 0;
    snapshot.trainerBattle = (core.battle_type_flags & REMASTER_EMERALD_BATTLE_TYPE_TRAINER) != 0;
    snapshot.ended = core.ended != 0; snapshot.outcome = core.outcome;
    snapshot.weather = core.weather; snapshot.turn = core.turn_number;
    snapshot.trainerId = core.opponent_trainer_id;
    if (snapshot.trainerBattle) if (const auto* trainer = remaster_emerald_battle_trainer_find(core.opponent_trainer_id)) {
        snapshot.trainerPic = trainer->trainer_pic;
        snapshot.trainerIdentity = TrainerIdentity(trainer->trainer_pic);
        snapshot.trainerName.assign(trainer->trainer_name, std::find(trainer->trainer_name,
            trainer->trainer_name + sizeof trainer->trainer_name, '\0'));
    }
    unsigned sideSlots[2]{};
    for (unsigned i = 0; i < 4; ++i) {
        const auto& mon = core.battlers[i];
        if (!mon.active) continue;
        if (mon.side > 1 || sideSlots[mon.side] >= (snapshot.doubles ? 2u : 1u)
            || mon.pokemon.hp > mon.pokemon.max_hp) return false;
        auto& view = snapshot.battlers[i];
        view.active = true; view.fainted = mon.fainted != 0;
        view.side = mon.side; view.sideSlot = static_cast<uint8_t>(sideSlots[mon.side]++);
        view.coreSpecies = mon.species; view.hp = mon.pokemon.hp; view.maxHp = mon.pokemon.max_hp;
        view.level = mon.pokemon.level; view.currentType = mon.types[0]; view.status = mon.pokemon.status;
        view.personality = mon.pokemon.box.personality; view.otId = mon.pokemon.box.ot_id;
        view.shiny = Shiny(view.otId, view.personality);
        if (const auto* species = ResolveSpecies(mon.species)) {
            view.nationalDex = species->nationalDex; view.name = species->name;
            // Height is an explicit primitive fallback display policy, not measured model calibration.
            view.displayHeightCm = std::clamp(species->heightCm, 25u, 300u);
        } else view.name = "Unknown #" + std::to_string(mon.species);
        view.form = Form(mon.species, view.personality, view.currentType);
        view.assetKey = AssetKey(mon.species, view.personality, view.otId, view.currentType);
        view.position = SlotPosition(view.side, view.sideSlot, snapshot.doubles);
    }
    out = std::move(snapshot); return true;
}
enum class Motion { Idle, Entry, Attack, Hit, Faint };
enum class Shot { Wide, Move, Impact, Faint, Result };
struct Cue {
    uint64_t epoch = 0, serial = 0, token = 0;
    RemasterEmeraldBattleEvent event{};
    Snapshot snapshot;
    Motion motion = Motion::Idle;
    Shot shot = Shot::Wide;
    int actor = -1;
    double seconds = 0.12;
    std::string identity, vfxIdentity;
};
inline Cue MakeCue(const RemasterEmeraldBattleEvent& event, const Snapshot& snapshot) {
    Cue cue; cue.event = event; cue.snapshot = snapshot; cue.identity = EventIdentity(event.kind);
    cue.vfxIdentity = "vfx." + cue.identity.substr(7);
    switch (event.kind) {
    case REMASTER_EMERALD_BATTLE_EVENT_STARTED:
    case REMASTER_EMERALD_BATTLE_EVENT_SWITCH:
        cue.motion = Motion::Entry; cue.actor = event.battler < 4 ? event.battler : -1; cue.seconds = 0.4; break;
    case REMASTER_EMERALD_BATTLE_EVENT_MOVE_USED:
        cue.motion = Motion::Attack; cue.shot = Shot::Move; cue.actor = event.battler < 4 ? event.battler : -1; cue.seconds = 0.35; break;
    case REMASTER_EMERALD_BATTLE_EVENT_DAMAGE:
        cue.motion = Motion::Hit; cue.shot = Shot::Impact;
        // aux=1 is source substitute damage, not damage to the Pokémon model.
        cue.actor = event.aux == 0 && event.target < 4 ? event.target : -1; cue.seconds = 0.2; break;
    case REMASTER_EMERALD_BATTLE_EVENT_FAINT:
        cue.motion = Motion::Faint; cue.shot = Shot::Faint; cue.actor = event.battler < 4 ? event.battler : -1; cue.seconds = 0.45; break;
    case REMASTER_EMERALD_BATTLE_EVENT_ENDED:
        cue.shot = Shot::Result; cue.seconds = 0.35; break;
    default: break;
    }
    return cue;
}

class Feed {
public:
    static constexpr std::size_t Capacity = 1024;
    bool Begin(uint64_t epoch) {
        if (!epoch || epoch <= currentEpoch) return false;
        currentEpoch = epoch; lastSerial = 0; nextToken = 1; queue.clear(); current = {}; active = false; latest = {};
        return true;
    }
    bool Submit(uint64_t epoch, uint64_t serial, const RemasterEmeraldBattleState& core, std::size_t firstEvent = 0) {
        if (!currentEpoch || epoch != currentEpoch || serial != lastSerial + 1
            || !serial || core.event_count > REMASTER_EMERALD_BATTLE_EVENT_CAPACITY || firstEvent > core.event_count)
            return false;
        const auto count = core.event_count - firstEvent;
        if (queue.size() + count + (active ? 1 : 0) > Capacity) return false;
        Snapshot snapshot;
        if (!Read(core, snapshot)) return false;
        for (std::size_t i = firstEvent; i < core.event_count; ++i) {
            auto cue = MakeCue(core.events[i], snapshot);
            cue.epoch = epoch; cue.serial = serial; cue.token = nextToken++;
            queue.push_back(std::move(cue));
        }
        latest = std::move(snapshot); lastSerial = serial; return true;
    }
    bool Next(Cue& out) {
        if (active || queue.empty()) return false;
        current = std::move(queue.front()); queue.pop_front(); active = true;
        out = current; return true;
    }
    bool Complete(uint64_t epoch, uint64_t token) {
        if (!active || epoch != currentEpoch || current.epoch != epoch || current.token != token) return false;
        active = false; return true; // Absolutely no battle API or gameplay callback.
    }
    void ClearPresentation() { queue.clear(); active = false; current = {}; }
    bool Resync(uint64_t epoch, uint64_t serial, const RemasterEmeraldBattleState& core) {
        if (!currentEpoch || epoch != currentEpoch || serial <= lastSerial) return false;
        Snapshot snapshot;
        if (!Read(core, snapshot)) return false;
        droppedVisualCues += Pending(); ClearPresentation(); latest = std::move(snapshot); lastSerial = serial;
        return true; // Explicit visual discontinuity; never stalls or rewrites battle state.
    }
    const Snapshot& Latest() const { return latest; }
    std::size_t Pending() const { return queue.size() + (active ? 1 : 0); }
    uint64_t Epoch() const { return currentEpoch; }
    std::size_t DroppedVisualCues() const { return droppedVisualCues; }
private:
    uint64_t currentEpoch = 0, lastSerial = 0, nextToken = 1;
    std::deque<Cue> queue;
    Cue current;
    bool active = false;
    Snapshot latest;
    std::size_t droppedVisualCues = 0;
};

inline Position FallbackOffset(Motion motion, double progress, unsigned side) {
    const double t = std::clamp(progress, 0.0, 1.0), inward = side == 0 ? 1.0 : -1.0;
    switch (motion) {
    case Motion::Entry: return {-inward * 80.0 * (1 - t), 0, 0};
    case Motion::Attack: return {inward * std::sin(t * 3.141592653589793) * 30.0, 0, 0};
    case Motion::Hit: return {0, std::sin(t * 4 * 3.141592653589793) * 8.0 * (1 - t), 0};
    case Motion::Faint: return {0, 0, -60 * t};
    default: return {};
    }
}
struct CameraView { Position location, target; double fov = 45; };
inline CameraView Camera(const Cue& cue) {
    Position target{0, 0, 70};
    if (cue.actor >= 0 && cue.actor < 4 && cue.shot != Shot::Wide && cue.shot != Shot::Result) {
        target = cue.snapshot.battlers[static_cast<unsigned>(cue.actor)].position;
        target.z = cue.snapshot.battlers[static_cast<unsigned>(cue.actor)].displayHeightCm * 0.5;
    }
    return {{-600, -650, 400}, target, cue.shot == Shot::Wide || cue.shot == Shot::Result ? 45.0 : 35.0};
}
}
