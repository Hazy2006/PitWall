#pragma once
#include <string>

// Resolves relative_path (e.g. "data" or "data/results.json") to a path
// that actually exists on disk, independent of the process's current
// working directory.
//
// Tries relative_path as-is against the working directory first, so
// running from the repo root keeps working exactly as before. If that
// doesn't exist, walks upward from the executable's own directory (found
// via the Windows GetModuleFileNameW API) looking for an ancestor under
// which relative_path exists -- this covers launching the exe directly
// from its build-output folder (e.g. F5 in Visual Studio, or double-
// clicking the .exe). Falls back to relative_path unchanged if no match is
// found anywhere, so downstream error messages still reference the path
// the caller asked for.
std::string resolve_repo_path(const std::string& relative_path);
