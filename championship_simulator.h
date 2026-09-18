#pragma once
#include "dirichlet_finish_model.h"
#include "result_row.h"
#include <map>
#include <string>
#include <vector>
#include <random>

class ChampionshipSimulator {
public:
    ChampionshipSimulator(const std::vector<ResultRow>& results,
                          unsigned int seed = 42);

    int race_count() const;

    // Indexing races + assigning points
    std::map<std::string, double> points_through_race(int through_race) const;

    // Note: The model can't follow race-pace dominance or other cognitive behaviour
    // ex: If Hamilton does 10 overtakes and he still finishes on p5, his chances shift
    // like one's that advanced from p6 to p5 (and finished).
    std::map<std::string, double> simulate_championship(int from_race, int num_simulations = 10000) const;

private:
    struct DriverSampler {
        std::vector<int> finishes;
        std::discrete_distribution<int> dist;
    };

    void index_results(const std::vector<ResultRow>& results);

    static DriverSampler make_sampler(const std::map<int, double>& distribution);

    // One sampler per driver from their Dirichlet-prior
    // finish distribution built from races [1, from_race] only.
    std::map<std::string, DriverSampler> build_bayesian_samplers(int from_race) const;

    DirichletFinishModel dirichlet_model_;
    std::vector<ResultRow> results_;
    int race_count_ = 0;
    std::vector<std::vector<const ResultRow*>> races_;
    std::vector<std::string> all_drivers_;
    mutable std::mt19937 rng_;
};
