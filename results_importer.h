#pragma once
#include "result_row.h"
#include <string>
#include <vector>

class Storage;

// Bootstraps a normalized SQLite schema (circuits, teams, drivers, races,
// results) from a data directory's JSON files. One-time import: no-op if
// the database is already populated.
class ResultsImporter {
public:
    explicit ResultsImporter(Storage& storage);

    void create_tables();
    void import_from_json(const std::string& data_dir);

    // Reads every result row back out, joined against drivers/teams/circuits
    // and ordered by race_order. Every prediction model consumes this
    // instead of a JSON path.
    std::vector<ResultRow> load_results();

private:
    int upsert_driver(const std::string& name);
    int upsert_team(const std::string& name);
    int upsert_circuit(const std::string& name);
    int insert_race(int circuit_id, int race_order);
    void insert_result(int race_id, int driver_id, int team_id, int position, int grid);

    Storage& storage;
};
