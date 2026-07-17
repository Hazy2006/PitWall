#include "graph.h"
#include <stdexcept>

Graph::Graph() : next_id(0) {}

int Graph::count_vertices() const {
    return static_cast<int>(node_registry.size());
}

int Graph::count_edges() const {
    return static_cast<int>(edges.size());
}

int Graph::add_node(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Cannot add a null node pointer.");
    }
    int id = next_id++;
    node_registry[id] = node;
    successors[id] = std::vector<int>();
    predecessors[id] = std::vector<int>();
    return id;
}

std::shared_ptr<Node> Graph::get_node(int id) const {
    if (node_registry.count(id) == 0) {
        throw std::invalid_argument("Node ID does not exist.");
    }
    return node_registry.at(id);
}



bool Graph::is_edge(int x, int y) const {
    // Before checking edges, ensure vertices actually exist in the registry
    if (node_registry.count(x) == 0 || node_registry.count(y) == 0) {
        throw std::invalid_argument("One or both vertices do not exist!");
    }
    return edges.count({ x, y }) > 0;
}

void Graph::add_edge(int x, int y, const Edge& edge_data) {
    if (node_registry.count(x) == 0 || node_registry.count(y) == 0) {
        throw std::invalid_argument("One or both vertices do not exist!");
    }
    if (is_edge(x, y)) {
        return; // Edge already exists, do nothing 
    }

    // Wire up the routing maps
    successors[x].push_back(y);
    predecessors[y].push_back(x);

    // Store the Markov engine stats
    edges[{x, y}] = edge_data;
}

Edge Graph::get_edge(int x, int y) const {
    if (!is_edge(x, y)) {
        throw std::invalid_argument("Edge does not exist!");
    }
    return edges.at({ x, y });
}