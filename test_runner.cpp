#include "graph.h"
#include "storage.h"
#include "graph_repository.h"
#include "results_importer.h"
#include "data_importer.h"
#include "markov_trainer.h"
#include "markov_engine.h"
#include "strategy_reporter.h"
#include "driver_adjustment.h"
#include "service.h"
#include "championship_simulator.h"
#include "dirichlet_finish_model.h"
#include "path_utils.h"
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace {
    // Bridges a test's JSON fixture into a ResultRow vector via the real
    // ResultsImporter pipeline (in-memory DB), instead of a parallel
    // test-only JSON reader.
    std::vector<ResultRow> load_test_results(const std::filesystem::path& dir) {
        namespace fs = std::filesystem;
        for (const char* name : { "drivers.json", "teams.json", "circuits.json" }) {
            fs::path p = dir / name;
            if (!fs::exists(p)) {
                std::ofstream f(p);
                f << "[]";
            }
        }
        Storage storage(":memory:");
        ResultsImporter importer(storage);
        importer.create_tables();
        importer.import_from_json(dir.string());
        return importer.load_results();
    }
}

void run_domain_tests() {
    std::cout << "--- Running Polymorphic Domain Graph Test ---\n";
    Graph g;

    auto leclerc = std::make_shared<DriverNode>("Leclerc", 0.87, -0.2);
    auto ferrari = std::make_shared<TeamNode>("Ferrari", 2.1);

    int id1 = g.add_node(leclerc);
    int id2 = g.add_node(ferrari);

    bool ok = true;
    ok &= (id1 == 0);
    ok &= (id2 == 1);
    ok &= (g.count_vertices() == 2);

    ok &= (g.get_node(id1)->get_name() == "Leclerc");
    ok &= (g.get_node(id1)->get_type_string() == "Driver");
    ok &= (g.get_node(id2)->get_type_string() == "Team");

    if (ok) {
        std::cout << "[PASS] Polymorphic Domain Graph Tests Passed!\n";
    }
    else {
        std::cout << "[FAIL] Polymorphic Domain Graph Tests failed.\n";
    }
}

void run_edge_tests() {
    std::cout << "--- Running Edge Routing Test ---\n";
    Graph g;

    auto leclerc = std::make_shared<DriverNode>("Leclerc", 0.87, -0.2);
    auto monaco = std::make_shared<CircuitNode>("Monaco", 1.2);

    int driver_id = g.add_node(leclerc);
    int circuit_id = g.add_node(monaco);

    Edge stats = { 0.15 };

    g.add_edge(driver_id, circuit_id, stats);

    bool ok = true;
    ok &= (g.count_edges() == 1);
    ok &= (g.is_edge(driver_id, circuit_id) == true);
    ok &= (g.is_edge(circuit_id, driver_id) == false);

    Edge retrieved = g.get_edge(driver_id, circuit_id);
    ok &= (retrieved.winRate == 0.15);

    if (ok) {
        std::cout << "[PASS] Edge Routing Tests Passed!\n";
    }
    else {
        std::cout << "[FAIL] Edge Routing Tests failed.\n";
    }
}

void test_bfs() {
    std::cout << "--- Running BFS Test ---\n";

    Graph g;
    int id1 = g.add_node(std::make_shared<DriverNode>("Node1", 0.0, 0.0));
    int id2 = g.add_node(std::make_shared<DriverNode>("Node2", 0.0, 0.0));
    int id3 = g.add_node(std::make_shared<DriverNode>("Node3", 0.0, 0.0));

    g.add_edge(id1, id2, Edge{ 1.0 });
    g.add_edge(id2, id3, Edge{ 1.0 });

    std::vector<int> visited = g.bfs(id1);

    if (visited.size() == 3 && visited[0] == id1 && visited[1] == id2 && visited[2] == id3) {
        std::cout << "[PASS] BFS traversed nodes in the correct order.\n";
    }
    else {
        std::cout << "[FAIL] BFS did not return the expected order.\n";
    }
}

void test_dfs() {
    std::cout << "--- Running DFS Test ---\n";
    Graph g;

    int a = g.add_node(std::make_shared<DriverNode>("N1",10.0,1.0));
    int b = g.add_node(std::make_shared<DriverNode>("N2",11.0,1.0));
    int c = g.add_node(std::make_shared<DriverNode>("N3",12.0,1.0));
    int d = g.add_node(std::make_shared<DriverNode>("N4",13.0,1.0));


    g.add_edge(a, b, Edge{ 1.0 });
    g.add_edge(b, d, Edge{ 1.0 });
    g.add_edge(a, c, Edge{ 1.0 });

    std::vector<int> visited = g.dfs(a);
    if (visited.size() == 4 && visited[0] == a && visited[1] == b && visited[2] == d && visited[3] == c) {
        std::cout << "[PASS] DFS traversed nodes in the correct depth-first order.\n";
    }
    else {
        std::cout << "[FAIL] DFS did not return expected branch order.\n";
    }
}

void test_remove_node() {
    std::cout << "--- Running remove_node Test ---\n";
    Graph g;

    int a = g.add_node(std::make_shared<DriverNode>("N1", 10.0, 1.0));
    int b = g.add_node(std::make_shared<DriverNode>("N2", 11.0, 1.0));
    int c = g.add_node(std::make_shared<DriverNode>("N3", 12.0, 1.0));

    g.add_edge(a, b, Edge{ 1.0 });
    g.add_edge(b, c, Edge{ 1.0 });
    g.add_edge(a, c, Edge{ 1.0 });

    g.remove_node(b);

    bool ok = true;
    ok &= (g.count_vertices() == 2);
    ok &= (g.count_edges() == 1);
    ok &= (g.is_edge(a, c) == true);

    try {
        g.get_node(b);
        ok = false;
    }
    catch (const std::invalid_argument&) {
        // expected
    }

    try {
        g.is_edge(a, b);
        ok = false;
    }
    catch (const std::invalid_argument&) {
        // expected
    }

    if (ok) {
        std::cout << "[PASS] remove_node cleaned up registry, adjacency, and edges.\n";
    }
    else {
        std::cout << "[FAIL] remove_node left stale state behind.\n";
    }
}

void test_dijkstra() {
    std::cout << "--- Running Dijkstra Test ---\n";
    Graph g;

    int a = g.add_node(std::make_shared<DriverNode>("N1", 0.0, 0.0));
    int b = g.add_node(std::make_shared<DriverNode>("N2", 0.0, 0.0));
    int c = g.add_node(std::make_shared<DriverNode>("N3", 0.0, 0.0));
    int d = g.add_node(std::make_shared<DriverNode>("N4", 0.0, 0.0));

    g.add_edge(a, b, Edge{ 1.0 });
    g.add_edge(b, d, Edge{ 1.0 });
    g.add_edge(a, c, Edge{ 5.0 });
    g.add_edge(c, d, Edge{ 1.0 });

    std::vector<int> path = g.dijkstra(a, d);

    if (path.size() == 3 && path[0] == a && path[1] == b && path[2] == d) {
        std::cout << "[PASS] Dijkstra found the lowest-weight path.\n";
    }
    else {
        std::cout << "[FAIL] Dijkstra did not return the expected shortest path.\n";
    }
}

void test_save_and_load() {
    std::cout << "--- Running Save/Load (SQLite Repository) Test ---\n";

    Graph original;
    int driver_id = original.add_node(std::make_shared<DriverNode>("Leclerc", 0.87, -0.2));
    int team_id = original.add_node(std::make_shared<TeamNode>("Ferrari", 2.1));
    int circuit_id = original.add_node(std::make_shared<CircuitNode>("Monaco", 1.2));

    original.add_edge(driver_id, team_id, Edge{ 0.62 });
    original.add_edge(driver_id, circuit_id, Edge{ 0.15 });
    original.add_edge(team_id, circuit_id, Edge{ 0.40 });

    Storage storage(":memory:");
    GraphRepository repo(storage);
    repo.create_tables();
    repo.save_graph(original);

    Graph loaded = repo.load_graph();

    bool ok = true;
    ok &= (loaded.count_vertices() == original.count_vertices());
    ok &= (loaded.count_edges() == original.count_edges());

    ok &= (loaded.get_node(driver_id)->get_name() == "Leclerc");
    ok &= (loaded.get_node(driver_id)->get_type_string() == "Driver");
    ok &= (loaded.get_node(team_id)->get_name() == "Ferrari");
    ok &= (loaded.get_node(team_id)->get_type_string() == "Team");
    ok &= (loaded.get_node(circuit_id)->get_name() == "Monaco");
    ok &= (loaded.get_node(circuit_id)->get_type_string() == "Circuit");

    auto driver = std::dynamic_pointer_cast<DriverNode>(loaded.get_node(driver_id));
    ok &= (driver != nullptr && driver->tire_management_modifier == 0.87 && driver->base_pace_delta == -0.2);

    auto team = std::dynamic_pointer_cast<TeamNode>(loaded.get_node(team_id));
    ok &= (team != nullptr && team->pit_stop_variance == 2.1);

    auto circuit = std::dynamic_pointer_cast<CircuitNode>(loaded.get_node(circuit_id));
    ok &= (circuit != nullptr && circuit->base_degradation_rate == 1.2);

    Edge e1 = loaded.get_edge(driver_id, team_id);
    ok &= (e1.winRate == 0.62);

    Edge e2 = loaded.get_edge(driver_id, circuit_id);
    ok &= (e2.winRate == 0.15);

    Edge e3 = loaded.get_edge(team_id, circuit_id);
    ok &= (e3.winRate == 0.40);

    if (ok) {
        std::cout << "[PASS] Graph round-tripped through SQLite repository intact.\n";
    }
    else {
        std::cout << "[FAIL] Loaded graph does not match the original.\n";
    }
}

