#pragma once
#include "markov_engine.h"
#include <map>
#include <string>
#include <vector>
#include <random>

// Baseline Monte Carlo championship simulator. Graph-free, Storage-free:
// depends only on a MarkovEngine and the results JSON, from which it derives
// the race order, each driver's actual points through a given race, and each
// driver's rounded average grid position.
class ChampionshipSimulator {
public:
    // seed defaults to a fixed value, so unseeded use reproduces identical
    // results run to run; pass an explicit seed to get different draws.
    ChampionshipSimulator(const MarkovEngine& engine,
                          const std::string& results_json_path,
                          unsigned int seed = 42);

    // Distinct circuits, in first-appearance order.
    int race_count() const;

    // Real points across races [1, through_race] (1-indexed; 0 = no races
    // yet). Standard 25-18-15-...-1 scoring for P1-P10. Every driver appears
    // in the result, even at 0.0, so callers don't need existence checks.
    std::map<std::string, double> points_through_race(int through_race) const;

    // Simulates the remainder of the season num_simulations times: races
    // [1, from_race] are fixed at actual results, the rest are simulated.
    // from_race == 0 simulates the whole season. Each simulated race samples
    // a finish per driver from their pooled avg-grid distribution, then
    // sample-then-ranks into a valid permutation (ties broken randomly);
    // drivers with no avg grid or no pooled data score 0 that race.
    // Returns each driver's fraction of simulations won (sums to 1.0).
    //
    // Before any of that, a hard arithmetic clinch/elimination check
    // overrides the RNG: if the leader's current lead already exceeds every
    // rival's maximum possible remaining points, the leader wins outright
    // (1.0, no simulation run); any driver who can't mathematically catch
    // the leader is credited 0.0, even if the RNG would occasionally sample
    // them into the lead.
    //
    // Known limitation: late-season title odds understate a dominant leader
    // who has not yet mathematically clinched. The model uses each driver's
    // average grid position and cannot capture race-pace dominance, so a
    // trailing driver retains simulated title chances that were, in
    // reality, effectively gone. The clinch/elimination constraint corrects
    // only the arithmetically-decided cases (a leader who cannot be caught,
    // or a driver who cannot catch up); the residual softness in
    // not-yet-decided late races is a model limitation, not a bug.
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
