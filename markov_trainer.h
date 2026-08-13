#pragma once
#include <map>
#include <string>

class MarkovTrainer {
private:
    std::map<int, std::map<int, int>> counts;

public:
    void train(const std::string& results_json_path);
    const std::map<int, std::map<int, int>>& get_counts() const;
    int total_observations() const;

    // Driver skill index: mean(grid - finish) over grid != 0 races.
    // Positive means the driver gains positions on average; drivers with
    // fewer than 10 valid rows are omitted (too little data to trust).
    std::map<std::string, double> compute_driver_indices(const std::string& results_json_path) const;
};
