#include "json_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

json load_json_array(const std::string& path) {
    std::filesystem::path p(path);
    std::error_code ec;
    auto abs_path = std::filesystem::absolute(p, ec);

    if (!std::filesystem::exists(p)) {
        std::ostringstream ss;
        ss << "File not found: " << path;
        if (!ec) ss << " (abs: " << abs_path.string() << ")";
        ss << " (cwd: " << std::filesystem::current_path().string() << ")";
        throw std::runtime_error(ss.str());
    }

    std::ifstream file(p);
    if (!file.is_open()) {
        std::ostringstream ss;
        ss << "Cannot open file: " << path;
        if (!ec) ss << " (abs: " << abs_path.string() << ")";
        ss << " (cwd: " << std::filesystem::current_path().string() << ")";
        throw std::runtime_error(ss.str());
    }

    json data;
    file >> data;
    return data;
}
