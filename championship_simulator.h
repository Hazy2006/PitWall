#pragma once
#include "markov_engine.h"
#include "dirichlet_finish_model.h"
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
    // a finish per driver from their per-driver Dirichlet-prior finish
    // distribution built from races [1, from_race] only (see
    // DirichletFinishModel) -- never from races after from_race, so an
    // early cutoff can't leak knowledge of how the season actually turned
    // out -- then sample-then-ranks into a valid permutation (ties broken
    // randomly). Returns each driver's fraction of simulations won (sums to
    // 1.0).
    //
    // The old pooled avg-grid distribution (via MarkovEngine, full-season
    // average grid position) is kept as build_avg_grid_samplers() for
    // comparison but is no longer used by default: it pools every driver's
    // entire season regardless of from_race, which is exactly the
    // future-leakage this Bayesian path exists to remove.
    //
    // Before any of that, a hard arithmetic clinch/elimination check
    // overrides the RNG: if the leader's current lead already exceeds every
    // rival's maximum possible remaining points, the leader wins outright
    // (1.0, no simulation run); any driver who can't mathematically catch
    // the leader is credited 0.0, even if the RNG would occasionally sample
    // them into the lead.
    //
    // Known limitation: late-season title odds understate a dominant leader
    // who has not yet mathematically clinched. The model draws each
    // driver's finish from their own Dirichlet-prior distribution and
    // cannot capture race-pace dominance, so a trailing driver retains
    // simulated title chances that were, in reality, effectively gone. The
    // clinch/elimination constraint corrects only the arithmetically-decided
    // cases (a leader who cannot be caught,
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

    struct DriverSampler {
        std::vector<int> finishes;             // finish positions, in distribution's map order
        std::discrete_distribution<int> dist;  // samples an index into finishes
    };

    void load_results(const std::string& results_json_path);

    // Builds a sampler that draws finish positions according to distribution.
    static DriverSampler make_sampler(const std::map<int, double>& distribution);

    // OLD path: one sampler per driver from the pooled full-season avg-grid
    // distribution. Kept for comparison; not called by simulate_championship.
    std::map<std::string, DriverSampler> build_avg_grid_samplers() const;

    // NEW (default) path: one sampler per driver from their Dirichlet-prior
    // finish distribution built from races [1, from_race] only.
    std::map<std::string, DriverSampler> build_bayesian_samplers(int from_race) const;

    const MarkovEngine& engine_;
    DirichletFinishModel dirichlet_model_;
    std::vector<RaceResult> results_;
    std::vector<std::string> race_order_;                // race_order_[i] = circuit for race i+1
    std::vector<std::vector<const RaceResult*>> races_;   // races_[i] = actual entries for race i+1
    std::vector<std::string> all_drivers_;                // every driver seen this season, first-seen order
    std::map<std::string, int> avg_grid_rounded_;         // driver -> rounded mean grid (grid != 0 rows only)
    mutable std::mt19937 rng_;
};
