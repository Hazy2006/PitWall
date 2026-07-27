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
