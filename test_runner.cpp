#include "graph.h"
#include "storage.h"
#include "graph_repository.h"
#include "data_importer.h"
#include "markov_trainer.h"
#include "markov_engine.h"
#include "strategy_reporter.h"
#include "service.h"
#include "championship_simulator.h"
#include "dirichlet_finish_model.h"
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <sstream>

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

    Edge stats = {
        0.15,  // winRate
    };

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

    Graph g;
    DataImporter importer(g);
    importer.import_drivers(drivers_path.string());
    importer.import_teams(teams_path.string());
    importer.import_circuits(circuits_path.string());
    importer.import_results(results_path.string());

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

    Graph g;
    DataImporter importer(g);

    importer.import_drivers("data/drivers.json");
    importer.import_teams("data/teams.json");
    importer.import_circuits("data/circuits.json");
    importer.import_results("data/results.json");

    std::cout << "Total nodes: " << g.count_vertices() << "\n";
    std::cout << "Total edges: " << g.count_edges() << "\n";

    for (int id : g.get_all_node_ids()) {
        auto node = g.get_node(id);
        std::cout << "  [" << id << "] " << node->get_name() << " (" << node->get_type_string() << ")\n";
    }

    bool can_persist = true;
    try {
        std::filesystem::remove("pitwall_f1.db");
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cout << "[WARN] Could not remove pitwall_f1.db (" << e.what() << "); skipping persistence step.\n";
        can_persist = false;
    }

    if (can_persist) {
        Storage storage("pitwall_f1.db");
        GraphRepository repo(storage);
        repo.create_tables();
        repo.save_graph(g);
        std::cout << "Saved graph to pitwall_f1.db\n";
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

    MarkovTrainer trainer;
    trainer.train(results_path.string());

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

void test_markov_engine() {
    std::cout << "--- Running MarkovEngine Test ---\n";

    std::map<int, std::map<int, int>> counts;
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

    MarkovTrainer trainer;
    trainer.train("data/results.json");

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
        // Alpha: exactly 10 valid (grid != 0) rows, each (grid - finish) = 3 -> mean = 3.0.
        // Meets the >= 10 valid-row threshold, so it must appear in the result.
        for (int i = 0; i < 10; ++i) {
            rows << R"({"driver_name": "Alpha", "circuit_name": "Circuit One", "position": 2, "grid": 5, "team_name": "Team One"},)";
        }
        // Beta: 2 valid rows -> below the threshold -> must be omitted entirely.
        rows << R"({"driver_name": "Beta", "circuit_name": "Circuit One", "position": 6, "grid": 3, "team_name": "Team Two"},)";
        rows << R"({"driver_name": "Beta", "circuit_name": "Circuit Two", "position": 2, "grid": 1, "team_name": "Team Two"},)";
        // Gamma: only a grid==0 row -> zero valid rows -> must be omitted entirely.
        rows << R"({"driver_name": "Gamma", "circuit_name": "Circuit One", "position": 9, "grid": 0, "team_name": "Team Three"})";

        std::ofstream f(results_path);
        f << "[" << rows.str() << "]";
    }

    MarkovTrainer trainer;
    std::map<std::string, double> indices = trainer.compute_driver_indices(results_path.string());

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    bool ok = true;

    // Alpha: 10 valid rows, each contributing 3 -> mean = 3.0, meets the threshold.
    ok &= (indices.count("Alpha") == 1);
    ok &= (std::fabs(indices.at("Alpha") - 3.0) < epsilon);

    // Beta: only 2 valid rows -> below the 10-row minimum -> omitted from the map.
    ok &= (indices.count("Beta") == 0);

    // Gamma: zero valid rows -> below the 10-row minimum -> omitted from the map.
    ok &= (indices.count("Gamma") == 0);

    // Only drivers meeting the threshold are present at all.
    ok &= (indices.size() == 1);

    if (ok) {
        std::cout << "[PASS] compute_driver_indices computed correct means and filtered grid==0 rows.\n";
    }
    else {
        std::cout << "[FAIL] compute_driver_indices output did not match expectations.\n";
    }
}

