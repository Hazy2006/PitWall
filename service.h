#pragma once
#include "graph.h"
#include "data_importer.h"
#include "markov_trainer.h"
#include "markov_engine.h"
#include "strategy_reporter.h"
#include "championship_simulator.h"
#include <map>
#include <optional>
#include <string>

// Owns the full prediction pipeline. MarkovEngine, StrategyReporter, and
// ChampionshipSimulator each hold a const& into other members here
// (transition counts, driver indices, the engine), so one owner must control
// every lifetime. Members are declared in dependency order so reverse-order
// destruction tears down simulator_/reporter_ before what they reference;
// non-copyable/non-movable keeps those references from ever binding to a
// different object. engine_/simulator_/reporter_ are optional because they
// can't be built until trainer_ (and then engine_) has run.
class PitWallService {
public:
    PitWallService();

    PitWallService(const PitWallService&) = delete;
    PitWallService& operator=(const PitWallService&) = delete;
    PitWallService(PitWallService&&) = delete;
    PitWallService& operator=(PitWallService&&) = delete;

    // Runs the full pipeline; the service is ready for report() and
    // simulate_championship() once this returns.
    void load(const std::string& data_dir);

    std::string report(int grid_position, const std::string& driver_name) const;

    // See ChampionshipSimulator::simulate_championship.
    std::map<std::string, double> simulate_championship(int from_race, int num_simulations = 10000) const;

private:
    void importData(const std::string& data_dir);
    void trainModel(const std::string& data_dir);
    void applyIndices();
    void buildReporter();

    Graph graph_;
    MarkovTrainer trainer_;
    std::map<std::string, double> driver_indices_;
    DataImporter importer_;
    std::optional<MarkovEngine> engine_;
    std::optional<ChampionshipSimulator> simulator_;
    std::optional<StrategyReporter> reporter_;
};
