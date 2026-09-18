#include "markov_engine.h"
#include <algorithm>

MarkovEngine::MarkovEngine(const std::map<int, std::map<int, double>>& transition_counts)
    : counts(transition_counts) {
}

std::map<int, double> MarkovEngine::predict_finish_distribution(int grid_position) const {
    std::map<int, double> distribution;

    auto row_it = counts.find(grid_position);
    if (row_it == counts.end()) {
        return distribution;
    }

    const std::map<int, double>& row = row_it->second;
    double row_total = 0.0;
    for (const auto& [finish, weight] : row) {
        row_total += weight;
    }

    if (row_total <= 0.0) {
        return distribution;
    }

    for (const auto& [finish, weight] : row) {
        distribution[finish] = weight / row_total;
    }

    return distribution;
}

int MarkovEngine::most_likely_finish(int grid_position) const {
    std::map<int, double> distribution = predict_finish_distribution(grid_position);
    if (distribution.empty()) {
        return -1;
    }

    int best_finish = -1;
    double best_prob = -1.0;
    for (const auto& [finish, prob] : distribution) {
        if (prob > best_prob) {
            best_prob = prob;
            best_finish = finish;
        }
    }
    return best_finish;
}

std::map<int, double> MarkovEngine::predict_finish_distribution_for_driver(int grid_position, const std::map<int, double>& delta_distribution) const {
    std::map<int, double> pooled = predict_finish_distribution(grid_position);
    if (pooled.empty()) {
        return pooled;
    }

    int max_finish = pooled.rbegin()->first;

    std::map<int, double> shifted;
    for (const auto& [finish, prob] : pooled) {
        for (const auto& [delta, weight] : delta_distribution) {
            int target = std::clamp(finish - delta, 1, max_finish);
            shifted[target] += prob * weight;
        }
    }
    return shifted;
}
