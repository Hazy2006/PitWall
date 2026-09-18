#pragma once
#include "graph.h"
#include "result_row.h"
#include <string>
#include <unordered_map>
#include <vector>

class DataImporter {
private:
    Graph& graph;
    std::unordered_map<std::string, int> name_to_id;

public:
    explicit DataImporter(Graph& graph_ref);

    void import_drivers(const std::string& path);
    void import_teams(const std::string& path);
    void import_circuits(const std::string& path);

    // Driver-circuit win rates, computed from the same ResultRow vector the
    // prediction pipeline ingests from SQL, not a second JSON parse.
    void import_results(const std::vector<ResultRow>& results);

    // Returns the node id for a previously imported name, or -1 if unknown.
    int get_node_id(const std::string& name) const;
};