void test_save_and_load_with_id_gap() {
    std::cout << "--- Running Save/Load With ID Gap Test ---\n";

    Graph original;
    int a = original.add_node(std::make_shared<DriverNode>("N1", 1.0, 2.0));
    int b = original.add_node(std::make_shared<DriverNode>("N2", 3.0, 4.0));
    int c = original.add_node(std::make_shared<DriverNode>("N3", 5.0, 6.0));

    original.add_edge(a, c, Edge{ 0.20 });

    original.remove_node(b);

    Storage storage(":memory:");
    GraphRepository repo(storage);
    repo.create_tables();
    repo.save_graph(original);

    Graph loaded = repo.load_graph();

    bool ok = true;
    ok &= (loaded.count_vertices() == original.count_vertices());
    ok &= (loaded.count_edges() == original.count_edges());

    ok &= (loaded.get_node(a)->get_name() == "N1");
    ok &= (loaded.get_node(c)->get_name() == "N3");

    try {
        loaded.get_node(b);
        ok = false;
    }
    catch (const std::invalid_argument&) {
        // expected: id b was removed, the gap must survive the round trip
    }

    Edge e = loaded.get_edge(a, c);
    ok &= (e.winRate == 0.20);

    if (ok) {
        std::cout << "[PASS] Save/load preserved node IDs across a removal gap.\n";
    }
    else {
        std::cout << "[FAIL] Save/load did not preserve the ID gap correctly.\n";
    }
}

void test_results_importer() {
    std::cout << "--- Running ResultsImporter Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_results_importer_test";
    fs::create_directories(temp_dir);

    { std::ofstream f(temp_dir / "drivers.json"); f << R"([{"name": "Driver A"}])"; }
    { std::ofstream f(temp_dir / "teams.json"); f << R"([{"name": "Team One"}])"; }
    { std::ofstream f(temp_dir / "circuits.json"); f << R"([{"name": "Circuit One"}])"; }
    { std::ofstream f(temp_dir / "results.json");
      f << R"([
            {"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit One", "position": 2, "grid": 0, "team_name": "Team Two"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Two", "position": 3, "grid": 2, "team_name": "Team One"}
        ])"; }

    fs::path db_path = temp_dir / "test.db";
    fs::remove(db_path);
    bool ok = true;
    {
        Storage storage(db_path.string());
        ResultsImporter importer(storage);
        importer.create_tables();
        importer.import_from_json(temp_dir.string());

        // Driver B and Team Two only appear in results.json, not their
        // own metadata files -- must still be created.
        auto drivers = storage.query("SELECT name FROM drivers ORDER BY name;");
        ok &= (drivers.size() == 2);
        ok &= (drivers[0].at("name") == "Driver A");
        ok &= (drivers[1].at("name") == "Driver B");

        auto teams = storage.query("SELECT name FROM teams ORDER BY name;");
        ok &= (teams.size() == 2);

        auto races = storage.query("SELECT race_order FROM races ORDER BY race_order;");
        ok &= (races.size() == 2);
        ok &= (races[0].at("race_order") == "0");
        ok &= (races[1].at("race_order") == "1");

        // All 3 result rows preserved, including the grid==0 one.
        auto results = storage.query("SELECT grid FROM results;");
        ok &= (results.size() == 3);
    }
    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] ResultsImporter populated all tables, upserted entities missing from "
                     "metadata JSON, and preserved the grid==0 row.\n";
    }
    else {
        std::cout << "[FAIL] ResultsImporter output did not match expectations.\n";
    }
}

void test_results_importer_idempotent() {
    std::cout << "--- Running ResultsImporter Idempotency Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_results_importer_idempotent_test";
    fs::create_directories(temp_dir);

    { std::ofstream f(temp_dir / "drivers.json"); f << R"([{"name": "Driver A"}])"; }
    { std::ofstream f(temp_dir / "teams.json"); f << R"([{"name": "Team One"}])"; }
    { std::ofstream f(temp_dir / "circuits.json"); f << R"([{"name": "Circuit One"}])"; }
    { std::ofstream f(temp_dir / "results.json");
      f << R"([{"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"}])"; }

    fs::path db_path = temp_dir / "test.db";
    fs::remove(db_path);
    bool ok = true;
    {
        Storage storage(db_path.string());
        ResultsImporter importer(storage);
        importer.create_tables();
        importer.import_from_json(temp_dir.string());
        importer.import_from_json(temp_dir.string());  // must no-op

        auto results = storage.query("SELECT COUNT(*) AS n FROM results;");
        ok &= (results.at(0).at("n") == "1");
    }
    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] ResultsImporter's second call was a no-op against an already-populated database.\n";
    }
    else {
        std::cout << "[FAIL] ResultsImporter was not idempotent.\n";
    }
}

void test_results_importer_load_results() {
    std::cout << "--- Running ResultsImporter::load_results Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_results_importer_load_results_test";
    fs::create_directories(temp_dir);

    { std::ofstream f(temp_dir / "drivers.json"); f << "[]"; }
    { std::ofstream f(temp_dir / "teams.json"); f << "[]"; }
    { std::ofstream f(temp_dir / "circuits.json"); f << "[]"; }
    { std::ofstream f(temp_dir / "results.json");
      f << R"([
            {"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit One", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Two", "position": 3, "grid": 2, "team_name": "Team One"}
        ])"; }

    bool ok = true;
    {
        Storage storage(":memory:");
        ResultsImporter importer(storage);
        importer.create_tables();
        importer.import_from_json(temp_dir.string());

        std::vector<ResultRow> results = importer.load_results();

        ok &= (results.size() == 3);

        // Ordered by race_order (Circuit One = 0, Circuit Two = 1), not by
        // insertion order of drivers within a race -- if this were wrong,
        // Circuit Two's row wouldn't sort last.
        ok &= (results[0].circuit_name == "Circuit One") && (results[0].race_order == 0);
        ok &= (results[1].circuit_name == "Circuit One") && (results[1].race_order == 0);
        ok &= (results[2].circuit_name == "Circuit Two") && (results[2].race_order == 1);

        // Field-by-field check on one row: every ResultRow field must survive
        // the JOIN round-trip, not just the ones convenient to check above.
        ok &= (results[2].driver_name == "Driver A");
        ok &= (results[2].team_name == "Team One");
        ok &= (results[2].position == 3);
        ok &= (results[2].grid == 2);
    }
    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] ResultsImporter::load_results returned every field correctly joined and ordered by race_order.\n";
    }
    else {
        std::cout << "[FAIL] ResultsImporter::load_results output did not match expectations.\n";
    }
}

void test_data_importer() {
    std::cout << "--- Running DataImporter Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_data_importer_test";
    fs::create_directories(temp_dir);

    fs::path drivers_path = temp_dir / "drivers.json";
    fs::path teams_path = temp_dir / "teams.json";
    fs::path circuits_path = temp_dir / "circuits.json";
    fs::path results_path = temp_dir / "results.json";

    {
        std::ofstream f(drivers_path);
        f << R"([
            {"name": "Driver One", "tire_management_modifier": 0.5, "base_pace_delta": -0.1},
            {"name": "Driver Two", "tire_management_modifier": 0.6, "base_pace_delta": 0.2}
        ])";
    }
    {
        std::ofstream f(teams_path);
        f << R"([
            {"name": "Team One", "pit_stop_variance": 1.5}
        ])";
    }
    {
        std::ofstream f(circuits_path);
        f << R"([
            {"name": "Circuit One", "base_degradation_rate": 0.8}
        ])";
    }
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver One", "circuit_name": "Circuit One", "position": 1, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver One", "circuit_name": "Circuit One", "position": 3, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Two", "circuit_name": "Circuit One", "position": 2, "grid": 3, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);

    Graph g;
    DataImporter importer(g);
    importer.import_drivers(drivers_path.string());
    importer.import_teams(teams_path.string());
    importer.import_circuits(circuits_path.string());
    importer.import_results(results);

    bool ok = true;
    ok &= (g.count_vertices() == 4);
    ok &= (g.count_edges() == 2);

    int driver_one_id = -1, driver_two_id = -1, circuit_one_id = -1;
    for (int id : g.get_all_node_ids()) {
        auto node = g.get_node(id);
        if (node->get_name() == "Driver One") driver_one_id = id;
        if (node->get_name() == "Driver Two") driver_two_id = id;
        if (node->get_name() == "Circuit One") circuit_one_id = id;
    }
    ok &= (driver_one_id != -1 && driver_two_id != -1 && circuit_one_id != -1);

    ok &= g.is_edge(driver_one_id, circuit_one_id);
    ok &= g.is_edge(driver_two_id, circuit_one_id);

    Edge e1 = g.get_edge(driver_one_id, circuit_one_id);
    ok &= (e1.winRate == 0.5);

    Edge e2 = g.get_edge(driver_two_id, circuit_one_id);
    ok &= (e2.winRate == 0.0);

    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] DataImporter populated the graph and computed win rates correctly.\n";
    }
    else {
        std::cout << "[FAIL] DataImporter output did not match expectations.\n";
    }
}

