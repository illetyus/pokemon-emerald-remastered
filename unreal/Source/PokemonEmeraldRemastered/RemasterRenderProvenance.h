#pragma once
#include <string_view>

namespace RemasterRenderProvenance {
inline bool Hex(std::string_view value, unsigned length) {
    if (value.size() != length) return false;
    for (const unsigned char c : value)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            return false;
    return true;
}
// The pinned source repository uses Git SHA1 object IDs; payloads use SHA256.
inline bool GitCommit(std::string_view value) { return Hex(value, 40); }
inline bool Sha256(std::string_view value) { return Hex(value, 64); }
}
