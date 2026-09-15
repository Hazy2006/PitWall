#include "markov_trainer.h"
#include "json_utils.h"
#include "recency.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <vector>

using json = nlohmann::json;

void MarkovTrainer::train(const std::string& results_json_path) {
    json data = load_json_array(results_json_path);

    struct ValidRow {
        int grid;
        int finish;
        int race_index;
    };

    std::map<std::string, int> race_index_by_circuit;
    std::vector<ValidRow> valid_rows;
    int race_count = 0;

    for (const auto& entry : data) {
        std::string circuit = entry.at("circuit_name").get<std::string>();
        auto it = race_index_by_circuit.find(circuit);
        int race_index;
        if (it == race_index_by_circuit.end()) {
            race_index = race_count++;
            race_index_by_circuit[circuit] = race_index;
        }
        else {
            race_index = it->second;
        }

        int grid = entry.at("grid").get<int>();
        if (grid == 0) {
            continue;
        }
        int finish = entry.at("position").get<int>();
        valid_rows.push_back({ grid, finish, race_index });
    }

    // Recency-weighted, anchored to the most recent race, so the pooled
    // table reflects current form rather than blending in stale history.
    for (const auto& row : valid_rows) {
        int k = race_count - 1 - row.race_index;
        counts[row.grid][row.finish] += std::pow(RECENCY_LAMBDA, k);
    }
}

const std::map<int, std::map<int, double>>& MarkovTrainer::get_counts() const {
    return counts;
}

double MarkovTrainer::total_observations() const {
    double total = 0.0;
    for (const auto& [grid, finishes] : counts) {
        for (const auto& [finish, weight] : finishes) {
            total += weight;
        }
    }
    return total;
}

std::map<std::string, double> MarkovTrainer::compute_driver_indices(const std::string& results_json_path) const {
    constexpr int kMinValidRows = 10;

    json data = load_json_array(results_json_path);

    std::map<std::string, double> sum_by_driver;
    std::map<std::string, int> count_by_driver;

    for (const auto& entry : data) {
        std::string driver = entry.at("driver_name").get<std::string>();

        int grid = entry.at("grid").get<int>();
        if (grid == 0) {
            continue;
        }
        int finish = entry.at("position").get<int>();
        sum_by_driver[driver] += static_cast<double>(grid - finish);
        count_by_driver[driver] += 1;
    }

    std::map<std::string, double> indices;
    for (const auto& [driver, count] : count_by_driver) {
        if (count < kMinValidRows) {
            continue;
        }
        indices[driver] = sum_by_driver.at(driver) / static_cast<double>(count);
    }
    return indices;
}

std::map<std::string, double> MarkovTrainer::compute_team_indices(const std::string& results_json_path) const {
    constexpr int kMinValidRows = 10;

    json data = load_json_array(results_json_path);

    std::map<std::string, double> sum_by_team;
    std::map<std::string, int> count_by_team;

    for (const auto& entry : data) {
        std::string team = entry.at("team_name").get<std::string>();

        int grid = entry.at("grid").get<int>();
        if (grid == 0) {
            continue;
        }
        int finish = entry.at("position").get<int>();
        sum_by_team[team] += static_cast<double>(grid - finish);
        count_by_team[team] += 1;
    }

    std::map<std::string, double> indices;
    for (const auto& [team, count] : count_by_team) {
        if (count < kMinValidRows) {
            continue;
        }
        indices[team] = sum_by_team.at(team) / static_cast<double>(count);
    }
    return indices;
}

std::map<std::string, std::string> MarkovTrainer::compute_driver_teams(const std::string& results_json_path) const {
    json data = load_json_array(results_json_path);

    std::map<std::string, std::string> driver_team;
    for (const auto& entry : data) {
        std::string driver = entry.at("driver_name").get<std::string>();
        std::string team = entry.at("team_name").get<std::string>();
        driver_team[driver] = team;  // last occurrence in file order wins
    }
    return driver_team;
}
