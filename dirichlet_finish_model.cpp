#include "dirichlet_finish_model.h"
#include "json_utils.h"
#include "recency.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

DirichletFinishModel::DirichletFinishModel(const std::string& results_json_path) {
    load(results_json_path);
}

void DirichletFinishModel::load(const std::string& results_json_path) {
    json data = load_json_array(results_json_path);

    std::map<std::string, int> race_index_by_circuit;

    for (const auto& entry : data) {
        std::string driver = entry.at("driver_name").get<std::string>();
        std::string circuit = entry.at("circuit_name").get<std::string>();
        int position = entry.at("position").get<int>();
        int grid = entry.at("grid").get<int>();

        if (race_index_by_circuit.find(circuit) == race_index_by_circuit.end()) {
            race_index_by_circuit[circuit] = static_cast<int>(race_order_.size());
            race_order_.push_back(circuit);
        }

        all_drivers_.insert(driver);
        max_position_ = std::max(max_position_, position);

        if (grid == 0) {
            continue;  // pit-lane starts carry no finish-position evidence
        }
        int race_index = race_index_by_circuit.at(circuit);
        driver_race_finishes_[driver].emplace_back(race_index, position);
    }
}

int DirichletFinishModel::race_count() const {
    return static_cast<int>(race_order_.size());
}

std::map<int, double> DirichletFinishModel::driver_finish_distribution(const std::string& driver_name, int through_race) const {
    if (all_drivers_.find(driver_name) == all_drivers_.end()) {
        return {};
    }

    std::map<int, double> counts;
    for (int pos = 1; pos <= max_position_; ++pos) {
        counts[pos] = 1.0;  // Dirichlet(1,...,1) prior
    }

    auto it = driver_race_finishes_.find(driver_name);
    if (it != driver_race_finishes_.end()) {
        for (const auto& [race_index, position] : it->second) {
            if (race_index < through_race) {
                // race_index is 0-based, through_race is a 1-based cutoff;
                // race_index + 1 is this race's 1-based number, so k counts
                // how many races before the cutoff it happened.
                int k = through_race - (race_index + 1);
                counts[position] += std::pow(RECENCY_LAMBDA, k);
            }
        }
    }

    double total = 0.0;
    for (const auto& [pos, count] : counts) {
        total += count;
    }

    // Normalize counts into the posterior probability distribution.
    std::map<int, double> distribution;
    for (const auto& [pos, count] : counts) {
        distribution[pos] = count / total;
    }
    return distribution;
}
