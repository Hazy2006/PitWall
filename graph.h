#pragma once
#include "node.h"
#include "edge.h"
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

class Graph {
private:
    std::unordered_map<int, std::shared_ptr<Node>> node_registry;
    std::unordered_map<int, std::vector<int>> successors;
    std::unordered_map<int, std::vector<int>> predecessors;
    std::map<std::pair<int, int>, Edge> edges;
    int next_id;

public:
    Graph();
    int count_vertices() const;
    int add_node(std::shared_ptr<Node> node);
    std::shared_ptr<Node> get_node(int id) const;

    
    int count_edges() const;
    void add_edge(int x, int y, const Edge& edge_data);
    bool is_edge(int x, int y) const;
    Edge get_edge(int x, int y) const;
};

//a