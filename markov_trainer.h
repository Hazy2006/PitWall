#pragma once
#include "result_row.h"
#include <map>
#include <string>
#include <vector>

class MarkovTrainer {
private:
    std::map<int, std::map<int, double>> counts;

public:
    void train(const std::vector<ResultRow>& results);
    const std::map<int, std::map<int, double>>& get_counts() const;
    double total_observations() const;

    // Driver skill index: mean(grid - finish) over grid != 0 races.
    // Positive means the driver gains positions on average; drivers with
    // fewer than 10 valid rows are omitted (too little data to trust).
    std::map<std::string, double> compute_driver_indices(const std::vector<ResultRow>& results) const;

    // Team skill index: same formula as compute_driver_indices, pooled
    // across every driver who raced for that team. Used as a fallback for
    // drivers below the personal-data threshold.
    std::map<std::string, double> compute_team_indices(const std::vector<ResultRow>& results) const;

    // Most recent team each driver raced for (last occurrence in results-
    // vector order wins, so mid-season team changes resolve to the newer team).
    std::map<std::string, std::string> compute_driver_teams(const std::vector<ResultRow>& results) const;

    // Per-driver posterior over grid-minus-finish deltas: a pooled-shaped
    // prior updated with that driver's own recency-weighted evidence.
    // Every driver in results gets an entry.
    std::map<std::string, std::map<int, double>> compute_driver_delta_distributions(const std::vector<ResultRow>& results) const;

    // Same, pooled per team.
    std::map<std::string, std::map<int, double>> compute_team_delta_distributions(const std::vector<ResultRow>& results) const;
};