void test_driver_aware_prediction() {
    std::cout << "--- Running Driver-Aware Prediction Test ---\n";

    std::map<int, std::map<int, int>> counts;
    // grid 1: finish 1 x2, finish 2 x2, finish 3 x4 (total 8)
    counts[1][1] = 2;
    counts[1][2] = 2;
    counts[1][3] = 4;

    MarkovEngine engine(counts);
    const double epsilon = 1e-9;
    bool ok = true;

    // Pooled: {1: 0.25, 2: 0.25, 3: 0.5}
    // driver_index = +1.0 is an exact integer shift (no fractional split):
    // finish 1 -> target 0 -> clamped to 1, finish 2 -> target 1 -> 1,
    // finish 3 -> target 2 -> 2
    // Result: {1: 0.5, 2: 0.5}
    std::map<int, double> shifted = engine.predict_finish_distribution_for_driver(1, 1.0);

    ok &= (shifted.size() == 2);
    ok &= (shifted.count(1) == 1);
    ok &= (std::fabs(shifted.at(1) - 0.5) < epsilon);
    ok &= (shifted.count(2) == 1);
    ok &= (std::fabs(shifted.at(2) - 0.5) < epsilon);

    double sum = 0.0;
    for (const auto& [finish, prob] : shifted) {
        ok &= (finish >= 1);
        sum += prob;
    }
    ok &= (std::fabs(sum - 1.0) < epsilon);

    // Large positive index clamps everything to P1.
    std::map<int, double> fully_clamped = engine.predict_finish_distribution_for_driver(1, 10.0);
    ok &= (fully_clamped.size() == 1);
    ok &= (fully_clamped.count(1) == 1);
    ok &= (std::fabs(fully_clamped.at(1) - 1.0) < epsilon);

    // Unseen grid position returns an empty map, no throw.
    std::map<int, double> unseen = engine.predict_finish_distribution_for_driver(99, 2.0);
    ok &= unseen.empty();

    // --- Fractional shift with mass interpolation ---
    std::map<int, std::map<int, int>> counts2;
    // grid 5: finish 2 x2 (0.2), finish 5 x3 (0.3), finish 8 x5 (0.5), total 10
    counts2[5][2] = 2;
    counts2[5][5] = 3;
    counts2[5][8] = 5;

    MarkovEngine engine2(counts2);

    // Pooled: {2: 0.2, 5: 0.3, 8: 0.5}, max_finish = 8, driver_index = 2.4
    // finish 2: target = 2 - 2.4 = -0.4 -> lo=-1, hi=0, both clamp to 1
    //           -> shifted[1] += 0.2 * 1.0 = 0.2
    // finish 5: target = 5 - 2.4 = 2.6 -> lo=2, hi=3, no clamping needed
    //           -> shifted[2] += 0.3 * 0.4 = 0.12, shifted[3] += 0.3 * 0.6 = 0.18
    // finish 8: target = 8 - 2.4 = 5.6 -> lo=5, hi=6, no clamping needed
    //           -> shifted[5] += 0.5 * 0.4 = 0.2, shifted[6] += 0.5 * 0.6 = 0.3
    // Result: {1: 0.2, 2: 0.12, 3: 0.18, 5: 0.2, 6: 0.3}
    std::map<int, double> fractional = engine2.predict_finish_distribution_for_driver(5, 2.4);

    ok &= (fractional.size() == 5);
    ok &= (fractional.count(1) == 1) && (std::fabs(fractional.at(1) - 0.2) < epsilon);
    ok &= (fractional.count(2) == 1) && (std::fabs(fractional.at(2) - 0.12) < epsilon);
    ok &= (fractional.count(3) == 1) && (std::fabs(fractional.at(3) - 0.18) < epsilon);
    ok &= (fractional.count(5) == 1) && (std::fabs(fractional.at(5) - 0.2) < epsilon);
    ok &= (fractional.count(6) == 1) && (std::fabs(fractional.at(6) - 0.3) < epsilon);

    double fractional_sum = 0.0;
    for (const auto& [finish, prob] : fractional) {
        ok &= (finish >= 1 && finish <= 8);
        fractional_sum += prob;
    }
    ok &= (std::fabs(fractional_sum - 1.0) < epsilon);

    // A fractional index must NOT collapse to the same result as its rounded
    // integer counterpart -- this is the whole point of dropping std::llround.
    // On the grid-1 pooled distribution {1: 0.25, 2: 0.25, 3: 0.5}:
    //   index 1.5 -> finish 1: target -0.5 -> clamps to 1 (mass 0.25)
    //                finish 2: target  0.5 -> lo=0/hi=1, both clamp to 1 (mass 0.25)
    //                finish 3: target  1.5 -> lo=1/hi=2, split 0.5/0.5 (mass 0.5)
    //                Result: {1: 0.75, 2: 0.25}
    //   index 2.0 -> every target is an exact integer <= 1 -> fully clamped to P1
    //                Result: {1: 1.0}
    // Under the old std::llround-based shift both 1.5 and 2.0 rounded to the
    // same integer shift and produced an identical distribution; they must
    // now differ.
    std::map<int, double> frac_index = engine.predict_finish_distribution_for_driver(1, 1.5);
    std::map<int, double> rounded_index = engine.predict_finish_distribution_for_driver(1, 2.0);

    ok &= (frac_index.size() == 2);
    ok &= (frac_index.count(1) == 1) && (std::fabs(frac_index.at(1) - 0.75) < epsilon);
    ok &= (frac_index.count(2) == 1) && (std::fabs(frac_index.at(2) - 0.25) < epsilon);

    ok &= (rounded_index.size() == 1);
    ok &= (rounded_index.count(1) == 1) && (std::fabs(rounded_index.at(1) - 1.0) < epsilon);

    ok &= (frac_index != rounded_index);

    if (ok) {
        std::cout << "[PASS] predict_finish_distribution_for_driver split fractional mass correctly, "
                     "respected the P1 clamp, conserved total probability, and diverged from its rounded counterpart.\n";
    }
    else {
        std::cout << "[FAIL] predict_finish_distribution_for_driver output did not match expectations.\n";
    }
}

