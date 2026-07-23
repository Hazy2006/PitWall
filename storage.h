#pragma once
#include <string>
#include <vector>
#include <map>

struct sqlite3;

class Storage {
private:
    sqlite3* db;

public:
    explicit Storage(const std::string& path);
    ~Storage();

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    void execute(const std::string& sql);
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
};
