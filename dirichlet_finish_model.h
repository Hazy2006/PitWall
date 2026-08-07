#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

// Per-driver, per-race-cutoff Bayesian finishing-position model. Standalone:
// depends only on the results JSON, no Graph/Storage/MarkovEngine coupling.
//
// Each driver's distribution starts from a symmetric Dirichlet(1,...,1)
// prior -- one pseudo-finish per possible position, i.e. "everyone equally
// uncertain before evidence" -- and is updated only with that driver's real
// finishes from races [1, through_race]. Races after through_race are never
// read, which is what makes this leakage-free: forecasting "from race N"
// only ever reflects what was actually known after race N.
class DirichletFinishModel {
public:
    explicit DirichletFinishModel(const std::string& results_json_path);

    // Distinct circuits, in first-appearance order.
    int race_count() const;

    // driver_name's finish-position distribution: Dirichlet(1,...,1) prior
    // over positions [1, max_position] (max_position is the highest
    // finishing position appearing anywhere in the season, i.e. the field
    // size -- a structural fact, not a race outcome), updated with
    // driver_name's real finishes from races [1, through_race] only.
    // grid==0 (pit-lane start) rows are skipped as evidence, consistent with
    // existing filters elsewhere. through_race <= 0 returns the pure prior.
    // A driver never seen in the results file returns an empty map.
    std::map<int, double> driver_finish_distribution(const std::string& driver_name, int through_race) const;

private:
    void load(const std::string& results_json_path);

    std::vector<std::string> race_order_;  // race_order_[i] = circuit for race i+1
    std::set<std::string> all_drivers_;
    // driver -> list of (0-based race index, finish position), grid != 0 rows only.
    std::map<std::string, std::vector<std::pair<int, int>>> driver_race_finishes_;
    int max_position_ = 0;
};