void test_driver_index_real() {
    std::cout << "--- Running Real Data Driver Index Smoke Test ---\n";

    MarkovTrainer trainer;
    trainer.train("data/results.json");
    std::map<std::string, double> indices = trainer.compute_driver_indices("data/results.json");

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

    // Part C: run the same real data through the full PitWallService pipeline
    // (import, train, write indices into the graph, build the reporter) and
    // confirm the strong/weak drivers' reports reflect their computed indices.
    // This is the same integration point the old apply_driver_indices helper
    // covered directly; it now lives in the service instead of being
    // duplicated here.
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
    std::map<int, std::map<int, int>> counts;
    counts[5][4] = 5;
    counts[5][6] = 3;
    counts[5][8] = 2;
    MarkovEngine engine(counts);

    std::map<std::string, double> indices;
    indices["Test Driver"] = 1.85; // rounds to 1.9

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

    // Driver adjustment: index rounded to 1 decimal, and the actual
    // shifted most-likely finish computed via predict_finish_distribution_for_driver
    // (pooled {4:0.5,6:0.3,8:0.2} shifted by 1.85 -> most mass, 0.425, lands on P2).
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

    MarkovTrainer trainer;
    trainer.train("data/results.json");
    std::map<std::string, double> indices = trainer.compute_driver_indices("data/results.json");
    MarkovEngine engine(trainer.get_counts());
    StrategyReporter reporter(engine, indices);

    std::vector<std::pair<std::string, double>> sorted_indices(indices.begin(), indices.end());
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

    std::cout << "\nOliver Bearman (filtered out, <10 races), starting P10:\n  "
               << reporter.report_single(10, "Oliver Bearman") << "\n";

    std::cout << "\nVerstappen, starting P1:\n  " << reporter.report_single(1, "Max Verstappen") << "\n";
}

