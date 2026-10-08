#pragma once

// This model is shared by Unreal and native tests. No Unreal or platform types.
extern "C" {
#include "remaster/emerald_items.h"
#include "remaster/emerald_qol.h"
#include "remaster/emerald_quest.h"
#include "remaster/emerald_rtc.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_script_runtime.h"
}
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace RemasterUi {
#include "RemasterUiCatalog.inl"

enum class Screen { Hud, Menu, Map, Quest, Party, Bag, Item, Summary,
    Settings, Rtc, SaveLoad, ConfirmSave, ConfirmLoad, Dialogue, Relearner,
    ReplaceMove, Nickname, QuickItems, Repel };
enum class Action { Up, Down, Left, Right, Confirm, Cancel, Menu, Map, Quest, QuickItem };
enum class Command { None, Open, SelectParty, SelectItem, Sort, AutoSort,
    RegisterItem, UnregisterItem, Relearn, SelectMove, ReplaceMove, Nickname,
    ToggleText, ToggleRepeat, RtcDays, RtcHours, RtcMinutes, ApplyRtc, Save,
    Load, DialogueComplete, FieldItem };
enum class Effect { None, Save, Load, ApplyRtc, Nickname, FieldItem, DialogueComplete };
struct Row {
    std::string label;
    Command command = Command::None;
    int value = 0;
    bool enabled = true;
};
struct Frame {
    Screen screen = Screen::Hud;
    std::string title, body;
    std::vector<Row> rows;
    unsigned selected = 0;
    std::uint64_t revision = 1;
    bool modal = false;
    bool hasMarker = false;
    int markerX = 0, markerY = 0, markerWidth = 0, markerHeight = 0;
};
struct Context {
    // Host facts: not inferred from presentation or timer state.
    bool mapReady = false, scriptBusy = false, battleBusy = false;
    std::string mapName;
    int mapGroup = -1, mapNum = -1;
};
struct Intent {
    Effect effect = Effect::None;
    int value = 0;
    std::uint64_t revision = 0, fingerprint = 0;
    RemasterEmeraldTime time{};
};

inline bool Usable(const RemasterEmeraldSave* save) {
    return save && (save->status == REMASTER_EMERALD_SAVE_OK
        || save->status == REMASTER_EMERALD_SAVE_DEGRADED);
}
inline std::uint64_t Fingerprint(const RemasterEmeraldSave* save) {
    if (!save) return 0;
    std::uint64_t hash = 14695981039346656037ULL;
    const auto bytes = [&hash](const uint8_t* p, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) { hash ^= p[i]; hash *= 1099511628211ULL; }
    };
    // Never hash struct padding. Include provenance/status and all persistent domains.
    const uint8_t status[] = {static_cast<uint8_t>(save->status), save->source_is_stock};
    bytes(status, sizeof status);
    bytes(save->save_block1, sizeof save->save_block1);
    bytes(save->save_block2, sizeof save->save_block2);
    bytes(save->pokemon_storage, sizeof save->pokemon_storage);
    bytes(save->stock_item_metadata, sizeof save->stock_item_metadata);
    return hash;
}
inline std::string Name(const char* name, unsigned id) {
    return name ? std::string(name) : "#" + std::to_string(id);
}
inline std::string Nickname(const RemasterEmeraldBoxPokemon& mon) {
    std::string text;
    for (auto byte : mon.nickname) {
        if (byte == 255) break;
        text += mon.language == 1 ? JapaneseNicknameGlyph(byte) : NicknameGlyph(byte);
    }
    return text;
}

// 60 Hz presentation ticks. It cannot synthesize world movement or confirm actions.
struct Repeat {
    unsigned held = 0;
    bool Tick(bool pressed, bool enabled) {
        if (!pressed || !enabled) { held = 0; return false; }
        ++held;
        return held == 24 || (held > 24 && (held - 24) % 3 == 0);
    }
};

class Model {
public:
    Frame frame;
    bool fastText = true, repeatEnabled = true;
    std::string notice;