void test_real_import() {
    std::cout << "--- Running Real Data Import Smoke Test ---\n";

    std::vector<ResultRow> results = load_test_results(std::filesystem::path(resolve_repo_path("data")));

    Graph g;
    DataImporter importer(g);

    importer.import_drivers(resolve_repo_path("data/drivers.json"));
    importer.import_teams(resolve_repo_path("data/teams.json"));
    importer.import_circuits(resolve_repo_path("data/circuits.json"));
    importer.import_results(results);

    std::cout << "Total nodes: " << g.count_vertices() << "\n";
    std::cout << "Total edges: " << g.count_edges() << "\n";

    for (int id : g.get_all_node_ids()) {
        auto node = g.get_node(id);
        std::cout << "  [" << id << "] " << node->get_name() << " (" << node->get_type_string() << ")\n";
    }
}

void test_markov_trainer() {
    std::cout << "--- Running MarkovTrainer Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_markov_trainer_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver One", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Two", "circuit_name": "Circuit One", "position": 2, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Three", "circuit_name": "Circuit One", "position": 1, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver Four", "circuit_name": "Circuit One", "position": 5, "grid": 0, "team_name": "Team One"},
            {"driver_name": "Driver Five", "circuit_name": "Circuit One", "position": 3, "grid": 2, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    trainer.train(results);

    fs::remove_all(temp_dir);

    const auto& counts = trainer.get_counts();

    bool ok = true;
    // grid 0 row must be filtered: only grid 1 and grid 2 keys should exist
    ok &= (counts.size() == 2);
    ok &= (counts.count(0) == 0);

    // grid 1 -> finish 1: count 1, finish 2: count 1
    ok &= (counts.at(1).at(1) == 1);
    ok &= (counts.at(1).at(2) == 1);

    // grid 2 -> finish 1: count 1, finish 3: count 1
    ok &= (counts.at(2).at(1) == 1);
    ok &= (counts.at(2).at(3) == 1);

    ok &= (trainer.total_observations() == 4);

    if (ok) {
        std::cout << "[PASS] MarkovTrainer filtered grid==0 and counted transitions correctly.\n";
    }
    else {
        std::cout << "[FAIL] MarkovTrainer counts did not match expectations.\n";
    }
}

void test_markov_recency_weighting() {
    std::cout << "--- Running MarkovTrainer Recency Weighting Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_markov_recency_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // 4 races, all starting from grid 5: an old finish (P8, race 1) and a
    // recent finish (P2, race 4) at the same grid slot. Equal weighting
    // would treat them identically; recency weighting must not.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver Q", "circuit_name": "Circuit One", "position": 8, "grid": 5, "team_name": "Team One"},
            {"driver_name": "Driver R", "circuit_name": "Circuit Two", "position": 6, "grid": 5, "team_name": "Team One"},
            {"driver_name": "Driver S", "circuit_name": "Circuit Three", "position": 4, "grid": 5, "team_name": "Team One"},
            {"driver_name": "Driver T", "circuit_name": "Circuit Four", "position": 2, "grid": 5, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    trainer.train(results);
    fs::remove_all(temp_dir);

    const auto& counts = trainer.get_counts();
    const double epsilon = 1e-9;
    const double lambda = 0.85;
    bool ok = true;

    // Anchored to the most recent race (race 4, k=0): race 1 (P8) is k=3
    // races back, race 2 (P6) is k=2, race 3 (P4) is k=1, race 4 (P2) is k=0.
    ok &= (counts.count(5) == 1);
    const auto& row = counts.at(5);
    ok &= (row.count(8) == 1) && (std::fabs(row.at(8) - std::pow(lambda, 3)) < epsilon);
    ok &= (row.count(6) == 1) && (std::fabs(row.at(6) - std::pow(lambda, 2)) < epsilon);
    ok &= (row.count(4) == 1) && (std::fabs(row.at(4) - std::pow(lambda, 1)) < epsilon);
    ok &= (row.count(2) == 1) && (std::fabs(row.at(2) - std::pow(lambda, 0)) < epsilon);

    // The most recent finish must carry more weight than the oldest one,
    // even though both are single observations.
    ok &= (row.at(2) > row.at(8));

    // Feeding the weighted table into MarkovEngine should make the most
    // recent finish the plurality favorite from that grid slot, not
    // whichever finish happens to sort first.
    MarkovEngine engine(counts);
    ok &= (engine.most_likely_finish(5) == 2);

    if (ok) {
        std::cout << "[PASS] MarkovTrainer weighted transition counts by recency (lambda=0.85), anchored to the "
                     "most recent race, and let a recent finish outweigh an older one at the same grid slot.\n";
    }
    else {
        std::cout << "[FAIL] MarkovTrainer recency weighting did not match expectations.\n";
    }
}

void test_markov_engine() {
    std::cout << "--- Running MarkovEngine Test ---\n";

    std::map<int, std::map<int, double>> counts;
    // grid 1: finish 1 x3, finish 2 x1  (total 4)
    counts[1][1] = 3;
    counts[1][2] = 1;
    // grid 2: finish 1 x1, finish 3 x1  (total 2)
    counts[2][1] = 1;
    counts[2][3] = 1;

    MarkovEngine engine(counts);

    bool ok = true;
    const double epsilon = 1e-9;

    std::map<int, double> dist1 = engine.predict_finish_distribution(1);
    ok &= (dist1.size() == 2);
    ok &= (std::fabs(dist1.at(1) - 0.75) < epsilon);
    ok &= (std::fabs(dist1.at(2) - 0.25) < epsilon);

    double sum1 = 0.0;
    for (const auto& [finish, prob] : dist1) {
        sum1 += prob;
    }
    ok &= (std::fabs(sum1 - 1.0) < epsilon);

    std::map<int, double> dist2 = engine.predict_finish_distribution(2);
    double sum2 = 0.0;
    for (const auto& [finish, prob] : dist2) {
        sum2 += prob;
    }
    ok &= (std::fabs(sum2 - 1.0) < epsilon);

    ok &= (engine.most_likely_finish(1) == 1);

    // unseen grid position
    std::map<int, double> dist_unseen = engine.predict_finish_distribution(99);
    ok &= dist_unseen.empty();
    ok &= (engine.most_likely_finish(99) == -1);

    if (ok) {
        std::cout << "[PASS] MarkovEngine normalized distributions and predicted correctly.\n";
    }
    else {
        std::cout << "[FAIL] MarkovEngine output did not match expectations.\n";
    }
}

void test_markov_real() {
    std::cout << "--- Running Real Data MarkovTrainer Smoke Test ---\n";

    std::vector<ResultRow> real_results = load_test_results(std::filesystem::path(resolve_repo_path("data")));
    MarkovTrainer trainer;
    trainer.train(real_results);

    std::cout << "Total observations: " << trainer.total_observations() << "\n";

    MarkovEngine engine(trainer.get_counts());

    for (int grid : {1, 5, 10, 15}) {
        int best = engine.most_likely_finish(grid);
        std::cout << "Grid " << grid << " -> most likely finish: " << best << "\n";

        std::map<int, double> distribution = engine.predict_finish_distribution(grid);

        std::vector<std::pair<int, double>> sorted_by_prob(distribution.begin(), distribution.end());
        std::sort(sorted_by_prob.begin(), sorted_by_prob.end(),
            [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                return a.second > b.second;
            });

        std::cout << "  Top " << (sorted_by_prob.size() < 3 ? sorted_by_prob.size() : 3) << " finishes:\n";
        for (size_t i = 0; i < sorted_by_prob.size() && i < 3; ++i) {
            std::cout << "    finish " << sorted_by_prob[i].first
                       << " : " << sorted_by_prob[i].second << "\n";
        }
    }
}

void test_driver_index() {
    std::cout << "--- Running Driver Performance Index Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_driver_index_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    {
        std::ostringstream rows;
        // Alpha: 10 valid rows at (grid - finish) = 3 -> mean 3.0, meets the 10-row threshold.
        for (int i = 0; i < 10; ++i) {
            rows << R"({"driver_name": "Alpha", "circuit_name": "Circuit One", "position": 2, "grid": 5, "team_name": "Team One"},)";
        }
        // Beta: only 2 valid rows -> below threshold -> omitted.
        rows << R"({"driver_name": "Beta", "circuit_name": "Circuit One", "position": 6, "grid": 3, "team_name": "Team Two"},)";
        rows << R"({"driver_name": "Beta", "circuit_name": "Circuit Two", "position": 2, "grid": 1, "team_name": "Team Two"},)";
        // Gamma: grid==0 only -> zero valid rows -> omitted.
        rows << R"({"driver_name": "Gamma", "circuit_name": "Circuit One", "position": 9, "grid": 0, "team_name": "Team Three"})";

        std::ofstream f(results_path);
        f << "[" << rows.str() << "]";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    std::map<std::string, double> indices = trainer.compute_driver_indices(results);

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    bool ok = true;

    ok &= (indices.count("Alpha") == 1);
    ok &= (std::fabs(indices.at("Alpha") - 3.0) < epsilon);
    ok &= (indices.count("Beta") == 0);
    ok &= (indices.count("Gamma") == 0);
    ok &= (indices.size() == 1);  // only drivers meeting the threshold are present

    if (ok) {
        std::cout << "[PASS] compute_driver_indices computed correct means and filtered grid==0 rows.\n";
    }
    else {
        std::cout << "[FAIL] compute_driver_indices output did not match expectations.\n";
    }
}

void test_compute_team_indices() {
    std::cout << "--- Running Team Performance Index Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_team_index_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    {
        std::ostringstream rows;
        // Team One: 10 valid rows total, split across two drivers, all at
        // (grid - finish) = 2 -> mean 2.0, meets the 10-row threshold.
        for (int i = 0; i < 5; ++i) {
            rows << R"({"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 3, "grid": 5, "team_name": "Team One"},)";
        }
        for (int i = 0; i < 5; ++i) {
            rows << R"({"driver_name": "Driver B", "circuit_name": "Circuit Two", "position": 3, "grid": 5, "team_name": "Team One"},)";
        }
        // Team Two: only 3 valid rows -> below threshold -> omitted.
        rows << R"({"driver_name": "Driver C", "circuit_name": "Circuit One", "position": 6, "grid": 4, "team_name": "Team Two"},)";
        rows << R"({"driver_name": "Driver C", "circuit_name": "Circuit Two", "position": 5, "grid": 4, "team_name": "Team Two"},)";
        rows << R"({"driver_name": "Driver C", "circuit_name": "Circuit Three", "position": 4, "grid": 4, "team_name": "Team Two"})";

        std::ofstream f(results_path);
        f << "[" << rows.str() << "]";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    std::map<std::string, double> team_indices = trainer.compute_team_indices(results);

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    bool ok = true;

    ok &= (team_indices.count("Team One") == 1);
    ok &= (std::fabs(team_indices.at("Team One") - 2.0) < epsilon);
    ok &= (team_indices.count("Team Two") == 0);
    ok &= (team_indices.size() == 1);

    if (ok) {
        std::cout << "[PASS] compute_team_indices pooled both drivers into one mean and enforced the row threshold.\n";
    }
    else {
        std::cout << "[FAIL] compute_team_indices output did not match expectations.\n";
    }
}

