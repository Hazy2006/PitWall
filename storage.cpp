#include "storage.h"
#include <sqlite3.h>
#include <stdexcept>

Storage::Storage(const std::string& path) : db(nullptr) {
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string msg = db ? sqlite3_errmsg(db) : "unknown sqlite3_open failure";
        sqlite3_close(db);
        throw std::runtime_error("Failed to open database: " + msg);
    }
}

Storage::~Storage() {
    sqlite3_close(db);
}

void Storage::execute(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string msg = err_msg ? err_msg : "unknown sqlite3_exec failure";
        sqlite3_free(err_msg);
        throw std::runtime_error("SQL execute failed: " + msg);
    }
}

namespace {
    int collect_rows_callback(void* user_data, int column_count, char** column_values, char** column_names) {
        auto* rows = static_cast<std::vector<std::map<std::string, std::string>>*>(user_data);
        std::map<std::string, std::string> row;
        for (int i = 0; i < column_count; ++i) {
            row[column_names[i]] = column_values[i] ? column_values[i] : "";
        }
        rows->push_back(std::move(row));
        return 0;
    }
}

std::vector<std::map<std::string, std::string>> Storage::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> rows;
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), collect_rows_callback, &rows, &err_msg);
    if (rc != SQLITE_OK) {
        std::string msg = err_msg ? err_msg : "unknown sqlite3_exec failure";
        sqlite3_free(err_msg);
        throw std::runtime_error("SQL query failed: " + msg);
    }
    return rows;
}
