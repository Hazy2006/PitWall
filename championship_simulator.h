#pragma once
#include "markov_engine.h"
#include <map>
#include <string>
#include <vector>
#include <random>

// Baseline Monte Carlo championship simulator.
//
// Graph-free, Storage-free: depends only on a MarkovEngine (the pooled
// grid -> finish transition model) and the results JSON, which is used to
// derive three things -- the season's race order, each driver's actual
// points through a given race, and each driver's rounded average grid
// position. 
class ChampionshipSimulator {
public:
    // seed defaults to a random_device draw, so unseeded production use
    // varies run to run; tests pass a fixed seed for reproducibility.
    ChampionshipSimulator(const MarkovEngine& engine,
                          const std::string& results_json_path,
                          unsigned int seed = std::random_device{}());

    // Number of races in the season: the count of distinct circuits, in the
    // order they first appear in the results file.
    int race_count() const;

    // Real points earned by every driver across races [1, through_race]
    // (1-indexed; through_race == 0 means no races have happened yet).
    // Scoring: 25-18-15-12-10-8-6-4-2-1 for finishing positions 1-10, 0
    // otherwise. Every driver who appears anywhere in the season is present
    // in the result, even with 0.0 points.
    std::map<std::string, double> points_through_race(int through_race) const;

    // Simulates the remainder of the season num_simulations times.
    // Races [1, from_race] are fixed at actual results; [from_race+1, end] are simulated.
    // @param from_race  races up to and including this use real results; 0 = simulate all
    // @param num_simulations  number of Monte Carlo runs
    // @return per-driver fraction of simulations won (sums to 1.0)
    // @note Each race: sample finish from pooled avg-grid distribution, then
    //  sample-then-rank into a valid permutation (ties broken randomly).
    //  Drivers with no avg grid or no pooled data score 0 for that race.


    std::map<std::string, double> simulate_championship(int from_race, int num_simulations = 10000) const;

private:
    struct RaceResult {
        std::string driver_name;
        std::string circuit_name;
        int position;
        int grid;
    };

    void load_results(const std::string& results_json_path);

    const MarkovEngine& engine_;
    std::vector<RaceResult> results_;
    std::vector<std::string> race_order_;                // race_order_[i] = circuit for race i+1
    std::vector<std::vector<const RaceResult*>> races_;   // races_[i] = actual entries for race i+1
    std::vector<std::string> all_drivers_;                // every driver seen this season, first-seen order
    std::map<std::string, int> avg_grid_rounded_;         // driver -> rounded mean grid (grid != 0 rows only)
    mutable std::mt19937 rng_;
};
