#ifndef R17_ATOMIC_BASELINE
#include "RemasterAtomicSave.h"
#endif
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
static unsigned checks, failures;
static void check(bool ok, const char* what)
{
    ++checks;
    if (!ok) { ++failures; std::cerr << "atomic save: " << what << '\n'; }
}
static std::string read(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
static bool write(const fs::path& path, const std::string& value)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(value.data(), static_cast<std::streamsize>(value.size()));
    file.close();
    return !file.fail();
}
int main()
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() / ("r17-atomic-" + std::to_string(stamp));
    if (!fs::create_directory(root)) return 1;
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code error; fs::remove_all(path, error); } } cleanup{root};
    const fs::path destination = root / fs::u8path("save-\xC3\xA7.sav");
    const fs::path temporary = root / "same-directory.tmp";
    const std::string old_image(131072, 'O'), new_image(131072, 'N');
    for (int fault = 0; fault < 4; ++fault)
    {
        check(write(destination, old_image), "fixture existing image");
        unsigned writes = 0, replaces = 0, discards = 0;
        const auto write_temporary = [&]() {
            ++writes;
#ifdef R17_ATOMIC_BASELINE
            const fs::path& output = destination; // Existing Unreal writer truncates destination.
#else
            const fs::path& output = temporary;
#endif
            if (fault == 1) { (void)write(output, new_image.substr(0, 4096)); return false; }
            const bool ok = write(output, new_image);
            return ok && fault != 2; // Simulated failure reported after close.
        };
        const auto replace_committed = [&]() {
            ++replaces;
            check(read(destination) == old_image, "destination unchanged before replace");
            if (fault == 3) return false;
#ifndef R17_ATOMIC_BASELINE
            return RemasterAtomicSave::Replace(temporary.c_str(), destination.c_str());
#else
            return true;
#endif
        };
        const auto discard_temporary = [&]() { ++discards; std::error_code error; fs::remove(temporary, error); };
#ifdef R17_ATOMIC_BASELINE
        (void)replace_committed; (void)discard_temporary;
        const bool ok = write_temporary();
#else
        const bool ok = RemasterAtomicSave::Commit(write_temporary, replace_committed, discard_temporary);
#endif
        check(ok == (fault == 0), "failure propagates to caller");
        check(read(destination) == (fault == 0 ? new_image : old_image), "short write/close/replace failure preserves old image");
        check(!fs::exists(temporary), "temporary image is removed on failure or consumed on success");
        check(writes == 1 && replaces == (fault == 1 || fault == 2 ? 0u : 1u)
            && discards == (fault == 0 ? 0u : 1u), "commit protocol stops at failing stage");
    }
#ifndef R17_ATOMIC_BASELINE
    check(!RemasterAtomicSave::Replace(temporary.c_str(), destination.c_str())
        && read(destination) == old_image, "actual missing-source rename fails without changing destination");
#endif
    // A new destination also commits via rename, without requiring an old file.
    fs::remove(destination);
#ifndef R17_ATOMIC_BASELINE
    check(write(temporary, new_image) && RemasterAtomicSave::Replace(temporary.c_str(), destination.c_str())
        && read(destination) == new_image && !fs::exists(temporary), "first image creation atomic rename");
#endif
    std::cout << "R17 atomic file save: " << checks << " checks, " << failures << " failures.\n";
    return failures ? 1 : 0;
}
