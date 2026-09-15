#pragma once
#include <map>
#include <string>

class MarkovTrainer {
private:
    std::map<int, std::map<int, double>> counts;

public:
    void train(const std::string& results_json_path);
    const std::map<int, std::map<int, double>>& get_counts() const;
    double total_observations() const;

    // Driver skill index: mean(grid - finish) over grid != 0 races.
    // Positive means the driver gains positions on average; drivers with
    // fewer than 10 valid rows are omitted (too little data to trust).
    std::map<std::string, double> compute_driver_indices(const std::string& results_json_path) const;

    // Team skill index: same formula as compute_driver_indices, pooled
    // across every driver who raced for that team. Used as a fallback for
    // drivers below the personal-data threshold.
    std::map<std::string, double> compute_team_indices(const std::string& results_json_path) const;

    // Most recent team each driver raced for (last occurrence in file order
    // wins, so mid-season team changes resolve to the newer team).
    std::map<std::string, std::string> compute_driver_teams(const std::string& results_json_path) const;
};
