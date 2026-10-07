#include "RemasterUiModel.h"
extern "C" {
#include "remaster/platform.h"
}
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace RemasterUi;
static unsigned checks = 0;
static void Check(bool ok, const char* message) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "R9: %s\n", message); std::exit(1); }
}
static int Clock(void*, RemasterWallClock* clock) {
    *clock = {2026, 10, 7, 12, 34, 56}; return 1;
}
static RemasterEmeraldSave Seed() {
    RemasterEmeraldSave save{}; save.status = REMASTER_EMERALD_SAVE_OK;
    RemasterEmeraldPartyPokemon mon{};
    Check(remaster_emerald_box_pokemon_set_species(&mon.box, 2), "seed species");
    mon.level = 10; mon.hp = 23; mon.max_hp = 30; mon.attack = 19;
    mon.defense = 17; mon.speed = 15; mon.sp_attack = 21; mon.sp_defense = 20;
    mon.box.nickname[0] = 1; mon.box.nickname[1] = 255; mon.box.language = 2; mon.mail = 255;
    for (unsigned i = 0; i < 6; ++i) {
        Check(remaster_emerald_box_pokemon_set_iv(&mon.box, i, static_cast<uint8_t>(i + 20)), "seed iv");
        Check(remaster_emerald_box_pokemon_set_ev(&mon.box, i, static_cast<uint8_t>(i + 1)), "seed ev");
    }
    Check(remaster_emerald_party_set_count(&save, 1), "seed count");
    Check(remaster_emerald_party_set(&save, 0, &mon), "seed mon");
    RemasterEmeraldOverworldState world{};
    Check(remaster_emerald_overworld_get(&save, &world), "seed world read");
    world.money = 54321;
    Check(remaster_emerald_overworld_set(&save, &world), "seed world write");
    Check(remaster_emerald_bag_add(&save, 13, 5), "seed potion");
    Check(remaster_emerald_bag_add(&save, 86, 2), "seed repel");
    Check(remaster_emerald_bag_add(&save, 111, 2), "seed scales");
    Check(remaster_emerald_bag_add(&save, 259, 1), "seed bike");
    Check(remaster_emerald_flag_set(&save, 0x860, 1), "seed starter flag");
    return save;
}
static Context Free() { Context c; c.mapReady = true; c.mapName = "LittlerootTown"; c.mapGroup = 0; c.mapNum = 9; return c; }
static unsigned Find(const Frame& frame, Command command, int value = -999) {
    for (unsigned i = 0; i < frame.rows.size(); ++i)
        if (frame.rows[i].command == command && (value == -999 || frame.rows[i].value == value)) return i;
    Check(false, "expected row missing"); return 0;
}
static Intent Choose(Model& model, RemasterEmeraldSave& save, Command command, int value = -999) {
    model.Build(&save, Free());
    const auto intent = model.Activate(Find(model.frame, command, value), model.frame.revision, &save, Free());
    model.Build(&save, Free()); return intent;
}
static void ReadOnlyScreens() {
    auto save = Seed(); const auto before = save; const auto context = Free();
    for (const auto screen : {Screen::Hud, Screen::Menu, Screen::Map, Screen::Quest,
        Screen::Party, Screen::Bag, Screen::Item, Screen::Summary, Screen::Settings,
        Screen::Rtc, Screen::SaveLoad, Screen::ConfirmLoad, Screen::ConfirmSave,
        Screen::Relearner, Screen::ReplaceMove, Screen::Nickname, Screen::QuickItems, Screen::Repel}) {
        Model model; model.Open(screen); model.Build(&save, context);
        Check(!model.frame.title.empty(), "screen has title");
        Check(std::memcmp(&save, &before, sizeof save) == 0, "render changed save bytes");
    }
    Model summary; summary.Open(Screen::Summary); summary.Build(&save, context);
    Check(summary.frame.body.find("HP 23/30") != std::string::npos, "summary current core stats");
    Check(summary.frame.body.find("IV 20 · EV 1") != std::string::npos, "summary core iv/ev");
    Check(summary.frame.body.find("Toplam EV 21") != std::string::npos, "summary total ev core");
    Check(summary.frame.body.find("Ğ") != std::string::npos, "source Turkish nickname encoding");
    RemasterEmeraldBoxPokemon japanese{}; japanese.language = 1;
    japanese.nickname[0] = 1; japanese.nickname[1] = 255;
    Check(Nickname(japanese) == "あ", "source Japanese alias uses language page");
    const auto* objective = remaster_emerald_quest_active(&save);
    Check(objective && objective->id == REMASTER_EMERALD_QUEST_MEET_RIVAL_ROUTE103, "source first quest active");
    Model quest; quest.Open(Screen::Map); quest.Build(&save, context);
    Check(quest.frame.hasMarker && quest.frame.markerX == objective->region_marker.x
        && quest.frame.markerY == objective->region_marker.y, "R10 authoritative region marker consumed");
    Check(quest.frame.body.find(objective->title) != std::string::npos, "core quest title consumed");
    Check(std::memcmp(&save, &before, sizeof save) == 0, "quest view preserves all flags and vars");
    RemasterEmeraldSave corrupt = save; corrupt.status = REMASTER_EMERALD_SAVE_CORRUPT;
    Model bad; bad.Open(Screen::Party); bad.Build(&corrupt, context);
    Check(bad.frame.rows.empty(), "corrupt party is hidden");
    Check(!bad.CanAct(&corrupt, context), "corrupt save cannot act");
    Check(!bad.CanAct(nullptr, context), "missing save cannot act");
    save.save_block1[0x238 + 28] ^= 1; // Source party payload checksum corruption.
    Model invalid; invalid.Open(Screen::Party); invalid.Build(&save, context);
    Check(!invalid.frame.rows.empty() && !invalid.frame.rows[0].enabled, "checksum invalid party disabled");
}
static void NavigationAndTransactions() {
    auto save = Seed(); auto context = Free(); const auto before = save;
    Model model; model.Build(&save, context);
    Check(!model.Input(Action::Up, &save, context), "HUD directions stay world inputs");
    Check(model.Input(Action::Menu, &save, context), "menu consumes input"); model.Build(&save, context);
    Check(model.Modal(), "menu modal");
    model.Input(Action::Up, &save, context); model.Build(&save, context);
    Check(model.frame.selected == model.frame.rows.size() - 1, "focus wraps");
    model.Input(Action::Cancel, &save, context); model.Build(&save, context);
    Check(!model.Modal(), "cancel returns HUD");
    Check(std::memcmp(&save, &before, sizeof save) == 0, "navigation never writes core");
    model.Open(Screen::Bag); model.Build(&save, context);
    const auto revision = model.frame.revision; const auto row = Find(model.frame, Command::Sort, 3);
    Check(remaster_emerald_bag_add(&save, 13, 1), "external state update"); const auto external = save;
    model.Activate(row, revision, &save, context);
    Check(std::memcmp(&save, &external, sizeof save) == 0, "old core fingerprint rejected");
    model.Build(&save, context); model.Input(Action::Down, &save, context); model.Build(&save, context);
    model.Activate(row, revision, &save, context);
    Check(std::memcmp(&save, &external, sizeof save) == 0, "old view revision rejected");
    context.scriptBusy = true; model.Build(&save, context);
    model.Activate(Find(model.frame, Command::Sort, 3), model.frame.revision, &save, context);
    Check(std::memcmp(&save, &external, sizeof save) == 0, "busy command rejected without write");
    context.scriptBusy = false; model.Build(&save, context);
    const auto bagCount = remaster_emerald_bag_count(&save, 13);
    Choose(model, save, Command::Sort, 3);
    Check(remaster_emerald_bag_count(&save, 13) == bagCount, "sort preserves quantities");
    Check(remaster_emerald_qol_bag_sort_mode(&save, 1) == REMASTER_EMERALD_QOL_ITEM_SORT_QUANTITY, "sort uses native preference");
    Choose(model, save, Command::AutoSort);
    Check(remaster_emerald_qol_bag_auto_sort_enabled(&save, 1), "native auto-sort toggle");
    model.Input(Action::Left, &save, Free()); model.Build(&save, Free());
    Check(model.frame.body.find("Cep 5") != std::string::npos, "left pocket wraps");
    Choose(model, save, Command::SelectItem, 259);
    Choose(model, save, Command::RegisterItem);
    Check(remaster_emerald_qol_quick_item_count(&save) == 1, "native quick registration");
    Choose(model, save, Command::UnregisterItem);
    Check(remaster_emerald_qol_quick_item_count(&save) == 0, "native quick unregistration");
    Model badItem; badItem.Open(Screen::Item); badItem.Build(&save, Free()); const auto beforeBad = save;
    Choose(badItem, save, Command::RegisterItem);
    Check(std::memcmp(&save, &beforeBad, sizeof save) == 0, "rejected item preserves uninitialized metadata");
    Model noSave; noSave.Build(nullptr, Free()); noSave.Input(Action::Menu, nullptr, Free());
    noSave.Build(nullptr, Free()); Check(noSave.Modal(), "menu accessible without save for load");
    Model saveUi; saveUi.Open(Screen::ConfirmSave); const auto beforeIo = save;
    Check(Choose(saveUi, save, Command::Save).effect == Effect::Save, "save yields host intent");
    Check(std::memcmp(&save, &beforeIo, sizeof save) == 0, "save UI does not serialize itself");
    saveUi.Back(); saveUi.Open(Screen::ConfirmLoad);
    Check(Choose(saveUi, save, Command::Load).effect == Effect::Load, "load yields explicit host intent");
    Model rtc; rtc.Open(Screen::Rtc); rtc.Build(&save, Free());
    Choose(rtc, save, Command::RtcHours, 1);
    const auto timeIntent = Choose(rtc, save, Command::ApplyRtc);
    Check(timeIntent.effect == Effect::ApplyRtc && timeIntent.time.hours == 13, "RTC draft forwards to R1");
    Check(std::memcmp(&save, &beforeIo, sizeof save) == 0, "RTC draft is not gameplay clock write");
}
static void Relearner() {
    auto save = Seed(); Model model; model.Open(Screen::Relearner);
    Choose(model, save, Command::SelectMove);
    const auto count = remaster_emerald_bag_count(&save, 111);
    Choose(model, save, Command::ReplaceMove, 0);
    Check(remaster_emerald_bag_count(&save, 111) == count - 1, "native relearner consumes one scale");
    RemasterEmeraldPartyPokemon mon{}; int valid = 0;
    Check(remaster_emerald_party_get(&save, 0, &mon, &valid) && valid, "native relearner preserves checksum");
    uint16_t moves[4]{}; uint8_t pp[4]{}; remaster_emerald_box_pokemon_moves(&mon.box, moves, pp);
    Check(moves[0] && pp[0] == remaster_emerald_move_info(moves[0])->pp, "native relearner result and PP");
    Model stale; stale.Open(Screen::Relearner); Choose(stale, save, Command::SelectMove);
    stale.Build(&save, Free()); const auto revision = stale.frame.revision;
    Check(remaster_emerald_bag_remove(&save, 111, 1), "external removal of last scale"); const auto before = save;
    stale.Activate(0, revision, &save, Free());
    Check(std::memcmp(&save, &before, sizeof save) == 0, "stale relearner cannot consume or teach");
}
static void DialogueAndTiming() {
    auto save = Seed(); Model model;
    RemasterEmeraldScriptInstruction ins[4]{};
    ins[0].opcode = REMASTER_EMERALD_SCRIPT_CHOICE; ins[0].resource_id = "fixture_choices";
    ins[1].opcode = REMASTER_EMERALD_SCRIPT_SET_FLAG; ins[1].a = 0x100;
    ins[2].opcode = REMASTER_EMERALD_SCRIPT_PLAY_FANFARE;
    ins[3].opcode = REMASTER_EMERALD_SCRIPT_END;
    RemasterEmeraldScriptProgram program{}; program.script_id = "UiFixture"; program.instructions = ins; program.instruction_count = 4;
    RemasterEmeraldScriptRegistry registry{&program, 1}; RemasterEmeraldScriptRuntime runtime{};
    remaster_emerald_script_runtime_start(&runtime, &save, &registry, 0, 0);
    Check(remaster_emerald_script_runtime_run(&runtime, 20) == REMASTER_EMERALD_SCRIPT_YIELDED, "real VM yields choice");
    RemasterEmeraldScriptRequest request{};
    Check(remaster_emerald_script_runtime_pending_request(&runtime, &request), "pending core choice");
    Check(!model.PresentDialogue(request, "Ğabcde", {}), "unresolved choices blocked");
    Check(model.PresentDialogue(request, "Ğabcde", {{"Evet", Command::None, 7, true}, {"Hayır", Command::None, 9, true}}), "resolved typed choice hook");
    Check(!model.PresentDialogue(request, "other", {}), "same request cannot be repurposed");
    model.TextTick(); model.Build(&save, Free());
    Check(model.frame.body == "Ğabc", "four UTF-8 glyphs per tick");
    model.Input(Action::Cancel, &save, Free()); model.Build(&save, Free());
    Check(model.frame.screen == Screen::Dialogue, "cancel cannot escape pending core dialogue");
    const auto first = model.Activate(0, model.frame.revision, &save, Free());
    Check(first.effect == Effect::None, "first confirm reveals remaining text only");
    model.Build(&save, Free()); auto intent = model.Activate(0, model.frame.revision, &save, Free());
    auto forged = intent; forged.value = 8;
    Check(!model.CompleteDialogue(&runtime, forged), "undeclared choice value rejected");
    int flag = 0; Check(remaster_emerald_flag_get(&save, 0x100, &flag) && !flag, "text/timing did not advance VM");
    Check(model.CompleteDialogue(&runtime, intent), "matching sequence completes through native API");
    Check(!model.CompleteDialogue(&runtime, intent), "duplicate completion rejected");
    Check(remaster_emerald_flag_get(&save, 0x100, &flag) && !flag, "completion does not run next command");
    Check(remaster_emerald_script_runtime_run(&runtime, 20) == REMASTER_EMERALD_SCRIPT_YIELDED, "host resumes at fanfare");
    Check(remaster_emerald_script_runtime_pending_request(&runtime, &request), "fanfare pending");
    Check(!model.PresentDialogue(request, "", {}), "UI cannot claim audio barrier");
    Check(!model.CompleteDialogue(&runtime, intent), "old choice cannot complete fanfare");
    Repeat repeat; unsigned events = 0;
    for (unsigned tick = 1; tick <= 30; ++tick) {
        const bool emitted = repeat.Tick(true, true);
        Check(emitted == (tick == 24 || tick == 27 || tick == 30), "24/3 repeat schedule"); events += emitted;
    }
    Check(events == 3, "repeat count"); Check(!repeat.Tick(false, true), "release resets repeat");
    Check(!repeat.Tick(true, false), "disabled repeat reset");
}
static std::string Json(const std::string& input) {
    std::string output = "\"";
    for (const unsigned char c : input) {
        if (c == '"' || c == '\\') { output += '\\'; output += static_cast<char>(c); }
        else if (c == '\n') output += "\\n";
        else if (c < 32) output += " ";
        else output += static_cast<char>(c);
    }
    return output + "\"";
}
static void Fixtures() {
    auto save = Seed(); std::cout << "["; bool first = true;
    for (const auto screen : {Screen::Hud, Screen::Menu, Screen::Map, Screen::Quest, Screen::Party,
        Screen::Bag, Screen::Summary, Screen::Settings, Screen::Rtc, Screen::SaveLoad}) {
        Model model; model.Open(screen); model.Build(&save, Free());
        if (!first) std::cout << ",";
        first = false;
        std::cout << "{\"screen\":" << static_cast<int>(screen) << ",\"title\":" << Json(model.frame.title)
            << ",\"body\":" << Json(model.frame.body) << ",\"modal\":" << (model.frame.modal ? "true" : "false") << ",\"rows\":[";
        for (unsigned i = 0; i < model.frame.rows.size(); ++i) {
            if (i) std::cout << ",";
            std::cout << "{\"label\":" << Json(model.frame.rows[i].label) << ",\"enabled\":"
                << (model.frame.rows[i].enabled ? "true" : "false") << "}";
        }
        std::cout << "]}";
    }
    std::cout << "]\n";
}
int main(int argc, char** argv) {
    RemasterPlatformVTable platform{}; platform.read_wall_clock = Clock; remaster_platform_install(&platform);
    if (argc == 2 && std::strcmp(argv[1], "--fixtures") == 0) { Fixtures(); return 0; }
    ReadOnlyScreens(); NavigationAndTransactions(); Relearner(); DialogueAndTiming();
    std::printf("R9 portable UI: %u checks passed\n", checks); return 0;
}
