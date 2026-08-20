#include "path_utils.h"
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    std::filesystem::path executable_directory() {
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length == 0 || length == MAX_PATH) {
            return std::filesystem::current_path();
        }
        return std::filesystem::path(buffer).parent_path();
    }
}

std::string resolve_repo_path(const std::string& relative_path) {
    namespace fs = std::filesystem;

    if (fs::exists(relative_path)) {
        return relative_path;
    }

    fs::path dir = executable_directory();
    for (int level = 0; level < 8; ++level) {
        fs::path candidate = dir / relative_path;
        if (fs::exists(candidate)) {
            return candidate.string();
        }
        fs::path parent = dir.parent_path();
        if (parent == dir) {
            break;  // reached filesystem root
        }
        dir = parent;
    }

    return relative_path;
}
