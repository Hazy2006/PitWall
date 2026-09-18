#include "dirichlet_finish_model.h"
#include "recency.h"
#include <algorithm>
#include <cmath>

DirichletFinishModel::DirichletFinishModel(const std::vector<ResultRow>& results) {
    load(results);
}

void DirichletFinishModel::load(const std::vector<ResultRow>& results) {
    for (const auto& row : results) {
        all_drivers_.insert(row.driver_name);
        max_position_ = std::max(max_position_, row.position);
        race_count_ = std::max(race_count_, row.race_order + 1);

        if (row.grid == 0) {
            continue;  // pit-lane starts carry no finish-position evidence
        }
        driver_race_finishes_[row.driver_name].emplace_back(row.race_order, row.position);
    }
}

int DirichletFinishModel::race_count() const {
    return race_count_;
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