void test_compute_driver_teams() {
    std::cout << "--- Running Driver-Team Lookup Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_driver_teams_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Driver M starts at Team Old, then moves to Team New mid-season --
    // the most recent (later in file order) team must win.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver M", "circuit_name": "Circuit One", "position": 5, "grid": 5, "team_name": "Team Old"},
            {"driver_name": "Driver N", "circuit_name": "Circuit One", "position": 6, "grid": 6, "team_name": "Team Stable"},
            {"driver_name": "Driver M", "circuit_name": "Circuit Two", "position": 3, "grid": 3, "team_name": "Team New"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    std::map<std::string, std::string> driver_teams = trainer.compute_driver_teams(results);

    fs::remove_all(temp_dir);

    bool ok = true;
    ok &= (driver_teams.count("Driver M") == 1) && (driver_teams.at("Driver M") == "Team New");
    ok &= (driver_teams.count("Driver N") == 1) && (driver_teams.at("Driver N") == "Team Stable");

    if (ok) {
        std::cout << "[PASS] compute_driver_teams resolved a mid-season team change to the most recent team.\n";
    }
    else {
        std::cout << "[FAIL] compute_driver_teams output did not match expectations.\n";
    }
}

void test_driver_delta_distribution() {
    std::cout << "--- Running Driver Delta Distribution Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_driver_delta_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // 4 valid rows total: delta=2 once, delta=0 three times. Pooled
    // frequency: {2: 0.25, 0: 0.75}. Driver X has 3 valid rows (2, 0, 0);
    // Driver Y has 1 (0); Driver Z appears only in a grid==0 row (zero
    // evidence -- gets the pooled shape exactly).
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver X", "circuit_name": "Circuit One", "position": 1, "grid": 3, "team_name": "Team One"},
            {"driver_name": "Driver Y", "circuit_name": "Circuit One", "position": 2, "grid": 2, "team_name": "Team Two"},
            {"driver_name": "Driver X", "circuit_name": "Circuit Two", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver X", "circuit_name": "Circuit Three", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Z", "circuit_name": "Circuit Four", "position": 5, "grid": 0, "team_name": "Team Two"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    std::map<std::string, std::map<int, double>> distributions =
        trainer.compute_driver_delta_distributions(results);

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    const double lambda = 0.85;
    const double strength = 3.0;
    bool ok = true;

    // Prior mass: strength * pooled frequency, not "1 per bucket".
    double prior2 = strength * 0.25;
    double prior0 = strength * 0.75;

    // Driver X: race0 (delta=2, k=3), race1 (delta=0, k=2), race2 (delta=0, k=1).
    double x_delta2 = prior2 + std::pow(lambda, 3);
    double x_delta0 = prior0 + std::pow(lambda, 2) + std::pow(lambda, 1);
    double x_total = x_delta2 + x_delta0;

    ok &= (distributions.count("Driver X") == 1);
    const auto& x_dist = distributions.at("Driver X");
    ok &= (x_dist.size() == 2);
    ok &= (x_dist.count(2) == 1) && (std::fabs(x_dist.at(2) - x_delta2 / x_total) < epsilon);
    ok &= (x_dist.count(0) == 1) && (std::fabs(x_dist.at(0) - x_delta0 / x_total) < epsilon);

    // Driver Y: race0 only (delta=0, k=3); delta=2 stays pure prior.
    double y_delta0 = prior0 + std::pow(lambda, 3);
    double y_delta2 = prior2;
    double y_total = y_delta0 + y_delta2;

    ok &= (distributions.count("Driver Y") == 1);
    const auto& y_dist = distributions.at("Driver Y");
    ok &= (y_dist.count(0) == 1) && (std::fabs(y_dist.at(0) - y_delta0 / y_total) < epsilon);
    ok &= (y_dist.count(2) == 1) && (std::fabs(y_dist.at(2) - y_delta2 / y_total) < epsilon);

    // Driver Z: zero evidence -- must match the pooled shape exactly.
    ok &= (distributions.count("Driver Z") == 1);
    const auto& z_dist = distributions.at("Driver Z");
    ok &= (z_dist.size() == 2);
    ok &= (z_dist.count(0) == 1) && (std::fabs(z_dist.at(0) - 0.75) < epsilon);
    ok &= (z_dist.count(2) == 1) && (std::fabs(z_dist.at(2) - 0.25) < epsilon);

    for (const auto& [driver, dist] : distributions) {
        double sum = 0.0;
        for (const auto& [delta, prob] : dist) {
            sum += prob;
        }
        ok &= (std::fabs(sum - 1.0) < epsilon);
    }

    if (ok) {
        std::cout << "[PASS] compute_driver_delta_distributions matched hand-calculated recency-weighted "
                     "posteriors and gave a zero-evidence driver the pooled field shape.\n";
    }
    else {
        std::cout << "[FAIL] compute_driver_delta_distributions output did not match expectations.\n";
    }
}

void test_team_delta_distribution() {
    std::cout << "--- Running Team Delta Distribution Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_team_delta_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Two races, two different drivers on the same team: their evidence
    // must pool into one shared team distribution.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 3, "grid": 5, "team_name": "Team Alpha"},
            {"driver_name": "Driver B", "circuit_name": "Circuit Two", "position": 4, "grid": 5, "team_name": "Team Alpha"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    MarkovTrainer trainer;
    std::map<std::string, std::map<int, double>> distributions =
        trainer.compute_team_delta_distributions(results);

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    const double lambda = 0.85;
    const double strength = 3.0;
    bool ok = true;

    // Pooled frequency (2 valid rows, one each): {2: 0.5, 1: 0.5}.
    // Driver A: delta=2, k=1 (race0, 2 races before the last). Driver B:
    // delta=1, k=0 (race1, the most recent). Both pool into Team Alpha.
    double delta2 = strength * 0.5 + std::pow(lambda, 1);
    double delta1 = strength * 0.5 + std::pow(lambda, 0);
    double total = delta1 + delta2;

    ok &= (distributions.count("Team Alpha") == 1);
    const auto& dist = distributions.at("Team Alpha");
    ok &= (dist.size() == 2);
    ok &= (dist.count(2) == 1) && (std::fabs(dist.at(2) - delta2 / total) < epsilon);
    ok &= (dist.count(1) == 1) && (std::fabs(dist.at(1) - delta1 / total) < epsilon);

    if (ok) {
        std::cout << "[PASS] compute_team_delta_distributions pooled both drivers' evidence into one distribution.\n";
    }
    else {
        std::cout << "[FAIL] compute_team_delta_distributions output did not match expectations.\n";
    }
}

void test_driver_aware_prediction() {
    std::cout << "--- Running Driver-Aware Prediction Test ---\n";

    // Pooled grid 5: finish 2 (30%), finish 5 (50%), finish 8 (20%).
    std::map<int, std::map<int, double>> counts;
    counts[5][2] = 3;
    counts[5][5] = 5;
    counts[5][8] = 2;
    MarkovEngine engine(counts);

    const double epsilon = 1e-9;
    bool ok = true;

    // Delta distribution: 60% chance of gaining 1, 40% chance of gaining 3.
    // Convolved by hand: {1: 0.30, 2: 0.20, 4: 0.30, 5: 0.08, 7: 0.12} --
    // a split shape a single mean shift (1.8) could never produce.
    std::map<int, double> delta_dist = { {1, 0.6}, {3, 0.4} };
    std::map<int, double> shifted = engine.predict_finish_distribution_for_driver(5, delta_dist);

    ok &= (shifted.size() == 5);
    ok &= (shifted.count(1) == 1) && (std::fabs(shifted.at(1) - 0.30) < epsilon);
    ok &= (shifted.count(2) == 1) && (std::fabs(shifted.at(2) - 0.20) < epsilon);
    ok &= (shifted.count(4) == 1) && (std::fabs(shifted.at(4) - 0.30) < epsilon);
    ok &= (shifted.count(5) == 1) && (std::fabs(shifted.at(5) - 0.08) < epsilon);
    ok &= (shifted.count(7) == 1) && (std::fabs(shifted.at(7) - 0.12) < epsilon);

    double sum = 0.0;
    for (const auto& [finish, prob] : shifted) {
        ok &= (finish >= 1);
        sum += prob;
    }
    ok &= (std::fabs(sum - 1.0) < epsilon);

    // A large single-value delta clamps everything to P1.
    std::map<int, std::map<int, double>> counts2;
    counts2[1][1] = 2;
    counts2[1][2] = 2;
    counts2[1][3] = 4;
    MarkovEngine engine2(counts2);

    std::map<int, double> huge_delta = { {10, 1.0} };
    std::map<int, double> fully_clamped = engine2.predict_finish_distribution_for_driver(1, huge_delta);
    ok &= (fully_clamped.size() == 1);
    ok &= (fully_clamped.count(1) == 1) && (std::fabs(fully_clamped.at(1) - 1.0) < epsilon);

    // Unseen grid position returns an empty map, no throw.
    std::map<int, double> unseen = engine2.predict_finish_distribution_for_driver(99, delta_dist);
    ok &= unseen.empty();

    if (ok) {
        std::cout << "[PASS] predict_finish_distribution_for_driver convolved the delta distribution correctly, "
                     "respected the P1 clamp, and conserved total probability.\n";
    }
    else {
        std::cout << "[FAIL] predict_finish_distribution_for_driver output did not match expectations.\n";
    }
}

void test_driver_index_real() {
    std::cout << "--- Running Real Data Driver Index Smoke Test ---\n";

    std::vector<ResultRow> results = load_test_results(std::filesystem::path(resolve_repo_path("data")));
    MarkovTrainer trainer;
    trainer.train(results);
    std::map<std::string, double> indices = trainer.compute_driver_indices(results);

    std::vector<std::pair<std::string, double>> sorted_indices(indices.begin(), indices.end());
    std::sort(sorted_indices.begin(), sorted_indices.end(),
        [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
            return a.second > b.second;
        });

    std::cout << "Top 5 drivers by performance index (positions gained/lost):\n";
    for (size_t i = 0; i < sorted_indices.size() && i < 5; ++i) {
        std::cout << "  " << sorted_indices[i].first << " : " << sorted_indices[i].second << "\n";
    }

    std::cout << "Bottom 5 drivers by performance index:\n";
    for (size_t i = 0; i < sorted_indices.size() && i < 5; ++i) {
        size_t idx = sorted_indices.size() - 1 - i;
        std::cout << "  " << sorted_indices[idx].first << " : " << sorted_indices[idx].second << "\n";
    }

    // Integration check: the full service pipeline should produce reports
    // consistent with the strong/weak drivers' computed indices.
    PitWallService service;
    service.load("data");

    MarkovEngine engine(trainer.get_counts());

    auto print_distribution = [](const std::string& label, const std::map<int, double>& dist) {
        std::vector<std::pair<int, double>> sorted_by_prob(dist.begin(), dist.end());
        std::sort(sorted_by_prob.begin(), sorted_by_prob.end(),
            [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                return a.second > b.second;
            });
        std::cout << "  " << label << ":\n";
        for (size_t i = 0; i < sorted_by_prob.size() && i < 5; ++i) {
            std::cout << "    finish " << sorted_by_prob[i].first
                       << " : " << sorted_by_prob[i].second << "\n";
        }
    };

    std::map<int, double> pooled_p5 = engine.predict_finish_distribution(5);

    if (!sorted_indices.empty()) {
        const std::string& strong_driver = sorted_indices.front().first;
        const std::string& weak_driver = sorted_indices.back().first;

        std::cout << "Strong driver: " << strong_driver << " (index " << sorted_indices.front().second << ")\n";
        print_distribution("Pooled P5 distribution", pooled_p5);
        std::cout << "  Driver-aware report: " << service.report(5, strong_driver) << "\n";

        std::cout << "Weak driver: " << weak_driver << " (index " << sorted_indices.back().second << ")\n";
        print_distribution("Pooled P5 distribution", pooled_p5);
        std::cout << "  Driver-aware report: " << service.report(5, weak_driver) << "\n";
    }
}

void test_strategy_reporter() {
    std::cout << "--- Running Strategy Reporter Test ---\n";
    bool ok = true;

    // Pooled grid 5: finish 4 x5 (50%), finish 6 x3 (30%), finish 8 x2 (20%), total 10.
    std::map<int, std::map<int, double>> counts;
    counts[5][4] = 5;
    counts[5][6] = 3;
    counts[5][8] = 2;
    MarkovEngine engine(counts);

    // Mean 1.85 (rounds to 1.9); convolving it against the pooled table
    // below puts most mass on P2, matching the assertions further down.
    std::map<int, double> delta_dist = { {1, 0.15}, {2, 0.85} };

    std::map<std::string, DriverAdjustment> indices;
    indices["Test Driver"] = { 1.85, delta_dist, false };
    indices["Rookie Driver"] = { 1.85, delta_dist, true }; // same shift, via team fallback

    StrategyReporter reporter(engine, indices);

    // --- Known driver: correct most-likely finish, percentage, and adjustment ---
    std::string report = reporter.report_single(5, "Test Driver");

    // Pooled prediction: most likely finish is P4 at 50%.
    ok &= (report.find("P4") != std::string::npos);
    ok &= (report.find("50%") != std::string::npos);

    // Distribution shape: next two outcomes, P6 (30%) and P8 (20%).
    ok &= (report.find("P6") != std::string::npos);
    ok &= (report.find("30%") != std::string::npos);
    ok &= (report.find("P8") != std::string::npos);
    ok &= (report.find("20%") != std::string::npos);

    // Index rounds to 1.9; shifted distribution's most mass lands on P2.
    ok &= (report.find("Test Driver tends to gain 1.9 positions") != std::string::npos);
    ok &= (report.find("P2") != std::string::npos);

    if (!ok) {
        std::cout << "[FAIL] Known-driver report missing expected content: " << report << "\n";
    }

    // --- Driver NOT in the index map: honest "no adjustment" path ---
    std::string unindexed_report = reporter.report_single(5, "Nobody Special");
    bool unindexed_ok = true;
    unindexed_ok &= (unindexed_report.find("No driver-specific adjustment is available for Nobody Special") != std::string::npos);
    unindexed_ok &= (unindexed_report.find("fewer than 10 races") != std::string::npos);
    // Must not fabricate an index or a shifted-finish claim for this driver.
    unindexed_ok &= (unindexed_report.find("tends to") == std::string::npos);
    unindexed_ok &= (unindexed_report.find("shifting") == std::string::npos);
    ok &= unindexed_ok;

    if (!unindexed_ok) {
        std::cout << "[FAIL] Unindexed-driver report was not honest about missing data: " << unindexed_report << "\n";
    }

    // --- Team-fallback driver: same shift math as an own-data driver, but
    // labeled as coming from the team, not fabricated as personal form ---
    std::string fallback_report = reporter.report_single(5, "Rookie Driver");
    bool fallback_ok = true;
    fallback_ok &= (fallback_report.find("Rookie Driver tends to gain 1.9 positions") != std::string::npos);
    fallback_ok &= (fallback_report.find("based on team-level form") != std::string::npos);
    fallback_ok &= (fallback_report.find("fewer than 10 personal races") != std::string::npos);
    // Must not be confused with the "no data at all" path.
    fallback_ok &= (fallback_report.find("No driver-specific adjustment") == std::string::npos);
    ok &= fallback_ok;

    if (!fallback_ok) {
        std::cout << "[FAIL] Team-fallback report did not apply the shift or label its source correctly: " << fallback_report << "\n";
    }

    // --- Unseen grid position: clear "no data" message, no fabrication ---
    std::string no_data_report = reporter.report_single(99, "Test Driver");
    bool no_data_ok = (no_data_report.find("No historical data") != std::string::npos)
        && (no_data_report.find("P99") != std::string::npos);
    ok &= no_data_ok;

    if (!no_data_ok) {
        std::cout << "[FAIL] Unseen grid position did not return a clear no-data message: " << no_data_report << "\n";
    }

    if (ok) {
        std::cout << "[PASS] StrategyReporter reported correct percentages, distribution shape, "
                     "an honest driver adjustment, an honest no-adjustment path, and an honest no-data path.\n";
    }
}

void test_strategy_reporter_real() {
    std::cout << "--- Running Real Data Strategy Reporter Smoke Test ---\n";

    std::vector<ResultRow> results = load_test_results(std::filesystem::path(resolve_repo_path("data")));
    MarkovTrainer trainer;
    trainer.train(results);
    std::map<std::string, double> raw_indices = trainer.compute_driver_indices(results);
    std::map<std::string, std::map<int, double>> raw_deltas = trainer.compute_driver_delta_distributions(results);

    // Index derived from the same distribution used for the shift, matching
    // PitWallService, so narration and shift can never quietly disagree.
    auto mean_of = [](const std::map<int, double>& dist) {
        double mean = 0.0;
        for (const auto& [delta, prob] : dist) {
            mean += delta * prob;
        }
        return mean;
    };

    std::map<std::string, DriverAdjustment> indices;
    for (const auto& [driver, idx] : raw_indices) {
        const auto& dist = raw_deltas.at(driver);
        indices[driver] = { mean_of(dist), dist, false };
    }

    MarkovEngine engine(trainer.get_counts());
    StrategyReporter reporter(engine, indices);

    std::vector<std::pair<std::string, double>> sorted_indices(raw_indices.begin(), raw_indices.end());
    std::sort(sorted_indices.begin(), sorted_indices.end(),
        [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
            return a.second > b.second;
        });

    std::cout << "\nLeclerc, starting P3:\n  " << reporter.report_single(3, "Charles Leclerc") << "\n";

    if (!sorted_indices.empty()) {
        const std::string& strong_driver = sorted_indices.front().first;
        std::cout << "\n" << strong_driver << " (strongest index), starting P5:\n  "
                   << reporter.report_single(5, strong_driver) << "\n";
    }

    std::cout << "\nOliver Bearman (own data filtered out, <10 races), starting P10:\n  "
               << reporter.report_single(10, "Oliver Bearman") << "\n";

    std::cout << "\nVerstappen, starting P1:\n  " << reporter.report_single(1, "Max Verstappen") << "\n";

    // Bearman has <10 personal races but his team (Haas) has plenty --
    // the service should use team-level form instead of no adjustment.
    PitWallService service;
    service.load("data");
    std::string bearman_report = service.report(10, "Oliver Bearman");
    std::cout << "\nOliver Bearman via PitWallService (team-fallback expected), starting P10:\n  "
               << bearman_report << "\n";

    bool fallback_ok = (bearman_report.find("based on team-level form") != std::string::npos)
        && (bearman_report.find("No driver-specific adjustment is available for Oliver Bearman") == std::string::npos);

    if (fallback_ok) {
        std::cout << "[PASS] PitWallService applied a team-level fallback adjustment for a low-sample real driver.\n";
    }
    else {
        std::cout << "[FAIL] PitWallService did not apply the expected team-level fallback for Oliver Bearman.\n";
    }
}

void test_compare() {
    std::cout << "--- Running Strategy Reporter Compare Test ---\n";
    bool ok = true;

    // Grid 3 pooled: finish 2 x6 (60%), finish 4 x4 (40%).
    // Grid 6 pooled: finish 5 x1 (50%), finish 9 x1 (50%) -> expected 7.0.
    std::map<int, std::map<int, double>> counts;
    counts[3][2] = 6;
    counts[3][4] = 4;
    counts[6][5] = 1;
    counts[6][9] = 1;
    MarkovEngine engine(counts);

    // Only "Fast Driver" has an index; "Slow Driver" is unindexed
    // (fewer than 10 races), so it must fall back to the pooled distribution
    // and report the honest no-adjustment note.
    std::map<int, double> fast_delta = { {1, 1.0} };  // exact shift: {2,4} -> {1,3}, expected 1.8
    std::map<std::string, DriverAdjustment> indices;
    indices["Fast Driver"] = { 1.0, fast_delta, false };

    StrategyReporter reporter(engine, indices);

    std::string report = reporter.compare(3, "Fast Driver", 6, "Slow Driver");

    ok &= (report.find("Fast Driver") != std::string::npos);
    ok &= (report.find("Slow Driver") != std::string::npos);
    ok &= (report.find("P3") != std::string::npos);
    ok &= (report.find("P6") != std::string::npos);

    // Fast Driver: shifted expected finish = 1*0.6 + 3*0.4 = 1.8.
    ok &= (report.find("P1.8") != std::string::npos);
    // Slow Driver: pooled expected finish = 5*0.5 + 9*0.5 = 7.0.
    ok &= (report.find("P7.0") != std::string::npos);

    ok &= (report.find("favored") != std::string::npos);

    // Slow Driver has no index -- must get the same honest fallback text as
    // report_single, not a fabricated adjustment.
    ok &= (report.find("No driver-specific adjustment is available for Slow Driver") != std::string::npos);
    ok &= (report.find("fewer than 10 races") != std::string::npos);

    // Fast Driver has an index -- must NOT get the no-adjustment note.
    ok &= (report.find("No driver-specific adjustment is available for Fast Driver") == std::string::npos);

    // --- Team-fallback driver in a comparison gets its own labeled note,
    // distinct from both the own-data driver and the no-data driver ---
    std::map<int, double> rookie_delta = { {0, 0.5}, {1, 0.5} };
    indices["Team Rookie"] = { 0.5, rookie_delta, true };
    std::string fallback_report = reporter.compare(3, "Fast Driver", 3, "Team Rookie");
    bool fallback_ok = true;
    fallback_ok &= (fallback_report.find("Team Rookie's adjustment is based on team-level form") != std::string::npos);
    fallback_ok &= (fallback_report.find("No driver-specific adjustment is available for Team Rookie") == std::string::npos);
    ok &= fallback_ok;

    if (!fallback_ok) {
        std::cout << "[FAIL] Compare did not label the team-fallback driver correctly: " << fallback_report << "\n";
    }

    if (!ok) {
        std::cout << "[FAIL] Compare output did not match expectations: " << report << "\n";
    }
    else {
        std::cout << "[PASS] compare() reported both drivers' grids, expected finishes, an honest "
                     "no-adjustment note, and a favored-driver statement.\n";
    }
}

void test_dirichlet_finish_model() {
    std::cout << "--- Running Dirichlet Finish Model Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_dirichlet_finish_model_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // 4 races; positions used are {1, 2, 3}, so max_position == 3 and the
    // prior is exactly {1: 1, 2: 1, 3: 1}. Race 4 is a grid==0 pit-lane row.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver X", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Y", "circuit_name": "Circuit One", "position": 3, "grid": 3, "team_name": "Team One"},
            {"driver_name": "Driver X", "circuit_name": "Circuit Two", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver X", "circuit_name": "Circuit Three", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver X", "circuit_name": "Circuit Four", "position": 1, "grid": 0, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    DirichletFinishModel model(results);

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    const double lambda = 0.85;
    bool ok = true;

    ok &= (model.race_count() == 4);

    // No evidence yet -- pure Dirichlet(1,1,1) prior, uniform over positions.
    std::map<int, double> prior = model.driver_finish_distribution("Driver X", 0);
    ok &= (prior.size() == 3);
    for (int pos = 1; pos <= 3; ++pos) {
        ok &= (prior.count(pos) == 1) && (std::fabs(prior.at(pos) - (1.0 / 3.0)) < epsilon);
    }

    // Through race 2: race1 (P1) is k=1 race back, race2 (P2) is k=0 (most
    // recent) -- weighted counts {1: 1+lambda^1, 2: 1+lambda^0, 3: 1}.
    double c2_1 = 1.0 + std::pow(lambda, 1);
    double c2_2 = 1.0 + std::pow(lambda, 0);
    double c2_3 = 1.0;
    double c2_total = c2_1 + c2_2 + c2_3;

    std::map<int, double> through_2 = model.driver_finish_distribution("Driver X", 2);
    ok &= (through_2.size() == 3);
    ok &= (through_2.count(1) == 1) && (std::fabs(through_2.at(1) - c2_1 / c2_total) < epsilon);
    ok &= (through_2.count(2) == 1) && (std::fabs(through_2.at(2) - c2_2 / c2_total) < epsilon);
    ok &= (through_2.count(3) == 1) && (std::fabs(through_2.at(3) - c2_3 / c2_total) < epsilon);
    // The prior keeps this well short of 100% P1 -- the overconfidence this
    // model exists to avoid.
    ok &= (through_2.at(1) < 1.0 - epsilon);

    double sum_2 = 0.0;
    for (const auto& [pos, prob] : through_2) {
        sum_2 += prob;
    }
    ok &= (std::fabs(sum_2 - 1.0) < epsilon);

    // Through race 3: race1 (P1) is k=2, race2 (P2) is k=1, race3 (P1) is
    // k=0 -- P1 now carries two weighted observations, one of them the most
    // recent race.
    double c3_1 = 1.0 + std::pow(lambda, 2) + std::pow(lambda, 0);
    double c3_2 = 1.0 + std::pow(lambda, 1);
    double c3_3 = 1.0;
    double c3_total = c3_1 + c3_2 + c3_3;

    std::map<int, double> through_3 = model.driver_finish_distribution("Driver X", 3);
    ok &= (through_3.size() == 3);
    ok &= (through_3.count(1) == 1) && (std::fabs(through_3.at(1) - c3_1 / c3_total) < epsilon);
    ok &= (through_3.count(2) == 1) && (std::fabs(through_3.at(2) - c3_2 / c3_total) < epsilon);
    ok &= (through_3.count(3) == 1) && (std::fabs(through_3.at(3) - c3_3 / c3_total) < epsilon);
    // Must differ from through_2, proving through_race==2 genuinely
    // excluded race3 (and that its weights actually recomputed, not just
    // reused a cached ratio).
    ok &= (through_3 != through_2);

    // race4 (grid==0) adds no evidence, but the cutoff still advances, so
    // race1..race3's existing evidence decays one step further regardless.
    double c4_1 = 1.0 + std::pow(lambda, 3) + std::pow(lambda, 1);
    double c4_2 = 1.0 + std::pow(lambda, 2);
    double c4_3 = 1.0;
    double c4_total = c4_1 + c4_2 + c4_3;

    std::map<int, double> through_4 = model.driver_finish_distribution("Driver X", 4);
    ok &= (through_4.size() == 3);
    ok &= (through_4.count(1) == 1) && (std::fabs(through_4.at(1) - c4_1 / c4_total) < epsilon);
    ok &= (through_4.count(2) == 1) && (std::fabs(through_4.at(2) - c4_2 / c4_total) < epsilon);
    ok &= (through_4.count(3) == 1) && (std::fabs(through_4.at(3) - c4_3 / c4_total) < epsilon);
    // P1's mass should have decayed relative to through_3, since its
    // evidence is now one race further in the past with nothing new to
    // offset it.
    ok &= (through_4.at(1) < through_3.at(1));

    // Unknown driver -> empty map, no fabricated distribution.
    std::map<int, double> unknown = model.driver_finish_distribution("Nobody", 4);
    ok &= unknown.empty();

    if (ok) {
        std::cout << "[PASS] DirichletFinishModel matched hand-calculated recency-weighted posteriors, excluded "
                     "future races, skipped grid==0 evidence, and returned no data for an unknown driver.\n";
    }
    else {
        std::cout << "[FAIL] DirichletFinishModel output did not match expectations.\n";
    }
}

void test_dirichlet_recency_weighting() {
    std::cout << "--- Running Dirichlet Recency Weighting Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_dirichlet_recency_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Driver Z improves steadily from P5 to a win by race 4 -- equal
    // weighting would underrate this arc relative to early-season form.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver Z", "circuit_name": "Circuit One", "position": 5, "grid": 5, "team_name": "Team One"},
            {"driver_name": "Driver Z", "circuit_name": "Circuit Two", "position": 4, "grid": 4, "team_name": "Team One"},
            {"driver_name": "Driver Z", "circuit_name": "Circuit Three", "position": 3, "grid": 3, "team_name": "Team One"},
            {"driver_name": "Driver Z", "circuit_name": "Circuit Four", "position": 1, "grid": 1, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    DirichletFinishModel model(results);
    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    const double lambda = 0.85;
    bool ok = true;

    std::map<int, double> through_4 = model.driver_finish_distribution("Driver Z", 4);
    ok &= (through_4.size() == 5);

    // Hand-computed evidence: race r (1-based) contributes lambda^(4-r) to
    // its finishing position, added on top of the Dirichlet(1,...,1) prior.
    std::map<int, double> expected_counts;
    for (int pos = 1; pos <= 5; ++pos) {
        expected_counts[pos] = 1.0;  // prior
    }
    expected_counts[5] += std::pow(lambda, 4 - 1);  // race 1 -> P5, k=3
    expected_counts[4] += std::pow(lambda, 4 - 2);  // race 2 -> P4, k=2
    expected_counts[3] += std::pow(lambda, 4 - 3);  // race 3 -> P3, k=1
    expected_counts[1] += std::pow(lambda, 4 - 4);  // race 4 -> P1, k=0

    double expected_total = 0.0;
    for (const auto& [pos, count] : expected_counts) {
        expected_total += count;
    }

    for (int pos = 1; pos <= 5; ++pos) {
        double expected_prob = expected_counts.at(pos) / expected_total;
        ok &= (through_4.count(pos) == 1) && (std::fabs(through_4.at(pos) - expected_prob) < epsilon);
    }

    double sum = 0.0;
    for (const auto& [pos, prob] : through_4) {
        sum += prob;
    }
    ok &= (std::fabs(sum - 1.0) < epsilon);

    // The most recent finish (P1, race 4, k=0) must outweigh the oldest,
    // worst finish (P5, race 1, k=3) even though both are single
    // observations -- recency dominates raw frequency.
    ok &= (through_4.at(1) > through_4.at(5));

    // Decay is monotonic with recency: P1 (k=0) > P3 (k=1) > P4 (k=2) >
    // P5 (k=3), each one race further in the past.
    ok &= (through_4.at(1) > through_4.at(3));
    ok &= (through_4.at(3) > through_4.at(4));
    ok &= (through_4.at(4) > through_4.at(5));

    if (ok) {
        std::cout << "[PASS] DirichletFinishModel weighted evidence by exponential recency (lambda=0.85), matched "
                     "hand-computed weighted counts, and let a recent win outweigh an older, worse finish.\n";
    }
    else {
        std::cout << "[FAIL] DirichletFinishModel recency weighting did not match expectations.\n";
    }
}

void test_championship_simulator() {
    std::cout << "--- Running Championship Simulator Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_championship_simulator_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Driver A always P1, Driver B always P2, simulated from race 10 with
    // enough remaining points that clinch/elimination can't short-circuit.
    // Dominance is checked as "overwhelming", not exact 1.0 (Dirichlet prior).
    {
        std::ostringstream rows;
        for (int race = 1; race <= 20; ++race) {
            std::string circuit = "Circuit " + std::to_string(race);
            rows << R"({"driver_name": "Driver A", "circuit_name": ")" << circuit << R"(", "position": 1, "grid": 1, "team_name": "Team One"},)";
            rows << R"({"driver_name": "Driver B", "circuit_name": ")" << circuit << R"(", "position": 2, "grid": 2, "team_name": "Team One"})";
            if (race != 20) {
                rows << ",";
            }
        }
        std::ofstream f(results_path);
        f << "[" << rows.str() << "]";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);

    const double epsilon = 1e-9;
    bool ok = true;

    const unsigned int seed = 12345;
    ChampionshipSimulator sim1(results, seed);
    std::map<std::string, double> probs = sim1.simulate_championship(10, 1000);

    ok &= (probs.size() == 2);
    ok &= (probs.count("Driver A") == 1) && (probs.at("Driver A") > 0.95);
    ok &= (probs.count("Driver B") == 1) && (probs.at("Driver B") < 0.05);

    double sum = 0.0;
    for (const auto& [driver, prob] : probs) {
        sum += prob;
    }
    ok &= (std::fabs(sum - 1.0) < epsilon);

    // Determinism: a fresh simulator built from the same data and the same
    // seed must reproduce the exact same result.
    ChampionshipSimulator sim2(results, seed);
    std::map<std::string, double> probs_repeat = sim2.simulate_championship(10, 1000);
    ok &= (probs_repeat == probs);

    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] ChampionshipSimulator made the historically dominant driver an overwhelming favorite, "
                     "probabilities summed to 1.0, and results were reproducible under a fixed seed.\n";
    }
    else {
        std::cout << "[FAIL] ChampionshipSimulator output did not match expectations "
                     "(Driver A: " << probs["Driver A"] << ", Driver B: " << probs["Driver B"] << ").\n";
    }
}

void test_championship_clinch() {
    std::cout << "--- Running Championship Clinch Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_championship_clinch_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Driver A leads by 50 pts after 2 races; only 25 pts remain (1 race),
    // so the leader has mathematically clinched before any simulation runs.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit One", "position": 11, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Two", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit Two", "position": 11, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Three", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit Three", "position": 2, "grid": 2, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);

    const double epsilon = 1e-9;
    bool ok = true;

    // Two different seeds: if the result depends on the seed at all, the
    // clinch constraint isn't actually short-circuiting the RNG.
    ChampionshipSimulator sim_a(results, /*seed=*/1);
    ChampionshipSimulator sim_b(results, /*seed=*/999);

    std::map<std::string, double> probs_a = sim_a.simulate_championship(2, 1000);
    std::map<std::string, double> probs_b = sim_b.simulate_championship(2, 1000);

    ok &= (probs_a.count("Driver A") == 1) && (std::fabs(probs_a.at("Driver A") - 1.0) < epsilon);
    ok &= (probs_a.count("Driver B") == 1) && (std::fabs(probs_a.at("Driver B") - 0.0) < epsilon);
    ok &= (probs_a == probs_b);

    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] Clinched leader returned 1.0 without depending on the RNG seed.\n";
    }
    else {
        std::cout << "[FAIL] Clinch constraint did not resolve the season deterministically.\n";
    }
}

void test_championship_elimination() {
    std::cout << "--- Running Championship Elimination Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_championship_elimination_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // A and B are tied at 43 pts; C has 0. Only 25 pts remain, so C is
    // mathematically eliminated even though A vs B is still wide open.
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit One", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver C", "circuit_name": "Circuit One", "position": 11, "grid": 3, "team_name": "Team One"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Two", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit Two", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver C", "circuit_name": "Circuit Two", "position": 11, "grid": 3, "team_name": "Team One"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Three", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit Three", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver C", "circuit_name": "Circuit Three", "position": 3, "grid": 3, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    ChampionshipSimulator sim(results, /*seed=*/7);
    std::map<std::string, double> probs = sim.simulate_championship(2, 1000);

    const double epsilon = 1e-9;
    bool ok = true;

    ok &= (probs.count("Driver C") == 1) && (std::fabs(probs.at("Driver C") - 0.0) < epsilon);

    double sum = 0.0;
    for (const auto& [driver, prob] : probs) {
        sum += prob;
    }
    ok &= (std::fabs(sum - 1.0) < epsilon);

    // The title fight between A and B must still be genuinely open --
    // confirms this hit the real simulation path, not an accidental clinch.
    ok &= (probs.at("Driver A") > 0.0) && (probs.at("Driver B") > 0.0);

    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] Mathematically eliminated non-leader scored exactly 0.0 while the real contest stayed open.\n";
    }
    else {
        std::cout << "[FAIL] Elimination constraint did not zero out the eliminated driver correctly.\n";
    }
}

