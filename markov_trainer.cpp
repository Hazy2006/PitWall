#include "markov_trainer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

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

void MarkovTrainer::train(const std::string& results_json_path) {
    json data = load_json_array(results_json_path);

    for (const auto& entry : data) {
        int grid = entry.at("grid").get<int>();
        if (grid == 0) {
            continue;
        }
        int finish = entry.at("position").get<int>();
        counts[grid][finish]++;
    }
}

const std::map<int, std::map<int, int>>& MarkovTrainer::get_counts() const {
    return counts;
}

int MarkovTrainer::total_observations() const {
    int total = 0;
    for (const auto& [grid, finishes] : counts) {
        for (const auto& [finish, count] : finishes) {
            total += count;
        }
    }
    return total;
}

std::map<std::string, double> MarkovTrainer::compute_driver_indices(const std::string& results_json_path) const {
    json data = load_json_array(results_json_path);

    std::map<std::string, double> sum_by_driver;
    std::map<std::string, int> count_by_driver;

    for (const auto& entry : data) {
        std::string driver = entry.at("driver_name").get<std::string>();
        // Touch both maps so every driver appears in the output even if all
        // of their rows get filtered below (grid == 0 -> zero valid rows).
        sum_by_driver[driver] += 0.0;
        count_by_driver[driver] += 0;

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
        indices[driver] = (count == 0) ? 0.0 : (sum_by_driver.at(driver) / static_cast<double>(count));
    }
    return indices;
}
