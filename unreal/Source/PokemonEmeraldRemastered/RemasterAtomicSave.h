#pragma once

#include <cstdio>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Platform-only file transport. The gameplay core supplies the full image.
namespace RemasterAtomicSave
{
inline bool Replace(const char* Temporary, const char* Destination)
{
    // POSIX rename in the same directory replaces without deleting the old
    // pathname first. Failure leaves the committed destination in place.
    return std::rename(Temporary, Destination) == 0;
}

#if defined(_WIN32)
inline bool Replace(const wchar_t* Temporary, const wchar_t* Destination)
{
    return ::MoveFileExW(Temporary, Destination,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}
#endif

template <typename WriteTemporary, typename ReplaceCommitted, typename DiscardTemporary>
bool Commit(WriteTemporary Write, ReplaceCommitted ReplaceCallback, DiscardTemporary Discard)
{
    if (!Write())
    {
        Discard();
        return false;
    }
    if (!ReplaceCallback())
    {
        Discard();
        return false;
    }
    return true;
}
}
