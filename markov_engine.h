#pragma once
#include <map>

class MarkovEngine {
private:
    const std::map<int, std::map<int, double>>& counts;

public:
    explicit MarkovEngine(const std::map<int, std::map<int, double>>& transition_counts);

    std::map<int, double> predict_finish_distribution(int grid_position) const;
    int most_likely_finish(int grid_position) const;

    // Shifts the pooled distribution by driver_index positions (scalar
    // handicap). Fractional mass splits across straddling integer
    // positions, clamped to [1, max finish]; a P1 starter can never "gain".
    std::map<int, double> predict_finish_distribution_for_driver(int grid_position, double driver_index) const;
};
