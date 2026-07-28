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

    // Per-driver performance index: mean(grid - finish) across all grid != 0 rows.
    // Positive means the driver tends to finish ahead of their grid slot.
    // Drivers with fewer than 10 valid rows are omitted from the result entirely.
    std::map<std::string, double> compute_driver_indices(const std::string& results_json_path) const;
};
