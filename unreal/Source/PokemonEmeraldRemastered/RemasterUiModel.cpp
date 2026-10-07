#include "RemasterUiModel.h"
#include <memory>
#include <sstream>

namespace RemasterUi {
void Model::Build(const RemasterEmeraldSave* save, const Context& context) {
    const auto fingerprint = Fingerprint(save);
    if (fingerprint != coreFingerprint) { coreFingerprint = fingerprint; ++frame.revision; }
    const auto newContextKey = context.mapName + "/" + std::to_string(context.mapGroup)
        + "/" + std::to_string(context.mapNum) + "/" + std::to_string(context.mapReady)
        + std::to_string(context.scriptBusy) + std::to_string(context.battleBusy);
    if (newContextKey != contextKey) { contextKey = newContextKey; ++frame.revision; }
    frame.screen = screen; frame.modal = Modal(); frame.rows.clear(); frame.body.clear(); frame.hasMarker = false;
    const bool usable = Usable(save), canAct = CanAct(save, context);
    const auto add = [this](std::string label, Command command = Command::None,
                           int value = 0, bool enabled = true) {
        frame.rows.push_back({std::move(label), command, value, enabled});
    };
    const auto open = [&add](const char* label, Screen target, bool enabled = true) {
        add(label, Command::Open, static_cast<int>(target), enabled);
    };
    const auto* objective = usable ? remaster_emerald_quest_active(save) : nullptr;
    switch (screen) {
    case Screen::Hud: {
        frame.title = context.mapName.empty() ? "Pokémon Emerald" : context.mapName;
        RemasterEmeraldOverworldState world{};
        if (usable && remaster_emerald_overworld_get(save, &world)) {
            frame.body = "₽ " + std::to_string(world.money) + " · Takım "
                + std::to_string(remaster_emerald_party_count(save));
            if (objective) frame.body += "\n" + std::string(objective->title);
        } else frame.body = "Kayıt yüklenmedi.";
        open("Menü", Screen::Menu, !context.scriptBusy && !context.battleBusy);
        open("Harita", Screen::Map, canAct);
        open("Görev", Screen::Quest, canAct);
        open("Kayıt / Yükle", Screen::SaveLoad, !context.scriptBusy && !context.battleBusy);
        break;
    }
    case Screen::Menu:
        frame.title = "Menü";
        open("Takım", Screen::Party, usable); open("Çanta", Screen::Bag, usable);
        open("Harita", Screen::Map, usable); open("Görev", Screen::Quest, usable);
        open("Ayarlar", Screen::Settings); open("Kayıt / Yükle", Screen::SaveLoad);
        break;
    case Screen::Map:
    case Screen::Quest:
        frame.title = screen == Screen::Map ? "Harita" : "Görev";
        frame.body = "Konum: " + context.mapName;
        if (objective) {
            frame.body += "\n" + std::string(objective->title) + "\n" + objective->description;
            const auto& marker = objective->region_marker;
            frame.hasMarker = marker.width > 0 && marker.height > 0;
            frame.markerX = marker.x; frame.markerY = marker.y;
            frame.markerWidth = marker.width; frame.markerHeight = marker.height;
            frame.body += "\nHedef bölge: " + Name(RegionName(marker.map_section_id), marker.map_section_id)
                + " · " + std::to_string(marker.x) + "," + std::to_string(marker.y)
                + " · " + std::to_string(marker.width) + "×" + std::to_string(marker.height);
            if (context.mapGroup >= 0 && context.mapNum >= 0
                && remaster_emerald_quest_target_matches_map(objective,
                    static_cast<uint8_t>(context.mapGroup), static_cast<uint8_t>(context.mapNum)))
                frame.body += "\nHedef bulunduğun haritada.";
        } else frame.body += "\nEtkin ana görev yok.";
        open(screen == Screen::Map ? "Görev ayrıntısı" : "Haritada göster",
             screen == Screen::Map ? Screen::Quest : Screen::Map, objective != nullptr);
        break;
    case Screen::Party:
        frame.title = "Takım";
        if (usable) for (uint8_t slot = 0; slot < remaster_emerald_party_count(save); ++slot) {
            RemasterEmeraldPartyPokemon mon{}; int valid = 0;
            const bool ok = remaster_emerald_party_get(save, slot, &mon, &valid) && valid;
            const auto species = remaster_emerald_box_pokemon_species(&mon.box);
            add(ok ? Nickname(mon.box) + " · " + Name(SpeciesName(species), species)
                + " Sv " + std::to_string(mon.level) + " · HP " + std::to_string(mon.hp)
                + "/" + std::to_string(mon.max_hp) : "Okunamayan Pokémon",
                Command::SelectParty, slot, ok);
        }
        if (frame.rows.empty()) frame.body = "Takım boş.";
        break;
    case Screen::Summary:
    case Screen::Nickname:
    case Screen::Relearner:
    case Screen::ReplaceMove: {
        frame.title = screen == Screen::Summary ? "Özet" : screen == Screen::Nickname ? "Takma ad"
            : screen == Screen::Relearner ? "Hareket hatırlatıcı" : "Değiştirilecek hareket";
        RemasterEmeraldPartyPokemon mon{}; int valid = 0;
        if (!usable || partySlot >= remaster_emerald_party_count(save)
            || !remaster_emerald_party_get(save, partySlot, &mon, &valid) || !valid) {
            frame.body = "Pokémon verisi okunamıyor."; break;
        }
        const auto species = remaster_emerald_box_pokemon_species(&mon.box);
        uint8_t evs[6]{}, ivs[6]{}, pp[4]{}; uint16_t moves[4]{};
        remaster_emerald_box_pokemon_evs(&mon.box, evs);
        remaster_emerald_box_pokemon_ivs(&mon.box, ivs);
        remaster_emerald_box_pokemon_moves(&mon.box, moves, pp);
        frame.body = Nickname(mon.box) + " · " + Name(SpeciesName(species), species)
            + "\nSv " + std::to_string(mon.level) + " · HP " + std::to_string(mon.hp)
            + "/" + std::to_string(mon.max_hp);
        if (screen == Screen::Summary) {
            const char* stats[] = {"HP", "Saldırı", "Savunma", "Hız", "Öz. Saldırı", "Öz. Savunma"};
            const uint16_t current[] = {mon.max_hp, mon.attack, mon.defense, mon.speed, mon.sp_attack, mon.sp_defense};
            for (unsigned i = 0; i < 6; ++i) frame.body += "\n" + std::string(stats[i])
                + " " + std::to_string(current[i]) + " · IV " + std::to_string(ivs[i])
                + " · EV " + std::to_string(evs[i]);
            frame.body += "\nToplam EV " + std::to_string(remaster_emerald_qol_total_evs(&mon.box))
                + " · Gizli Güç türü " + std::to_string(remaster_emerald_qol_hidden_power_type(&mon.box));
            for (unsigned i = 0; i < 4; ++i) if (moves[i])
                frame.body += "\n" + Name(MoveName(moves[i]), moves[i]) + " · PP " + std::to_string(pp[i]);
            open("Takma ad", Screen::Nickname,
                canAct && remaster_emerald_qol_nickname_owned_by_player(save, &mon.box));
            open("Hareket hatırlatıcı", Screen::Relearner, canAct
                && remaster_emerald_qol_move_relearner_status(save, partySlot) == REMASTER_EMERALD_QOL_MOVE_RELEARNER_OK);
        } else if (screen == Screen::Nickname) {
            frame.body += "\nYeni adı girmek için aç.";
            add("Ad girişi", Command::Nickname, partySlot, canAct
                && remaster_emerald_qol_nickname_owned_by_player(save, &mon.box));
        } else if (screen == Screen::Relearner) {
            uint16_t candidates[512]{};
            const auto count = remaster_emerald_qol_move_relearner_candidates(save, partySlot, candidates, 512);
            for (std::size_t i = 0; i < std::min<std::size_t>(count, 512); ++i)
                add(Name(MoveName(candidates[i]), candidates[i]), Command::SelectMove, candidates[i], canAct);
            frame.body += "\nBaşarılı işlemde 1 Kalp Pulu kullanılır.";
        } else for (unsigned i = 0; i < 4; ++i)
            add(moves[i] ? Name(MoveName(moves[i]), moves[i]) : "Boş hareket yuvası",
                Command::ReplaceMove, static_cast<int>(i), canAct);
        break;
    }
    case Screen::Bag:
        frame.title = "Çanta";
        if (usable) {
            // Metadata getters initialize their input. Use an isolated copy for rendering.
            auto view = std::make_unique<RemasterEmeraldSave>(*save);
            frame.body = "Cep " + std::to_string(pocket) + " · Sıralama "
                + std::to_string(remaster_emerald_qol_bag_sort_mode(view.get(), pocket));
            add("Önceki cep", Command::SelectItem, -1);
            add("Sonraki cep", Command::SelectItem, -2);
            for (int mode = 1; mode <= 4; ++mode) {
                const char* labels[] = {"", "Ada göre sırala", "Türe göre sırala", "Adede göre sırala", "Değere göre sırala"};
                add(labels[mode], Command::Sort, mode, canAct);
            }
            add(remaster_emerald_qol_bag_auto_sort_enabled(view.get(), pocket)
                ? "Otomatik sıralama: açık" : "Otomatik sıralama: kapalı", Command::AutoSort, 0, canAct);
            for (std::size_t i = 0; i < remaster_emerald_bag_pocket_capacity(pocket); ++i) {
                RemasterEmeraldItemSlot item{};
                if (remaster_emerald_bag_slot_get(save, pocket, i, &item) && item.item_id && item.quantity)
                    add(Name(ItemName(item.item_id), item.item_id) + " × " + std::to_string(item.quantity),
                        Command::SelectItem, item.item_id);
            }
        } else frame.body = "Çanta okunamıyor.";
        break;
    case Screen::Item:
        frame.title = Name(ItemName(itemId), itemId);
        frame.body = "Adet: " + std::to_string(usable ? remaster_emerald_bag_count(save, itemId) : 0);
        if (const auto* info = remaster_emerald_item_info(itemId))
            frame.body += "\nDeğer: " + std::to_string(info->price);
        add("Hızlı eşyaya kaydet", Command::RegisterItem, itemId, canAct);
        add("Hızlı eşyadan çıkar", Command::UnregisterItem, itemId, canAct);
        // Field-use belongs to the script/domain host; never guess an item effect.
        add("Kullan", Command::FieldItem, itemId, canAct);
        break;
    case Screen::QuickItems:
    case Screen::Repel:
        frame.title = screen == Screen::Repel ? "Kovucu etkisi bitti" : "Hızlı eşya";
        if (usable) {
            if (screen == Screen::Repel) {
                for (const uint16_t id : {uint16_t(86), uint16_t(83), uint16_t(84)})
                    if (remaster_emerald_bag_count(save, id))
                        add(Name(ItemName(id), id), Command::FieldItem, id, canAct);
            } else {
                auto view = std::make_unique<RemasterEmeraldSave>(*save);
                for (uint8_t i = 0; i < remaster_emerald_qol_quick_item_count(view.get()); ++i) {
                    const auto id = remaster_emerald_qol_quick_item_get(view.get(), i);
                    add(Name(ItemName(id), id), Command::FieldItem, id,
                        canAct && remaster_emerald_bag_count(save, id) != 0);
                }
            }
        }
        if (frame.rows.empty()) frame.body = "Kullanılabilir eşya yok.";
        break;
    case Screen::Settings:
        frame.title = "Ayarlar";
        add(fastText ? "Metin hızı: hızlı" : "Metin hızı: normal", Command::ToggleText);
        add(repeatEnabled ? "Menü tuş tekrarı: açık" : "Menü tuş tekrarı: kapalı", Command::ToggleRepeat);
        open("Saat ayarı", Screen::Rtc, canAct);
        break;
    case Screen::Rtc:
        frame.title = "Saat ayarı";
        if (!rtcInitialized && usable) rtcInitialized = remaster_emerald_rtc_local_now(save, &rtcDraft) != 0;
        if (!rtcInitialized) frame.body = "Saat bilgisi alınamıyor.";
        else {
            frame.body = "Gün " + std::to_string(rtcDraft.days) + " · " + std::to_string(rtcDraft.hours)
                + ":" + std::to_string(rtcDraft.minutes) + "\nUygulamak oyun saatinin farkını değiştirir.";
            add("Gün +1", Command::RtcDays, 1); add("Gün −1", Command::RtcDays, -1);
            add("Saat +1", Command::RtcHours, 1); add("Saat −1", Command::RtcHours, -1);
            add("Dakika +1", Command::RtcMinutes, 1); add("Dakika −1", Command::RtcMinutes, -1);
            add("Saat ayarını uygula", Command::ApplyRtc, 0, canAct);
        }
        break;
    case Screen::SaveLoad:
        frame.title = "Kayıt / Yükle";
        frame.body = usable ? (save->status == REMASTER_EMERALD_SAVE_DEGRADED
            ? "Kayıt kurtarılan yuvadan yüklendi." : "Kayıt kullanılabilir.") : "Kullanılabilir kayıt yok.";
        open("Kaydet", Screen::ConfirmSave, canAct);
        open("Yükle", Screen::ConfirmLoad, !context.scriptBusy && !context.battleBusy);
        break;
    case Screen::ConfirmSave:
    case Screen::ConfirmLoad:
        frame.title = screen == Screen::ConfirmSave ? "Kaydedilsin mi?" : "Kayıt yüklensin mi?";
        frame.body = screen == Screen::ConfirmSave ? "Mevcut ilerleme kayıt dosyasına yazılır."
            : "Kaydedilmemiş ilerleme kaybolur.";
        add("Onayla", screen == Screen::ConfirmSave ? Command::Save : Command::Load, 0,
            screen == Screen::ConfirmSave ? canAct : !context.scriptBusy && !context.battleBusy);
        break;
    case Screen::Dialogue:
        frame.title = "Diyalog";
        frame.body = dialogueText.substr(0, visibleGlyphs);
        if (dialogueRequest.type == REMASTER_EMERALD_SCRIPT_REQUEST_CHOICE) frame.rows = dialogueChoices;
        else add("Devam", Command::DialogueComplete);
        break;
    }
    if (!notice.empty()) frame.body += "\n" + notice;
    if (selected >= frame.rows.size()) selected = 0;
    frame.selected = selected;
}

Intent Model::Activate(unsigned row, std::uint64_t revision,
                       RemasterEmeraldSave* save, const Context& context) {
    Intent intent{};
    if (revision != frame.revision || row >= frame.rows.size()
        || !frame.rows[row].enabled || Fingerprint(save) != coreFingerprint) return intent;
    const Row chosen = frame.rows[row];
    intent.revision = revision; intent.fingerprint = coreFingerprint;
    intent.value = chosen.value;
    const bool canAct = CanAct(save, context);
    switch (chosen.command) {
    case Command::Open:
        if (context.scriptBusy || context.battleBusy) break;
        if (chosen.value == static_cast<int>(Screen::Rtc)) rtcInitialized = false;
        Open(static_cast<Screen>(chosen.value)); break;
    case Command::SelectParty:
        partySlot = static_cast<uint8_t>(chosen.value); Open(Screen::Summary); break;
    case Command::SelectItem:
        if (chosen.value < 0) { pocket = static_cast<uint8_t>((pocket + (chosen.value == -1 ? 3 : 5)) % 5 + 1); ++frame.revision; }
        else { itemId = static_cast<uint16_t>(chosen.value); Open(Screen::Item); }
        break;
    case Command::SelectMove:
        moveId = static_cast<uint16_t>(chosen.value); Open(Screen::ReplaceMove); break;
    case Command::ToggleText: fastText = !fastText; ++frame.revision; break;
    case Command::ToggleRepeat: repeatEnabled = !repeatEnabled; ++frame.revision; break;
    case Command::RtcDays:
        rtcDraft.days = static_cast<int16_t>(std::clamp<int>(rtcDraft.days + chosen.value, 0, 32767)); ++frame.revision; break;
    case Command::RtcHours:
        rtcDraft.hours = static_cast<int8_t>((rtcDraft.hours + chosen.value + 24) % 24); ++frame.revision; break;
    case Command::RtcMinutes:
        rtcDraft.minutes = static_cast<int8_t>((rtcDraft.minutes + chosen.value + 60) % 60); ++frame.revision; break;
    case Command::Save: if (canAct) intent.effect = Effect::Save; break;
    case Command::Load: if (!context.scriptBusy && !context.battleBusy) intent.effect = Effect::Load; break;
    case Command::ApplyRtc:
        if (canAct && rtcInitialized) { intent.effect = Effect::ApplyRtc; intent.time = rtcDraft; } break;
    case Command::Nickname: if (canAct) intent.effect = Effect::Nickname; break;
    case Command::FieldItem:
        if (canAct && remaster_emerald_bag_count(save, static_cast<uint16_t>(chosen.value))) intent.effect = Effect::FieldItem;
        break;
    case Command::DialogueComplete:
        if (!dialogueReady) break;
        if (visibleGlyphs < dialogueText.size()) { visibleGlyphs = dialogueText.size(); ++frame.revision; }
        else intent.effect = Effect::DialogueComplete;
        break;
    case Command::Sort:
    case Command::AutoSort:
    case Command::RegisterItem:
    case Command::UnregisterItem:
    case Command::ReplaceMove: {
        if (!canAct) break;
        // Stage native commands so rejected actions cannot leave partial metadata writes.
        auto staged = std::make_unique<RemasterEmeraldSave>(*save);
        bool ok = false;
        if (chosen.command == Command::Sort)
            ok = remaster_emerald_qol_bag_sort(staged.get(), pocket,
                static_cast<RemasterEmeraldQolItemSortMode>(chosen.value)) != 0;
        else if (chosen.command == Command::AutoSort)
            ok = remaster_emerald_qol_bag_set_auto_sort_enabled(staged.get(), pocket,
                !remaster_emerald_qol_bag_auto_sort_enabled(staged.get(), pocket)) != 0;
        else if (chosen.command == Command::RegisterItem)
            ok = remaster_emerald_qol_quick_item_register(staged.get(), itemId) != 0;
        else if (chosen.command == Command::UnregisterItem)
            ok = remaster_emerald_qol_quick_item_unregister(staged.get(), itemId) != 0;
        else ok = remaster_emerald_qol_move_relearner_learn(staged.get(), partySlot, moveId,
                     static_cast<uint8_t>(chosen.value)) != 0;
        if (ok) { *save = *staged; notice = "İşlem tamamlandı."; }
        else notice = "İşlem uygulanamadı.";
        ++frame.revision;
        if (ok && chosen.command == Command::ReplaceMove) { Back(); Back(); }
        break;
    }
    default: break;
    }
    return intent;
}
}
