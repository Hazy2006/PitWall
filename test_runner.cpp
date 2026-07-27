#include "graph.h"
#include "storage.h"
#include "graph_repository.h"
#include "data_importer.h"
#include "markov_trainer.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>

void run_domain_tests() {
    Graph g;

    auto leclerc = std::make_shared<DriverNode>("Leclerc", 0.87, -0.2);
    auto ferrari = std::make_shared<TeamNode>("Ferrari", 2.1);

    int id1 = g.add_node(leclerc);
    int id2 = g.add_node(ferrari);

    assert(id1 == 0);
    assert(id2 == 1);
    assert(g.count_vertices() == 2);

    assert(g.get_node(id1)->get_name() == "Leclerc");
    assert(g.get_node(id1)->get_type_string() == "Driver");
    assert(g.get_node(id2)->get_type_string() == "Team");

    std::cout << "Polymorphic Domain Graph Tests Passed!\n";
}

void run_edge_tests() {
    Graph g;

    auto leclerc = std::make_shared<DriverNode>("Leclerc", 0.87, -0.2);
    auto monaco = std::make_shared<CircuitNode>("Monaco", 1.2);

    int driver_id = g.add_node(leclerc);
    int circuit_id = g.add_node(monaco);

    Edge stats = {
        0.15,  // winRate
        0.90,  // affinityScore
        -0.1,  // performanceDelta
        1.0    // tirePreference
    };

    g.add_edge(driver_id, circuit_id, stats);

    assert(g.count_edges() == 1);
    assert(g.is_edge(driver_id, circuit_id) == true);
    assert(g.is_edge(circuit_id, driver_id) == false);

    Edge retrieved = g.get_edge(driver_id, circuit_id);
    assert(retrieved.affinityScore == 0.90);

    std::cout << "Edge Routing Tests Passed!\n";
}

void test_bfs() {
    std::cout << "--- Running BFS Test ---\n";

    Graph g;
    int id1 = g.add_node(std::make_shared<DriverNode>("Node1", 0.0, 0.0));
    int id2 = g.add_node(std::make_shared<DriverNode>("Node2", 0.0, 0.0));
    int id3 = g.add_node(std::make_shared<DriverNode>("Node3", 0.0, 0.0));

    g.add_edge(id1, id2, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(id2, id3, Edge{ 0.0, 1.0, 0.0, 0.0 });

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


    g.add_edge(a, b, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(b, d, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(a, c, Edge{ 0.0, 1.0, 0.0, 0.0 });

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

    g.add_edge(a, b, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(b, c, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(a, c, Edge{ 0.0, 1.0, 0.0, 0.0 });

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

    g.add_edge(a, b, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(b, d, Edge{ 0.0, 1.0, 0.0, 0.0 });
    g.add_edge(a, c, Edge{ 0.0, 5.0, 0.0, 0.0 });
    g.add_edge(c, d, Edge{ 0.0, 1.0, 0.0, 0.0 });

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

    original.add_edge(driver_id, team_id, Edge{ 0.62, 0.90, -0.1, 1.0 });
    original.add_edge(driver_id, circuit_id, Edge{ 0.15, 0.75, 0.05, 0.5 });
    original.add_edge(team_id, circuit_id, Edge{ 0.40, 0.60, 0.0, 0.25 });

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
    ok &= (e1.winRate == 0.62 && e1.affinityScore == 0.90 && e1.performanceDelta == -0.1 && e1.tirePreference == 1.0);

    Edge e2 = loaded.get_edge(driver_id, circuit_id);
    ok &= (e2.winRate == 0.15 && e2.affinityScore == 0.75 && e2.performanceDelta == 0.05 && e2.tirePreference == 0.5);

    Edge e3 = loaded.get_edge(team_id, circuit_id);
    ok &= (e3.winRate == 0.40 && e3.affinityScore == 0.60 && e3.performanceDelta == 0.0 && e3.tirePreference == 0.25);

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

    original.add_edge(a, c, Edge{ 0.20, 0.30, 0.40, 0.50 });

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
    ok &= (e.winRate == 0.20 && e.affinityScore == 0.30 && e.performanceDelta == 0.40 && e.tirePreference == 0.50);

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

    std::filesystem::remove("pitwall_f1.db");
    Storage storage("pitwall_f1.db");
    GraphRepository repo(storage);
    repo.create_tables();
    repo.save_graph(g);
    std::cout << "Saved graph to pitwall_f1.db\n";
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