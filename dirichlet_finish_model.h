#pragma once
#include "result_row.h"
#include <map>
#include <set>
#include <string>
#include <vector>

// Bayesian per-driver finishing-position model: a Dirichlet(1,...,1) prior
// updated only with races [1, through_race]. Never reads later races, so
// forecasts from race N can't leak knowledge of how the season turned out.
class DirichletFinishModel {
public:
    explicit DirichletFinishModel(const std::vector<ResultRow>& results);

    int race_count() const;

    // Bayesian posterior over positions [1, max_position]: prior updated with
    // driver_name's real finishes through races [1, through_race]. grid==0
    // rows are skipped as evidence. Unknown driver returns an empty map.
    std::map<int, double> driver_finish_distribution(const std::string& driver_name, int through_race) const;

private:
    void load(const std::vector<ResultRow>& results);

    int race_count_ = 0;
    std::set<std::string> all_drivers_;
    // driver -> list of (0-based race_order, finish position), grid != 0 rows only.
    std::map<std::string, std::vector<std::pair<int, int>>> driver_race_finishes_;
    int max_position_ = 0;
};
