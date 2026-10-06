#include "RemasterFileSaveRead.h"

#include <chrono>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static unsigned checks, failures;
static void Expect(bool Ok, const char* Message)
{
    ++checks;
    if (!Ok) { ++failures; std::cerr << Message << '\n'; }
}
static RemasterSaveReadResult Read(const std::filesystem::path& Path,
    uint8_t* Buffer, size_t Capacity, size_t* Size)
{ return RemasterFileSaveRead::Read(Path.c_str(), Buffer, Capacity, Size); }
static void Write(const std::filesystem::path& Path, size_t Size)
{
    std::ofstream File(Path, std::ios::binary | std::ios::trunc);
    std::vector<char> Data(Size, 'S');
    File.write(Data.data(), static_cast<std::streamsize>(Size)); File.close();
    Expect(!File.fail(), "native fixture written and closed");
}
int main()
{
    const auto Token = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto Root = std::filesystem::temp_directory_path() / ("r17-read-" + std::to_string(Token));
    std::filesystem::create_directories(Root);
    const auto File = Root / std::filesystem::u8path("save-\xC3\xA7.sav");
    std::vector<uint8_t> Buffer(131074, 0xA5); size_t Size = 77;
    Expect(Read(File, Buffer.data() + 1, 131072, &Size) == REMASTER_SAVE_READ_MISSING && Size == 0,
        "only absent file is MISSING with initialized size");
    Expect(Read(Root / "absent-parent" / "save.sav", Buffer.data(), Buffer.size(), &Size)
        == REMASTER_SAVE_READ_MISSING && Size == 0, "confirmed absent parent permits new-save path");
    Expect(RemasterFileSaveRead::OpenFailure(File.c_str(), EACCES) == REMASTER_SAVE_READ_ERROR,
        "permission error cannot authorize MISSING");
    Expect(RemasterFileSaveRead::OpenFailure(File.c_str(), EIO) == REMASTER_SAVE_READ_ERROR,
        "storage error cannot authorize MISSING");
    Write(File, 131072);
    Expect(Read(File, Buffer.data() + 1, 131072, &Size) == REMASTER_SAVE_READ_OK && Size == 131072,
        "complete native Unicode-path read succeeds");
    bool Contents = true;
    for (size_t I = 1; I <= 131072; ++I) Contents = Contents && Buffer[I] == 'S';
    Expect(Contents && Buffer.front() == 0xA5 && Buffer.back() == 0xA5, "full payload and capacity canaries");
    for (size_t Length : {size_t(0), size_t(1), size_t(131071), size_t(131073), size_t(262144)})
    {
        Write(File, Length); std::fill(Buffer.begin(), Buffer.end(), uint8_t(0xA5));
        Expect(Read(File, Buffer.data() + 1, 131072, &Size) == REMASTER_SAVE_READ_OK && Size == Length,
            "found wrong-size file reports actual size, not MISSING");
        Expect(Buffer.front() == 0xA5 && Buffer.back() == 0xA5, "wrong-size reads stay within capacity");
        if (Length > 131072)
            Expect(Buffer[1] == 0xA5 && Buffer[131072] == 0xA5, "oversize probe copies no bytes");
    }
    Size = 77;
    Expect(Read(Root, Buffer.data(), Buffer.size(), &Size) == REMASTER_SAVE_READ_ERROR && Size == 0,
        "directory open/read failure is ERROR, not MISSING");
    Expect(Read(Root, nullptr, 0, &Size) == REMASTER_SAVE_READ_ERROR && Size == 0,
        "directory cannot pass the zero-capacity oversize probe");
    Expect(Read(File / "child", Buffer.data(), Buffer.size(), &Size) == REMASTER_SAVE_READ_ERROR,
        "non-directory path component is an error");
    Expect(RemasterFileSaveRead::Read(static_cast<const char*>(nullptr), Buffer.data(), Buffer.size(), &Size)
        == REMASTER_SAVE_READ_ERROR && Size == 0, "invalid path rejected");
    Expect(Read(File, nullptr, 131072, &Size) == REMASTER_SAVE_READ_ERROR && Size == 0, "null read buffer rejected");
    Expect(Read(File, Buffer.data(), Buffer.size(), nullptr) == REMASTER_SAVE_READ_ERROR, "null size rejected");
    Write(File, 131072);
    Expect(Read(File, nullptr, 0, &Size) == REMASTER_SAVE_READ_OK && Size == 131072, "zero-capacity size probe is bounded");
    std::filesystem::remove_all(Root);
    std::cout << "R17 native file read: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
