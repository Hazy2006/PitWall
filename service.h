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

// Owns the full prediction pipeline. MarkovEngine holds a const& to
// MarkovTrainer's transition-count map, StrategyReporter holds const& to
// both the engine and the driver-index map, and ChampionshipSimulator holds
// a const& to the engine too -- none of that is safe unless one owner
// controls every lifetime involved and never lets them outlive each other.
// PitWallService is that owner: graph_, trainer_, driver_indices_,
// importer_, engine_, simulator_, and reporter_ are declared in that order,
// so members are destroyed in reverse -- simulator_ and reporter_ are torn
// down before the engine/trainer/indices they reference -- and the class is
// non-copyable/non-movable so those internal references can never end up
// bound to a different object's members. engine_, simulator_, and
// reporter_ are std::optional because none of them can be constructed
// until trainer_ has run (and, for simulator_/reporter_, until engine_
// exists).
class PitWallService {
public:
    PitWallService();

    PitWallService(const PitWallService&) = delete;
    PitWallService& operator=(const PitWallService&) = delete;
    PitWallService(PitWallService&&) = delete;
    PitWallService& operator=(PitWallService&&) = delete;

    // Runs the entire pipeline: imports drivers/teams/circuits/results from
    // data_dir, trains the Markov model, computes and applies driver
    // indices, and builds the reporter and championship simulator. The
    // service is fully ready after this call returns.
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
