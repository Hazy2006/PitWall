#pragma once
#include <map>

class MarkovEngine {
private:
    const std::map<int, std::map<int, int>>& counts;

public:
    explicit MarkovEngine(const std::map<int, std::map<int, int>>& transition_counts);

    std::map<int, double> predict_finish_distribution(int grid_position) const;
    int most_likely_finish(int grid_position) const;
};
