#pragma once
#include <map>

class MarkovEngine {
private:
    const std::map<int, std::map<int, int>>& counts;

public:
    explicit MarkovEngine(const std::map<int, std::map<int, int>>& transition_counts);

    std::map<int, double> predict_finish_distribution(int grid_position) const;
    int most_likely_finish(int grid_position) const;

    // Shifts the pooled distribution toward better finishes by driver_index positions
    // (a single scalar "handicap displacement", not a per-driver transition matrix).
    // driver_index is used at full fractional precision (not rounded): each bucket's
    // mass lands on a fractional target position and is split between the two
    // integer positions straddling it, proportional to distance. Targets outside
    // [1, max finish present for this grid] are clamped to that bound before mass
    // is accumulated, so total probability mass is conserved.
    // Known V1 limitation: a driver starting P1 in a dominant car can never show a
    // "gain" no matter how large their index, since there is no room above P1.
    // Returns an empty map if grid_position has no pooled data.
    std::map<int, double> predict_finish_distribution_for_driver(int grid_position, double driver_index) const;
};