void test_dirichlet_finish_model() {
    std::cout << "--- Running Dirichlet Finish Model Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_dirichlet_finish_model_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // Race order: Circuit One (race1), Circuit Two (race2), Circuit Three
    // (race3), Circuit Four (race4). Positions used anywhere in the file are
    // {1, 2, 3}, except a single grid==0 pit-lane row (see below), so
    // max_position == 3 and the prior is exactly {1: 1, 2: 1, 3: 1}.
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

    DirichletFinishModel model(results_path.string());

    fs::remove_all(temp_dir);

    const double epsilon = 1e-9;
    bool ok = true;

    ok &= (model.race_count() == 4);

    // through_race == 0: no evidence at all -- pure Dirichlet(1,1,1) prior,
    // uniform over the 3 possible finishing positions.
    std::map<int, double> prior = model.driver_finish_distribution("Driver X", 0);
    ok &= (prior.size() == 3);
    for (int pos = 1; pos <= 3; ++pos) {
        ok &= (prior.count(pos) == 1) && (std::fabs(prior.at(pos) - (1.0 / 3.0)) < epsilon);
    }

    // through_race == 2: 1 real win (race1, P1) and 1 non-win (race2, P2).
    // Counts: prior {1:1,2:1,3:1} + evidence {1:+1,2:+1} = {1:2,2:2,3:1},
    // sum 5 -> {1: 0.4, 2: 0.4, 3: 0.2}. Even after a win, the prior keeps
    // this well short of 100% P1 -- exactly the overconfidence this model
    // exists to avoid.
    std::map<int, double> through_2 = model.driver_finish_distribution("Driver X", 2);
    ok &= (through_2.size() == 3);
    ok &= (through_2.count(1) == 1) && (std::fabs(through_2.at(1) - 0.4) < epsilon);
    ok &= (through_2.count(2) == 1) && (std::fabs(through_2.at(2) - 0.4) < epsilon);
    ok &= (through_2.count(3) == 1) && (std::fabs(through_2.at(3) - 0.2) < epsilon);
    ok &= (through_2.at(1) < 1.0 - epsilon);

    double sum_2 = 0.0;
    for (const auto& [pos, prob] : through_2) {
        sum_2 += prob;
    }
    ok &= (std::fabs(sum_2 - 1.0) < epsilon);

    // through_race == 3: race3 (P1) is now in scope and must be counted.
    // Counts: {1:2,2:2,3:1} + {1:+1} = {1:3,2:2,3:1}, sum 6
    // -> {1: 0.5, 2: 1/3, 3: 1/6}. Must differ from through_2 above -- this
    // proves through_race==2 genuinely excluded race3's result rather than
    // happening to match by coincidence.
    std::map<int, double> through_3 = model.driver_finish_distribution("Driver X", 3);
    ok &= (through_3.size() == 3);
    ok &= (through_3.count(1) == 1) && (std::fabs(through_3.at(1) - 0.5) < epsilon);
    ok &= (through_3.count(2) == 1) && (std::fabs(through_3.at(2) - (1.0 / 3.0)) < epsilon);
    ok &= (through_3.count(3) == 1) && (std::fabs(through_3.at(3) - (1.0 / 6.0)) < epsilon);
    ok &= (through_3 != through_2);

    // through_race == 4: race4 is a grid==0 pit-lane row for Driver X and
    // must be skipped for evidence, consistent with existing grid==0
    // filters elsewhere -- so this must equal through_3 exactly.
    std::map<int, double> through_4 = model.driver_finish_distribution("Driver X", 4);
    ok &= (through_4 == through_3);

    // Unknown driver -> empty map, no fabricated distribution.
    std::map<int, double> unknown = model.driver_finish_distribution("Nobody", 4);
    ok &= unknown.empty();

    if (ok) {
        std::cout << "[PASS] DirichletFinishModel matched hand-calculated posteriors, excluded future races, "
                     "skipped grid==0 evidence, and returned no data for an unknown driver.\n";
    }
    else {
        std::cout << "[FAIL] DirichletFinishModel output did not match expectations.\n";
    }
}

