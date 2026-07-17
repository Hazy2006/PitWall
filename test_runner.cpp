#include "graph.h"
#include <cassert>
#include <iostream>
#include <memory>

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