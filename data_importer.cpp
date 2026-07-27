#include "data_importer.h"
#include "node.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <map>
#include <utility>

using json = nlohmann::json;

namespace {
    json load_json_array(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        json data;
        file >> data;
        return data;
    }
}

DataImporter::DataImporter(Graph& graph_ref) : graph(graph_ref) {}

void DataImporter::import_drivers(const std::string& path) {
    json data = load_json_array(path);
    for (const auto& entry : data) {
        std::string name = entry.at("name").get<std::string>();
        double tire_mod = entry.value("tire_management_modifier", 0.0);
        double pace_delta = entry.value("base_pace_delta", 0.0);
        int id = graph.add_node(std::make_shared<DriverNode>(name, tire_mod, pace_delta));
        name_to_id[name] = id;
    }
}

void DataImporter::import_teams(const std::string& path) {
    json data = load_json_array(path);
    for (const auto& entry : data) {
        std::string name = entry.at("name").get<std::string>();
        double pit_variance = entry.value("pit_stop_variance", 0.0);
        int id = graph.add_node(std::make_shared<TeamNode>(name, pit_variance));
        name_to_id[name] = id;
    }
}

void DataImporter::import_circuits(const std::string& path) {
    json data = load_json_array(path);
    for (const auto& entry : data) {
        std::string name = entry.at("name").get<std::string>();
        double deg_rate = entry.value("base_degradation_rate", 0.0);
        int id = graph.add_node(std::make_shared<CircuitNode>(name, deg_rate));
        name_to_id[name] = id;
    }
}

void DataImporter::import_results(const std::string& path) {
    json data = load_json_array(path);

    struct RaceStats {
        int wins = 0;
        int total = 0;
    };
    std::map<std::pair<std::string, std::string>, RaceStats> stats_by_driver_circuit;

    for (const auto& entry : data) {
        std::string driver_name = entry.at("driver_name").get<std::string>();
        std::string circuit_name = entry.at("circuit_name").get<std::string>();
        int position = entry.at("position").get<int>();

        RaceStats& stats = stats_by_driver_circuit[{ driver_name, circuit_name }];
        stats.total += 1;
        if (position == 1) {
            stats.wins += 1;
        }
    }

    for (const auto& [key, stats] : stats_by_driver_circuit) {
        const std::string& driver_name = key.first;
        const std::string& circuit_name = key.second;

        auto driver_it = name_to_id.find(driver_name);
        auto circuit_it = name_to_id.find(circuit_name);
        if (driver_it == name_to_id.end() || circuit_it == name_to_id.end()) {
            continue;
        }

        double win_rate = static_cast<double>(stats.wins) / static_cast<double>(stats.total);
        Edge edge_data{ win_rate, 0.0, 0.0, 0.0 };
        graph.add_edge(driver_it->second, circuit_it->second, edge_data);
    }
}

int DataImporter::get_node_id(const std::string& name) const {
    auto it = name_to_id.find(name);
    return it == name_to_id.end() ? -1 : it->second;
}