void test_championship_points_through_race() {
    std::cout << "--- Running Championship Points-Through-Race Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_championship_points_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Race 1 (Circuit One): Driver A P1 (25 pts), Driver B P2 (18 pts).
    // Race 2 (Circuit Two): Driver A P3 (15 pts), Driver B P1 (25 pts).
    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver A", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit One", "position": 2, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver A", "circuit_name": "Circuit Two", "position": 3, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver B", "circuit_name": "Circuit Two", "position": 1, "grid": 2, "team_name": "Team One"}
        ])";
    }

    std::vector<ResultRow> results = load_test_results(temp_dir);
    ChampionshipSimulator sim(results);

    bool ok = true;
    ok &= (sim.race_count() == 2);

    std::map<std::string, double> p0 = sim.points_through_race(0);
    ok &= (p0.at("Driver A") == 0.0);
    ok &= (p0.at("Driver B") == 0.0);

    std::map<std::string, double> p1 = sim.points_through_race(1);
    ok &= (p1.at("Driver A") == 25.0);
    ok &= (p1.at("Driver B") == 18.0);

    std::map<std::string, double> p2 = sim.points_through_race(2);
    ok &= (p2.at("Driver A") == 40.0);
    ok &= (p2.at("Driver B") == 43.0);

    fs::remove_all(temp_dir);

    if (ok) {
        std::cout << "[PASS] points_through_race matched the hand-calculated standings at each split point.\n";
    }
    else {
        std::cout << "[FAIL] points_through_race did not match the hand-calculated standings.\n";
    }
}

