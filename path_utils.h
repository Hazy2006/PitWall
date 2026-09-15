#pragma once
#include <string>

// Resolves relative_path against disk regardless of the process's working
// directory: tries it as-is, then walks up from the exe's own directory
// (covers running the .exe outside the repo root, e.g. F5 in Visual Studio).
std::string resolve_repo_path(const std::string& relative_path);
