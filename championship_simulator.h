#pragma once
#include "markov_engine.h"
#include "dirichlet_finish_model.h"
#include <map>
#include <string>
#include <vector>
#include <random>

class ChampionshipSimulator {
public:
    ChampionshipSimulator(const MarkovEngine& engine,
                          const std::string& results_json_path,
                          unsigned int seed = 42);

    int race_count() const;

    // Indexing races + assigning points
    std::map<std::string, double> points_through_race(int through_race) const;

    // Note: The model can't follow race-pace dominance or other cognitive behaviour
    // ex: If Hamilton does 10 overtakes and he still finishes on p5, his chances shift
    // like one's that advanced from p6 to p5 (and finished).
    std::map<std::string, double> simulate_championship(int from_race, int num_simulations = 10000) const;

private:
    struct RaceResult {
        std::string driver_name;
        std::string circuit_name;
        int position;
        int grid;
    };

    struct DriverSampler {
        std::vector<int> finishes;             
        std::discrete_distribution<int> dist; 
    };

    void load_results(const std::string& results_json_path);

    static DriverSampler make_sampler(const std::map<int, double>& distribution);

    std::map<std::string, DriverSampler> build_avg_grid_samplers() const;

    // One sampler per driver from their Dirichlet-prior
    // finish distribution built from races [1, from_race] only.
    std::map<std::string, DriverSampler> build_bayesian_samplers(int from_race) const;

    const MarkovEngine& engine_;
    DirichletFinishModel dirichlet_model_;
    std::vector<RaceResult> results_;
    std::vector<std::string> race_order_;               
    std::vector<std::vector<const RaceResult*>> races_;   
    std::vector<std::string> all_drivers_;               
    std::map<std::string, int> avg_grid_rounded_;         
    mutable std::mt19937 rng_;
};