void test_championship_real() {
    std::cout << "--- Running Real Data Championship Simulator Smoke Test ---\n";

    std::vector<ResultRow> real_results = load_test_results(std::filesystem::path(resolve_repo_path("data")));
    ChampionshipSimulator sim(real_results, /*seed=*/42);

    auto print_top8 = [](const std::string& label, const std::map<std::string, double>& probs) {
        std::vector<std::pair<std::string, double>> sorted(probs.begin(), probs.end());
        std::sort(sorted.begin(), sorted.end(),
            [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
                return a.second > b.second;
            });
        std::cout << label << ":\n";
        for (size_t i = 0; i < sorted.size() && i < 8; ++i) {
            std::cout << "  " << (i + 1) << ". " << sorted[i].first
                       << " : " << (sorted[i].second * 100.0) << "%\n";
        }
    };

    int total_races = sim.race_count();
    std::cout << "Season has " << total_races << " races.\n\n";

    std::map<std::string, double> full_season = sim.simulate_championship(0, 10000);
    print_top8("Title probability, simulated from race 0 (entire season)", full_season);

    int mid_race = total_races / 2;
    std::cout << "\n";
    std::map<std::string, double> mid_season = sim.simulate_championship(mid_race, 10000);
    print_top8("Title probability, simulated from race " + std::to_string(mid_race) + " (mid-season split)", mid_season);
}

