#include "json_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <stdexcept>

using json = nlohmann::json;

json load_json_array(const std::string& path) {
    std::filesystem::path p(path);
    std::error_code ec;
    auto abs_path = std::filesystem::absolute(p, ec);

    std::string cwd;
    try {
        cwd = std::filesystem::current_path().string();
    } catch (...) {
        cwd = "<unavailable>";
    }

    if (ec) {
        std::ostringstream ss;
        ss << "Invalid path: " << path;
        ss << " (abs: " << abs_path.string() << ")";
        ss << " (error: " << ec.message() << ")";
        ss << " (cwd: " << cwd << ")";
        throw std::runtime_error(ss.str());
    }

    if (!std::filesystem::exists(abs_path, ec) || ec) {
        std::ostringstream ss;
        ss << "File not found: " << path << " (abs: " << abs_path.string() << ")";
        if (ec) ss << " (exists check error: " << ec.message() << ")";
        ss << " (cwd: " << cwd << ")";
        throw std::runtime_error(ss.str());
    }

    if (!std::filesystem::is_regular_file(abs_path, ec) || ec) {
        std::ostringstream ss;
        ss << "Not a regular file: " << path << " (abs: " << abs_path.string() << ")";
        if (ec) ss << " (is_regular_file error: " << ec.message() << ")";
        ss << " (cwd: " << cwd << ")";
        throw std::runtime_error(ss.str());
    }

    std::ifstream file(abs_path);
    if (!file.is_open()) {
        std::ostringstream ss;
        ss << "Cannot open file: " << path << " (abs: " << abs_path.string() << ")";
        ss << " (cwd: " << cwd << ")";
        throw std::runtime_error(ss.str());
    }

    json data;
    try {
        file >> data;
    } catch (const nlohmann::json::parse_error& e) {
        std::ostringstream ss;
        ss << "JSON parse error in file: " << abs_path.string() << " : " << e.what();
        throw std::runtime_error(ss.str());
    } catch (const std::exception& e) {
        std::ostringstream ss;
        ss << "I/O or JSON error reading file: " << abs_path.string() << " : " << e.what();
        throw std::runtime_error(ss.str());
    }

    return data;
}
