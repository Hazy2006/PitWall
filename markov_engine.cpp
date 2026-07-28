#include "markov_engine.h"
#include <cmath>
#include <algorithm>

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

std::map<int, double> MarkovEngine::predict_finish_distribution_for_driver(int grid_position, double driver_index) const {
    std::map<int, double> pooled = predict_finish_distribution(grid_position);
    if (pooled.empty()) {
        return pooled;
    }

    int max_finish = pooled.rbegin()->first;

    std::map<int, double> shifted;
    for (const auto& [finish, prob] : pooled) {
        double target = static_cast<double>(finish) - driver_index;
        int lo = static_cast<int>(std::floor(target));
        int hi = static_cast<int>(std::ceil(target));

        // frac_lo + frac_hi always sums to 1.0, including when target is an
        // exact integer (lo == hi, frac_hi == 0.0, all mass goes to lo).
        double frac_hi = target - lo;
        double frac_lo = 1.0 - frac_hi;

        int clamped_lo = std::clamp(lo, 1, max_finish);
        int clamped_hi = std::clamp(hi, 1, max_finish);

        shifted[clamped_lo] += prob * frac_lo;
        shifted[clamped_hi] += prob * frac_hi;
    }
    return shifted;
}
