#include "storage.h"
#include <sqlite3.h>
#include <stdexcept>
#include <type_traits>

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

namespace {
    void bind_params(sqlite3_stmt* stmt, const std::vector<SqlParam>& params) {
        for (size_t i = 0; i < params.size(); ++i) {
            int index = static_cast<int>(i) + 1;
            std::visit([&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, int64_t>) {
                    sqlite3_bind_int64(stmt, index, value);
                }
                else if constexpr (std::is_same_v<T, double>) {
                    sqlite3_bind_double(stmt, index, value);
                }
                else {
                    sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
                }
            }, params[i]);
        }
    }

    sqlite3_stmt* prepare(sqlite3* db, const std::string& sql, const std::vector<SqlParam>& params) {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("SQL prepare failed: " + std::string(sqlite3_errmsg(db)));
        }
        bind_params(stmt, params);
        return stmt;
    }
}

void Storage::execute(const std::string& sql, const std::vector<SqlParam>& params) {
    sqlite3_stmt* stmt = prepare(db, sql, params);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("SQL execute failed: " + msg);
    }
    sqlite3_finalize(stmt);
}

std::vector<std::map<std::string, std::string>> Storage::query(const std::string& sql, const std::vector<SqlParam>& params) {
    sqlite3_stmt* stmt = prepare(db, sql, params);

    std::vector<std::map<std::string, std::string>> rows;
    int column_count = sqlite3_column_count(stmt);

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::map<std::string, std::string> row;
        for (int i = 0; i < column_count; ++i) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            row[sqlite3_column_name(stmt, i)] = text ? reinterpret_cast<const char*>(text) : "";
        }
        rows.push_back(std::move(row));
    }

    if (rc != SQLITE_DONE) {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("SQL query failed: " + msg);
    }
    sqlite3_finalize(stmt);
    return rows;
}