void test_championship_calibration_2024_real() {
    std::cout << "--- Running Championship Calibration Test (Real 2024 Data) ---\n";

    std::vector<ResultRow> results = load_test_results(std::filesystem::path(resolve_repo_path("data")));
    ChampionshipSimulator sim(results, /*seed=*/42);

    const std::string champion = "Max Verstappen";  // real 2024 champion
    const int num_simulations = 10000;

    // Checkpoints spanning the season -- the same 20/40/60/80/95% shape
    // main.cpp's demo uses for a 24-race season.
    std::vector<int> checkpoints = { 5, 10, 14, 19, 23 };
    std::vector<double> champion_probs;
    std::map<std::string, double> final_probs;

    for (int checkpoint : checkpoints) {
        std::map<std::string, double> probs = sim.simulate_championship(checkpoint, num_simulations);
        champion_probs.push_back(probs.count(champion) ? probs.at(champion) : 0.0);
        if (checkpoint == checkpoints.back()) {
            final_probs = probs;
        }
    }

    bool ok = true;

    // Honest early uncertainty: even a dominant driver hasn't clinched
    // anything after only 5 of 24 races.
    ok &= (champion_probs[0] < 0.70);

    // Real mid-season dip (race 14->19, McLaren's documented late-season
    // surge), not a manufactured march to 100% -- proves the model tracks
    // real signal instead of smoothing toward its eventual answer.
    ok &= (champion_probs[3] < champion_probs[2]);

    // Real conviction by the second-to-last checkpoint, not just "the
    // season is basically over."
    ok &= (champion_probs[3] > 0.85);

    // The model's own favorite, not just Verstappen's raw share, must be
    // the real champion -- proof it picks the right driver outright.
    double top_prob = -1.0;
    std::string top_driver;
    for (const auto& [driver, prob] : final_probs) {
        if (prob > top_prob) {
            top_prob = prob;
            top_driver = driver;
        }
    }
    ok &= (top_driver == champion);

    if (ok) {
        std::cout << "[PASS] Championship model stayed honestly uncertain early, tracked a real mid-season form dip "
                     "instead of smoothing over it, and correctly converged on Max Verstappen as the outright favorite.\n";
    }
    else {
        std::cout << "[FAIL] Championship calibration against the real 2024 season did not match expectations.\n";
    }
}

