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
// position. Deliberately does NOT apply per-driver index shifts
// (MarkovEngine::predict_finish_distribution_for_driver) -- that is a later
// phase; this baseline always samples from the pooled distribution.
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

    // Simulates the rest of the season num_simulations times. Races
    // [1, from_race] are fixed at their actual results; races
    // [from_race + 1, race_count()] are simulated: each driver's finish is
    // sampled from engine.predict_finish_distribution(their rounded average
    // grid), all sampled finishes for the race are shuffled and then sorted
    // ascending (so ties are broken randomly, not by insertion order) to
    // produce a valid 1..K permutation, and points are awarded by that
    // assigned position. A driver with no valid average grid, or whose
    // rounded average grid has no pooled data in the engine, is skipped for
    // every remaining race (never assigned a position, scores 0).
    // Returns, per driver, the fraction of simulations in which they ended
    // the season with the most total points. An exact tie for the season
    // lead splits that simulation's win credit evenly among the tied
    // drivers, so the returned fractions always sum to 1.0.
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
