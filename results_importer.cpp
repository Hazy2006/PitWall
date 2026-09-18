#include "results_importer.h"
#include "storage.h"
#include "json_utils.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <map>

using json = nlohmann::json;

ResultsImporter::ResultsImporter(Storage& storage) : storage(storage) {}

void ResultsImporter::create_tables() {
    storage.execute(
        "CREATE TABLE IF NOT EXISTS circuits ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL UNIQUE"
        ");"
    );
    storage.execute(
        "CREATE TABLE IF NOT EXISTS teams ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL UNIQUE"
        ");"
    );
    storage.execute(
        "CREATE TABLE IF NOT EXISTS drivers ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL UNIQUE"
        ");"
    );
    storage.execute(
        "CREATE TABLE IF NOT EXISTS races ("
        "id INTEGER PRIMARY KEY,"
        "circuit_id INTEGER NOT NULL REFERENCES circuits(id),"
        "race_order INTEGER NOT NULL UNIQUE"
        ");"
    );
    storage.execute(
        "CREATE TABLE IF NOT EXISTS results ("
        "id INTEGER PRIMARY KEY,"
        "race_id INTEGER NOT NULL REFERENCES races(id),"
        "driver_id INTEGER NOT NULL REFERENCES drivers(id),"
        "team_id INTEGER NOT NULL REFERENCES teams(id),"
        "position INTEGER NOT NULL,"
        "grid INTEGER NOT NULL"
        ");"
    );
}

int ResultsImporter::upsert_driver(const std::string& name) {
    storage.execute("INSERT OR IGNORE INTO drivers (name) VALUES (?);", { name });
    auto rows = storage.query("SELECT id FROM drivers WHERE name = ?;", { name });
    return std::stoi(rows.at(0).at("id"));
}

int ResultsImporter::upsert_team(const std::string& name) {
    storage.execute("INSERT OR IGNORE INTO teams (name) VALUES (?);", { name });
    auto rows = storage.query("SELECT id FROM teams WHERE name = ?;", { name });
    return std::stoi(rows.at(0).at("id"));
}

int ResultsImporter::upsert_circuit(const std::string& name) {
    storage.execute("INSERT OR IGNORE INTO circuits (name) VALUES (?);", { name });
    auto rows = storage.query("SELECT id FROM circuits WHERE name = ?;", { name });
    return std::stoi(rows.at(0).at("id"));
}

int ResultsImporter::insert_race(int circuit_id, int race_order) {
    storage.execute(
        "INSERT INTO races (circuit_id, race_order) VALUES (?, ?);",
        { static_cast<int64_t>(circuit_id), static_cast<int64_t>(race_order) }
    );
    auto rows = storage.query("SELECT id FROM races WHERE race_order = ?;", { static_cast<int64_t>(race_order) });
    return std::stoi(rows.at(0).at("id"));
}

void ResultsImporter::insert_result(int race_id, int driver_id, int team_id, int position, int grid) {
    storage.execute(
        "INSERT INTO results (race_id, driver_id, team_id, position, grid) VALUES (?, ?, ?, ?, ?);",
        { static_cast<int64_t>(race_id), static_cast<int64_t>(driver_id), static_cast<int64_t>(team_id),
          static_cast<int64_t>(position), static_cast<int64_t>(grid) }
    );
}

void ResultsImporter::import_from_json(const std::string& data_dir) {
    auto existing = storage.query("SELECT COUNT(*) AS n FROM races;");
    if (!existing.empty() && std::stoi(existing[0].at("n")) > 0) {
        return;
    }

    json drivers_json = load_json_array(data_dir + "/drivers.json");
    json teams_json = load_json_array(data_dir + "/teams.json");
    json circuits_json = load_json_array(data_dir + "/circuits.json");
    json results_json = load_json_array(data_dir + "/results.json");

    for (const auto& entry : drivers_json) {
        upsert_driver(entry.at("name").get<std::string>());
    }
    for (const auto& entry : teams_json) {
        upsert_team(entry.at("name").get<std::string>());
    }
    for (const auto& entry : circuits_json) {
        upsert_circuit(entry.at("name").get<std::string>());
    }

    std::map<std::string, int> race_id_by_circuit;
    int race_order = 0;
    for (const auto& entry : results_json) {
        std::string driver_name = entry.at("driver_name").get<std::string>();
        std::string team_name = entry.at("team_name").get<std::string>();
        std::string circuit_name = entry.at("circuit_name").get<std::string>();
        int position = entry.at("position").get<int>();
        int grid = entry.at("grid").get<int>();

        int driver_id = upsert_driver(driver_name);
        int team_id = upsert_team(team_name);
        int circuit_id = upsert_circuit(circuit_name);

        auto it = race_id_by_circuit.find(circuit_name);
        int race_id;
        if (it == race_id_by_circuit.end()) {
            race_id = insert_race(circuit_id, race_order++);
            race_id_by_circuit[circuit_name] = race_id;
        }
        else {
            race_id = it->second;
        }

        insert_result(race_id, driver_id, team_id, position, grid);
    }
}

std::vector<ResultRow> ResultsImporter::load_results() {
    auto rows = storage.query(
        "SELECT d.name AS driver_name, t.name AS team_name, c.name AS circuit_name, "
        "r.position, r.grid, ra.race_order "
        "FROM results r "
        "JOIN races ra ON r.race_id = ra.id "
        "JOIN drivers d ON r.driver_id = d.id "
        "JOIN teams t ON r.team_id = t.id "
        "JOIN circuits c ON ra.circuit_id = c.id "
        "ORDER BY ra.race_order ASC, r.id ASC;"
    );

    std::vector<ResultRow> results;
    results.reserve(rows.size());
    for (const auto& row : rows) {
        ResultRow rr;
        rr.driver_name = row.at("driver_name");
        rr.team_name = row.at("team_name");
        rr.circuit_name = row.at("circuit_name");
        rr.position = std::stoi(row.at("position"));
        rr.grid = std::stoi(row.at("grid"));
        rr.race_order = std::stoi(row.at("race_order"));
        results.push_back(std::move(rr));
    }
    return results;
}
