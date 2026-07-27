#pragma once
#include "graph.h"
#include <string>
#include <unordered_map>

class DataImporter {
private:
    Graph& graph;
    std::unordered_map<std::string, int> name_to_id;

public:
    explicit DataImporter(Graph& graph_ref);

    void import_drivers(const std::string& path);
    void import_teams(const std::string& path);
    void import_circuits(const std::string& path);
    void import_results(const std::string& path);

    // Returns the node id for a previously imported name, or -1 if unknown.
    int get_node_id(const std::string& name) const;
};
