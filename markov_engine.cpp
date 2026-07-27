#include "markov_engine.h"

MarkovEngine::MarkovEngine(const std::map<int, std::map<int, int>>& transition_counts)
    : counts(transition_counts) {
}

std::map<int, double> MarkovEngine::predict_finish_distribution(int grid_position) const {
    std::map<int, double> distribution;

    auto row_it = counts.find(grid_position);
    if (row_it == counts.end()) {
        return distribution;
    }

    const std::map<int, int>& row = row_it->second;
    int row_total = 0;
    for (const auto& [finish, count] : row) {
        row_total += count;
    }

    if (row_total == 0) {
        return distribution;
    }

    for (const auto& [finish, count] : row) {
        distribution[finish] = static_cast<double>(count) / static_cast<double>(row_total);
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
