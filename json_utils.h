#pragma once
#include <nlohmann/json.hpp>
#include <string>

// Reads and parses a JSON file, throwing std::runtime_error with the
// attempted path, its resolved absolute path, and the current working
// directory if the file can't be found or opened.
nlohmann::json load_json_array(const std::string& path);