    bool Modal() const { return screen != Screen::Hud; }
    bool CanAct(const RemasterEmeraldSave* save, const Context& context) const {
        return Usable(save) && context.mapReady && !context.scriptBusy && !context.battleBusy;
    }
    void Reset() {
        screen = Screen::Hud; stack.clear(); selected = 0; partySlot = 0;
        itemId = 0; moveId = 0; dialogueReady = false; visibleGlyphs = 0;
        ++frame.revision;
    }
    void Open(Screen target) {
        if (target == screen) return;
        if (stack.size() >= 16) return;
        stack.push_back({screen, selected}); screen = target; selected = 0;
        ++frame.revision;
    }
    void Back() {
        if (screen == Screen::Dialogue) return; // Core owns dialogue lifetime.
        if (stack.empty()) { screen = Screen::Hud; selected = 0; }
        else { screen = stack.back().first; selected = stack.back().second; stack.pop_back(); }
        ++frame.revision;
    }
    bool Input(Action action, const RemasterEmeraldSave* save, const Context& context) {
        const bool wasModal = Modal();
        if (action == Action::Cancel) { if (wasModal) Back(); return wasModal; }
        if (action == Action::Menu || action == Action::Map || action == Action::Quest
            || action == Action::QuickItem) {
            if (wasModal || context.scriptBusy || context.battleBusy
                || (action != Action::Menu && !CanAct(save, context))) return true;
            Open(action == Action::Menu ? Screen::Menu : action == Action::Map ? Screen::Map
                : action == Action::Quest ? Screen::Quest : Screen::QuickItems);
            return true;
        }
        if (!wasModal) return context.scriptBusy || context.battleBusy || !CanAct(save, context);
        if (screen == Screen::Bag && (action == Action::Left || action == Action::Right)) {
            pocket = static_cast<uint8_t>((pocket + (action == Action::Left ? 3 : 5)) % 5 + 1);
            ++frame.revision;
        }
        if (!frame.rows.empty() && (action == Action::Up || action == Action::Down)) {
            const unsigned count = static_cast<unsigned>(frame.rows.size());
            selected = (selected + (action == Action::Up ? count - 1 : 1)) % count;
            ++frame.revision;
        }
        return true;
    }

    // Host resolves resources from the pending core request, including dynamic text.
    // A sequence cannot be repurposed, duplicated or completed by menu cancellation.
    bool PresentDialogue(const RemasterEmeraldScriptRequest& request,
                         const std::string& resolvedText, const std::vector<Row>& choices) {
        if (request.type != REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE
            && request.type != REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE) return false;
        if (dialogueReady && request.sequence == dialogueRequest.sequence) return false;
        if (request.sequence == 0) return false;
        if (request.type == REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE && choices.empty()) return false;
        if (request.type == REMASTER_EMERALD_SCRIPT_REQUEST_MESSAGE
            && request.action == REMASTER_EMERALD_SCRIPT_MESSAGE_SHOW && resolvedText.empty()) return false;
        dialogueRequest = request; dialogueText = resolvedText; dialogueChoices = choices;
        for (auto& row : dialogueChoices) row.command = Command::DialogueComplete;
        dialogueReady = true; visibleGlyphs = 0;
        // Core dialogue supersedes menus, rather than preserving stale UI commands.
        screen = Screen::Dialogue; stack.clear(); selected = 0; ++frame.revision;
        return true;
    }
    void TextTick() {
        if (!dialogueReady) return;
        unsigned remaining = fastText ? 4 : 1;
        while (visibleGlyphs < dialogueText.size() && remaining--) {
            ++visibleGlyphs;
            while (visibleGlyphs < dialogueText.size()
                && (static_cast<unsigned char>(dialogueText[visibleGlyphs]) & 0xC0) == 0x80)
                ++visibleGlyphs;
        }
    }
    bool CompleteDialogue(RemasterEmeraldScriptRuntime* runtime, const Intent& intent) {
        RemasterEmeraldScriptRequest pending{};
        if (!runtime || intent.effect != Effect::DialogueComplete || !dialogueReady
            || intent.revision != frame.revision
            || intent.fingerprint != coreFingerprint || Fingerprint(runtime->vm.save) != intent.fingerprint
            || !remaster_emerald_script_runtime_pending_request(runtime, &pending)
            || pending.sequence != dialogueRequest.sequence || pending.type != dialogueRequest.type
            || pending.program_index != dialogueRequest.program_index || pending.pc != dialogueRequest.pc)
            return false;
        if (pending.type == REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE
            && std::none_of(dialogueChoices.begin(), dialogueChoices.end(), [&intent](const Row& row) {
                return row.enabled && row.value == intent.value;
            })) return false;
        RemasterEmeraldScriptCompletion completion{};
        completion.type = pending.type; completion.sequence = pending.sequence;
        completion.result_u16 = static_cast<uint16_t>(intent.value); completion.accepted = 1;
        if (!remaster_emerald_script_runtime_complete(runtime, &completion)) return false;
        dialogueReady = false; screen = Screen::Hud; ++frame.revision;
        return true; // Never run the VM or acknowledge audio/movement/delay barriers here.
    }
    void Build(const RemasterEmeraldSave* save, const Context& context);
    Intent Activate(unsigned row, std::uint64_t revision,
                    RemasterEmeraldSave* save, const Context& context);

private:
    Screen screen = Screen::Hud;
    unsigned selected = 0;
    std::vector<std::pair<Screen, unsigned>> stack;
    uint8_t partySlot = 0, pocket = REMASTER_EMERALD_POCKET_ITEMS;
    uint16_t itemId = 0, moveId = 0;
    std::uint64_t coreFingerprint = 0;
    std::string contextKey;
    RemasterEmeraldTime rtcDraft{};
    bool rtcInitialized = false, dialogueReady = false;
    RemasterEmeraldScriptRequest dialogueRequest{};
    std::string dialogueText;
    std::vector<Row> dialogueChoices;
    std::size_t visibleGlyphs = 0;
};
}
