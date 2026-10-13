#pragma once

extern "C" {
#include "remaster/emerald_save.h"
}
#include "RemasterFileSaveRead.h"
#include <cstring>
#include <filesystem>
#include <memory>
#include <vector>
#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#include <share.h>
#else
#include <unistd.h>
#endif

// External input is read-only. The caller supplies a fresh, private quarantine
// pathname. An existing pathname is never replaced, even if it names the source.
namespace RemasterSaveImport {
struct Result {
    RemasterEmeraldSaveStatus status = REMASTER_EMERALD_SAVE_IO_ERROR;
    bool copyCreated = false;
};
inline bool WriteExclusive(const std::filesystem::path& path,
    const uint8_t* data, size_t size) {
    int file = -1;
#if defined(_WIN32)
    if (_wsopen_s(&file,path.c_str(),_O_WRONLY|_O_BINARY|_O_CREAT|_O_EXCL,
            _SH_DENYRW,_S_IREAD|_S_IWRITE) != 0) return false;
#else
    file = ::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    if (file < 0) return false;
#endif
    size_t done = 0;
    bool success = true;
    while (done < size) {
#if defined(_WIN32)
        const int count = _write(file,data+done,static_cast<unsigned>(size-done));
#else
        const auto count = ::write(file,data+done,size-done);
#endif
        if (count <= 0) { success = false; break; }
        done += static_cast<size_t>(count);
    }
#if defined(_WIN32)
    if (_commit(file) != 0) success = false;
    if (_close(file) != 0) success = false;
#else
    if (::fsync(file) != 0) success = false;
    if (::close(file) != 0) success = false;
#endif
    if (!success) {
        // Only a file this call created exclusively can reach this cleanup.
        std::error_code error; std::filesystem::remove(path,error);
    }
    return success;
}
inline Result CopyFirst(const std::filesystem::path& source,
    const std::filesystem::path& newCopy, RemasterEmeraldSaveFormat format,
    RemasterEmeraldSave& output) {
    Result result;
    if (format != REMASTER_EMERALD_SAVE_FORMAT_STOCK
        && format != REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS) {
        result.status = REMASTER_EMERALD_SAVE_UNSUPPORTED;
        return result;
    }
    std::vector<uint8_t> bytes(REMASTER_EMERALD_SAVE_IMAGE_BYTES);
    size_t size = 0;
    const auto read = RemasterFileSaveRead::Read(source.c_str(),bytes.data(),bytes.size(),&size);
    if (read == REMASTER_SAVE_READ_MISSING) {
        result.status = REMASTER_EMERALD_SAVE_EMPTY; return result;
    }
    if (read != REMASTER_SAVE_READ_OK) return result;
    if (size != bytes.size()) {
        result.status = REMASTER_EMERALD_SAVE_UNSUPPORTED; return result;
    }
    if (!WriteExclusive(newCopy,bytes.data(),bytes.size())) return result;
    result.copyCreated = true;
    // Validation runs on the actual reopened copy, never the external source.
    std::vector<uint8_t> copied(bytes.size());
    if (RemasterFileSaveRead::Read(newCopy.c_str(),copied.data(),copied.size(),&size)
            != REMASTER_SAVE_READ_OK || size != copied.size() || copied != bytes)
        return result;
    result.status = remaster_emerald_save_import_image(copied.data(),copied.size(),format,&output);
    // Failed validation preserves both the original and its quarantined copy.
    return result;
}
}
