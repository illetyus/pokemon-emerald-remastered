#include "remaster/emerald_save.h"
#include "remaster/emerald_state.h"
#include "remaster/emerald_quest.h"
#include "remaster/emerald_pokemon.h"
#include "remaster/emerald_items.h"
#include "remaster/platform.h"
#include "RemasterFileSaveRead.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>

// Test-only adapter. Input is read-only; persisted output is returned to the
// driver, which owns a temporary directory. No input path is ever written.
struct Transport
{
    std::vector<uint8_t> Disk;
    size_t Size = 0;
    RemasterSaveReadResult Result = REMASTER_SAVE_READ_ERROR;
    unsigned Writes = 0;
    bool FailWrite = false;
};
static RemasterSaveReadResult Read(void* User, const char*, uint8_t* Buffer,
    size_t Capacity, size_t* Size)
{
    auto& T = *static_cast<Transport*>(User);
    *Size = T.Size;
    if (T.Result == REMASTER_SAVE_READ_OK && T.Size <= Capacity)
        std::memcpy(Buffer, T.Disk.data(), T.Size);
    return T.Result;
}
static int Write(void* User, const char*, const uint8_t* Buffer, size_t Size)
{
    auto& T = *static_cast<Transport*>(User);
    ++T.Writes;
    if (T.FailWrite) return 0;
    T.Disk.assign(Buffer, Buffer + Size);
    T.Size = Size;
    return 1;
}
static void Hex(const uint8_t* Bytes, size_t Size)
{
    const char* Digits = "0123456789abcdef";
    std::cout << '"';
    for (size_t I = 0; I < Size; ++I)
        std::cout << Digits[Bytes[I] >> 4] << Digits[Bytes[I] & 15];
    std::cout << '"';
}
static bool Snapshot(const RemasterEmeraldSave& Save)
{
    RemasterEmeraldOverworldState World{};
    if (!remaster_emerald_overworld_get(&Save, &World)) return false;
    const auto* Quest = remaster_emerald_quest_active(&Save);
    const auto Offset = remaster_emerald_save_get_local_time_offset(&Save);
    const auto Berry = remaster_emerald_save_get_last_berry_update(&Save);
    std::cout << "{\"status\":" << Save.status << ",\"counter\":" << Save.counter
        << ",\"slot\":" << unsigned(Save.selected_slot)
        << ",\"rotation\":" << Save.last_written_sector
        << ",\"stock\":" << unsigned(Save.source_is_stock)
        << ",\"objective\":" << (Quest ? Quest->id : 0)
        << ",\"world\":[" << World.player_x << ',' << World.player_y
        << ',' << int(World.map_group) << ',' << int(World.map_num)
        << ',' << int(World.warp_id) << ',' << World.warp_x << ',' << World.warp_y
        << ',' << World.map_layout_id << ',' << World.saved_music
        << ',' << unsigned(World.weather) << ',' << unsigned(World.weather_cycle_stage)
        << ',' << unsigned(World.flash_level) << ',' << unsigned(World.party_count)
        << ',' << World.money << ',' << World.coins << ',' << World.registered_item
        << "],\"rtc\":[" << Offset.days << ',' << int(Offset.hours)
        << ',' << int(Offset.minutes) << ',' << int(Offset.seconds)
        << ',' << Berry.days << ',' << int(Berry.hours) << ',' << int(Berry.minutes)
        << ',' << int(Berry.seconds) << "],\"party\":[";
    const size_t Count = remaster_emerald_party_count(&Save);
    for (size_t I = 0; I < Count; ++I)
    {
        RemasterEmeraldPartyPokemon Mon{};
        int Valid = 0;
        if (!remaster_emerald_party_get(&Save, I, &Mon, &Valid)) return false;
        if (I) std::cout << ',';
        std::cout << '[' << Valid << ',' << remaster_emerald_box_pokemon_species(&Mon.box)
            << ',' << remaster_emerald_box_pokemon_held_item(&Mon.box)
            << ',' << remaster_emerald_box_pokemon_experience(&Mon.box)
            << ',' << unsigned(Mon.level) << ',' << Mon.hp << ',' << Mon.max_hp
            << ',' << Mon.status << ']';
    }
    std::cout << "],\"bag\":[";
    for (uint8_t Pocket = 1; Pocket <= 5; ++Pocket)
    {
        if (Pocket > 1) std::cout << ',';
        std::cout << '[';
        for (size_t I = 0; I < remaster_emerald_bag_pocket_capacity(Pocket); ++I)
        {
            RemasterEmeraldItemSlot Item{};
            if (!remaster_emerald_bag_slot_get(&Save, Pocket, I, &Item)) return false;
            if (I) std::cout << ',';
            std::cout << '[' << Item.item_id << ',' << Item.quantity << ']';
        }
        std::cout << ']';
    }
    std::cout << "],\"current_box\":" << unsigned(remaster_emerald_storage_current_box(&Save))
        << ",\"blocks\":[";
    Hex(Save.save_block2, Save.source_is_stock ? 0xF2C : 0xF44);
    std::cout << ',';
    Hex(Save.save_block1, Save.source_is_stock ? 0x3D88 : 0x3DC8);
    std::cout << ',';
    Hex(Save.pokemon_storage, 0x83D0);
    std::cout << "]}";
    return true;
}
int main(int Argc, char** Argv)
{
    if (Argc < 3 || Argc > 4 || (std::strcmp(Argv[1], "stock") != 0
        && std::strcmp(Argv[1], "vanillaplus") != 0)
        || (Argc == 4 && std::strcmp(Argv[3], "fail-write") != 0))
    {
        std::cerr << "usage: probe stock|vanillaplus input.sav [fail-write]\n";
        return 2;
    }
    const auto Format = std::strcmp(Argv[1], "stock") == 0
        ? REMASTER_EMERALD_SAVE_FORMAT_STOCK : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
    Transport T;
    T.Disk.resize(131072);
    T.FailWrite = Argc == 4;
    const auto Path = std::filesystem::u8path(Argv[2]);
    T.Result = RemasterFileSaveRead::Read(Path.c_str(), T.Disk.data(), T.Disk.size(), &T.Size);
    RemasterPlatformVTable Platform{};
    Platform.userdata = &T;
    Platform.save_read_result = Read;
    Platform.save_write = Write;
    remaster_platform_install(&Platform);
    std::vector<uint8_t> Scratch(131072);
    RemasterEmeraldSave Save{}, Reloaded{};
    const auto Status = remaster_emerald_save_load_platform_format(
        "fixture", Scratch.data(), Scratch.size(), Format, &Save);
    std::cout << "{\"status\":" << Status;
    if (Status != REMASTER_EMERALD_SAVE_OK && Status != REMASTER_EMERALD_SAVE_DEGRADED)
    {
        std::cout << ",\"writes\":0}\n";
        return 0;
    }
    std::cout << ",\"before\":";
    if (!Snapshot(Save)) return 1;
    RemasterEmeraldSave Before;
    std::memcpy(&Before, &Save, sizeof(Save));
    const bool Stored = remaster_emerald_save_store_platform(
        "fixture", Scratch.data(), Scratch.size(), &Save) != 0;
    std::cout << ",\"store_ok\":" << (Stored ? "true" : "false")
        << ",\"writes\":" << T.Writes
        << ",\"wrapper_unchanged\":"
        << (std::memcmp(&Before, &Save, sizeof(Save)) == 0 ? "true" : "false");
    if (Stored)
    {
        const auto Loaded = remaster_emerald_save_load_platform_format(
            "fixture", Scratch.data(), Scratch.size(), Format, &Reloaded);
        if (Loaded != REMASTER_EMERALD_SAVE_OK) return 1;
        std::cout << ",\"after\":";
        if (!Snapshot(Reloaded)) return 1;
    }
    std::cout << ",\"image\":";
    Hex(T.Disk.data(), T.Disk.size());
    std::cout << "}\n";
    return 0;
}
