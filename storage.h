#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

struct sqlite3;

// A bound parameter for a prepared statement, passed positionally for each
// '?' -- callers never escape or stringify a value by hand.
using SqlParam = std::variant<int64_t, double, std::string>;

class Storage {
private:
    sqlite3* db;

public:
    explicit Storage(const std::string& path);
    ~Storage();

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    void execute(const std::string& sql, const std::vector<SqlParam>& params = {});
    std::vector<std::map<std::string, std::string>> query(const std::string& sql, const std::vector<SqlParam>& params = {});
};
