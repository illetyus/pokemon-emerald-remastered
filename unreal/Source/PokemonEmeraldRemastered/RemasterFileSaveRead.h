#pragma once

#include "remaster/platform.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <system_error>

/* Shared native read transport; no UE dependency or gameplay interpretation.
 * Report MISSING only for ENOENT. Never allocate an unbounded file-sized array. */
namespace RemasterFileSaveRead
{
template<typename Character>
inline RemasterSaveReadResult OpenFailure(const Character* Path, int OpenError)
{
    if (OpenError != ENOENT)
        return REMASTER_SAVE_READ_ERROR;
    // Some CRTs collapse a non-directory parent into ENOENT. Confirm that the
    // target is absent and its nearest existing ancestor is a directory.
    std::error_code Error;
    std::filesystem::path Candidate(Path);
    auto Status = std::filesystem::status(Candidate, Error);
    if (Status.type() != std::filesystem::file_type::not_found
        || (Error && Error != std::errc::no_such_file_or_directory))
        return REMASTER_SAVE_READ_ERROR;
    Candidate = Candidate.parent_path();
    if (Candidate.empty()) Candidate = ".";
    for (;;)
    {
        Error.clear();
        Status = std::filesystem::status(Candidate, Error);
        if (Error && Error != std::errc::no_such_file_or_directory)
            return REMASTER_SAVE_READ_ERROR;
        if (Status.type() != std::filesystem::file_type::not_found)
            return std::filesystem::is_directory(Status)
                ? REMASTER_SAVE_READ_MISSING : REMASTER_SAVE_READ_ERROR;
        auto Parent = Candidate.parent_path();
        if (Parent.empty()) Parent = ".";
        if (Parent == Candidate) return REMASTER_SAVE_READ_ERROR;
        Candidate = Parent;
    }
}

inline RemasterSaveReadResult ReadOpened(
    std::FILE* File, uint8_t* Buffer, size_t Capacity, size_t* OutSize)
{
    *OutSize = 0;
    if (std::fseek(File, 0, SEEK_END) != 0)
    {
        std::fclose(File);
        return REMASTER_SAVE_READ_ERROR;
    }
    const long Length = std::ftell(File);
    if (Length < 0)
    {
        std::fclose(File);
        return REMASTER_SAVE_READ_ERROR;
    }
    const size_t Size = static_cast<size_t>(Length);
    if (Size <= Capacity)
    {
        if (std::fseek(File, 0, SEEK_SET) != 0
            || (Size > 0 && std::fread(Buffer, 1, Size, File) != Size)
            || std::fgetc(File) != EOF || std::ferror(File))
        {
            std::fclose(File);
            return REMASTER_SAVE_READ_ERROR;
        }
    }
    if (std::fclose(File) != 0)
        return REMASTER_SAVE_READ_ERROR;
    *OutSize = Size;
    return REMASTER_SAVE_READ_OK;
}

inline RemasterSaveReadResult Read(
    const char* Path, uint8_t* Buffer, size_t Capacity, size_t* OutSize)
{
    if (OutSize) *OutSize = 0;
    if (!Path || !*Path || !OutSize || (!Buffer && Capacity > 0))
        return REMASTER_SAVE_READ_ERROR;
    std::FILE* File = nullptr;
    int OpenError;
#if defined(_MSC_VER)
    OpenError = fopen_s(&File, Path, "rb");
#else
    File = std::fopen(Path, "rb");
    OpenError = File ? 0 : errno;
#endif
    if (!File)
        return OpenFailure(Path, OpenError);
    return ReadOpened(File, Buffer, Capacity, OutSize);
}

#if defined(_WIN32)
inline RemasterSaveReadResult Read(
    const wchar_t* Path, uint8_t* Buffer, size_t Capacity, size_t* OutSize)
{
    if (OutSize) *OutSize = 0;
    if (!Path || !*Path || !OutSize || (!Buffer && Capacity > 0))
        return REMASTER_SAVE_READ_ERROR;
    std::FILE* File = nullptr;
    const int OpenError = _wfopen_s(&File, Path, L"rb");
    if (!File)
        return OpenFailure(Path, OpenError);
    return ReadOpened(File, Buffer, Capacity, OutSize);
}
#endif
}
