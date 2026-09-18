#pragma once
#include <map>

class MarkovEngine {
private:
    const std::map<int, std::map<int, double>>& counts;

public:
    explicit MarkovEngine(const std::map<int, std::map<int, double>>& transition_counts);

    std::map<int, double> predict_finish_distribution(int grid_position) const;
    int most_likely_finish(int grid_position) const;

    // Convolves the pooled distribution with a driver's (or team's) own
    // distribution over grid-minus-finish deltas, clamped to [1, max
    // finish]; a P1 starter can never "gain".
    std::map<int, double> predict_finish_distribution_for_driver(int grid_position, const std::map<int, double>& delta_distribution) const;
};