void test_championship_simulator() {
    std::cout << "--- Running Championship Simulator Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_championship_simulator_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    // 20 races, two drivers: Driver A always finishes P1, Driver B always
    // finishes P2. Simulating from race 10 (10 real races locked in, 10
    // remaining) means Driver A's Dirichlet posterior through race 10 is
    // built from a real 10-0 record -- heavily favoring P1 -- while still
    // leaving enough remaining points (10 races x 25 max) that the
    // clinch/elimination shortcut can't resolve this outright; the win
    // probability below has to come from the real sample-then-rank RNG path.
    // A Dirichlet prior always keeps a little mass on every position for a
    // single race -- unlike the old rig, which pooled a distribution with
    // zero mass anywhere else -- but that residual mass is thin enough here
    // that it may not even show up over 1000 simulated 10-race seasons; the
    // assertions below check for overwhelming dominance, not the exact 1.0
    // the old engine-based rig guaranteed structurally.
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

    std::map<int, std::map<int, int>> counts;  // unused: the default path no longer consults the engine
    MarkovEngine engine(counts);

    const double epsilon = 1e-9;
    bool ok = true;

    const unsigned int seed = 12345;
    ChampionshipSimulator sim1(engine, results_path.string(), seed);
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
    ChampionshipSimulator sim2(engine, results_path.string(), seed);
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

    // Driver A wins races 1 and 2 outright (50 pts); Driver B scores 0 in
    // both. Race 3 (Circuit Three) exists only so race_count() == 3 -- one
    // race remains, worth at most 25 points, which can't close a 50-point
    // gap. The leader has mathematically clinched before any simulation runs.
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

    std::map<int, std::map<int, int>> counts;  // unused: the clinch check returns before touching the engine
    MarkovEngine engine(counts);

    const double epsilon = 1e-9;
    bool ok = true;

    // Two different seeds: if the result depends on the seed at all, the
    // clinch constraint isn't actually short-circuiting the RNG.
    ChampionshipSimulator sim_a(engine, results_path.string(), /*seed=*/1);
    ChampionshipSimulator sim_b(engine, results_path.string(), /*seed=*/999);

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

    // After 2 races, Driver A and Driver B are tied for the lead at 43 pts
    // each; Driver C has 0. One race remains (max 25 pts), so C's best
    // possible total (25) can never reach 43 -- mathematically eliminated,
    // even though the title fight between A and B is still wide open.
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

    std::map<int, std::map<int, int>> counts;
    counts[1][1] = 1; counts[1][2] = 1;  // grid 1 (Driver A) -> P1 or P2, 50/50
    counts[2][1] = 1; counts[2][2] = 1;  // grid 2 (Driver B) -> P1 or P2, 50/50
    counts[3][1] = 1;                    // grid 3 (Driver C) -> always P1 (best case; still not enough)
    MarkovEngine engine(counts);

    ChampionshipSimulator sim(engine, results_path.string(), /*seed=*/7);
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

    std::map<int, std::map<int, int>> counts;  // unused by this test, engine still required to construct
    MarkovEngine engine(counts);
    ChampionshipSimulator sim(engine, results_path.string());

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

    MarkovTrainer trainer;
    trainer.train("data/results.json");
    MarkovEngine engine(trainer.get_counts());
    ChampionshipSimulator sim(engine, "data/results.json", /*seed=*/42);

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