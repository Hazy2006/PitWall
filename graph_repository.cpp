#include "graph_repository.h"
#include "node.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace {
    std::string sql_escape(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '\'') out += '\'';
            out += c;
        }
        return out;
    }

    std::shared_ptr<Node> make_node(const std::string& type,
                                     const std::string& name,
                                     double param1, double param2) {
        if (type == "Driver") return std::make_shared<DriverNode>(name, param1, param2);
        if (type == "Team")   return std::make_shared<TeamNode>(name, param1);
        if (type == "Circuit") return std::make_shared<CircuitNode>(name, param1);
        throw std::invalid_argument("Unknown node type: " + type);
    }
}

GraphRepository::GraphRepository(Storage& storage_ref) : storage(storage_ref) {}

void GraphRepository::create_tables() {
    storage.execute(
        "CREATE TABLE IF NOT EXISTS nodes ("
        "id INTEGER PRIMARY KEY,"
        "type TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "param1 REAL,"
        "param2 REAL"
        ");"
    );

    storage.execute(
        "CREATE TABLE IF NOT EXISTS edges ("
        "source_id INTEGER NOT NULL REFERENCES nodes(id),"
        "target_id INTEGER NOT NULL REFERENCES nodes(id),"
        "win_rate REAL NOT NULL DEFAULT 0.0,"
        "PRIMARY KEY (source_id, target_id)"
        ");"
    );
}

void GraphRepository::save_graph(const Graph& g) {
    for (int id : g.get_all_node_ids()) {
        std::shared_ptr<Node> node = g.get_node(id);
        std::string type = node->get_type_string();
        std::string name = sql_escape(node->get_name());
        double param1 = 0.0;
        double param2 = 0.0;

        if (auto driver = std::dynamic_pointer_cast<DriverNode>(node)) {
            param1 = driver->tire_management_modifier;
            param2 = driver->base_pace_delta;
        }
        else if (auto team = std::dynamic_pointer_cast<TeamNode>(node)) {
            param1 = team->pit_stop_variance;
        }
        else if (auto circuit = std::dynamic_pointer_cast<CircuitNode>(node)) {
            param1 = circuit->base_degradation_rate;
        }

        std::ostringstream sql;
        sql << std::setprecision(17);
        sql << "INSERT INTO nodes (id, type, name, param1, param2) VALUES ("
            << id << ", '" << type << "', '" << name << "', "
            << param1 << ", " << param2 << ");";
        storage.execute(sql.str());
    }

    for (const auto& key : g.get_all_edge_keys()) {
        Edge e = g.get_edge(key.first, key.second);
        std::ostringstream sql;
        sql << std::setprecision(17);
        sql << "INSERT INTO edges (source_id, target_id, win_rate) VALUES ("
            << key.first << ", " << key.second << ", "
            << e.winRate << ");";
        storage.execute(sql.str());
    }
}

Graph GraphRepository::load_graph() {
    Graph g;

    auto node_rows = storage.query("SELECT id, type, name, param1, param2 FROM nodes ORDER BY id ASC;");
    for (const auto& row : node_rows) {
        std::string type = row.at("type");
        std::string name = row.at("name");
        double param1 = row.count("param1") ? std::stod(row.at("param1")) : 0.0;
        double param2 = row.count("param2") ? std::stod(row.at("param2")) : 0.0;

        int id = std::stoi(row.at("id"));
        g.add_node_with_id(id, make_node(type, name, param1, param2));
    }

    auto edge_rows = storage.query(
        "SELECT source_id, target_id, win_rate FROM edges;"
    );
    for (const auto& row : edge_rows) {
        int source_id = std::stoi(row.at("source_id"));
        int target_id = std::stoi(row.at("target_id"));
        Edge e{ std::stod(row.at("win_rate")) };
        g.add_edge(source_id, target_id, e);
    }

    return g;
}