void test_championship_calibration_2012_real() {
    std::cout << "--- Running Championship Calibration Test (Real 2012 Data -- Closest Title Fight in F1 History) ---\n";

    std::vector<ResultRow> results = load_test_results(std::filesystem::path(resolve_repo_path("data_2012")));
    ChampionshipSimulator sim(results, /*seed=*/42);

    const std::string champion = "Sebastian Vettel";  // real 2012 champion, by 3 points
    const int num_simulations = 10000;

    std::vector<int> checkpoints = { 4, 8, 12, 16, 19 };
    std::vector<double> champion_probs;
    std::map<std::string, double> final_probs;

    for (int checkpoint : checkpoints) {
        std::map<std::string, double> probs = sim.simulate_championship(checkpoint, num_simulations);
        champion_probs.push_back(probs.count(champion) ? probs.at(champion) : 0.0);
        if (checkpoint == checkpoints.back()) {
            final_probs = probs;
        }
    }

    bool ok = true;

    // The real fight went to the final race by 3 points; false certainty
    // this early would mean the model isn't reading how close it was.
    ok &= (champion_probs[0] < 0.50);

    // Vettel's own odds genuinely fall from race 4 to race 8 -- real
    // mid-season form, not noise to smooth away.
    ok &= (champion_probs[1] < champion_probs[0]);

    // Conviction stays well short of 2024's blowout-level certainty --
    // confidence should track how close the real fight actually was.
    ok &= (champion_probs[4] < 0.95);

    // Still must get the outright favorite right, even in the closest
    // title fight in F1 history -- the actual proof, not just a decent number.
    double top_prob = -1.0;
    std::string top_driver;
    for (const auto& [driver, prob] : final_probs) {
        if (prob > top_prob) {
            top_prob = prob;
            top_driver = driver;
        }
    }
    ok &= (top_driver == champion);

    if (ok) {
        std::cout << "[PASS] Championship model showed appropriately deep uncertainty through a genuinely close "
                     "title fight, never overclaimed blowout-level certainty, and still correctly converged on "
                     "Sebastian Vettel as the outright favorite.\n";
    }
    else {
        std::cout << "[FAIL] Championship calibration against the real 2012 season did not match expectations.\n";
    }
}