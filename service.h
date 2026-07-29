#pragma once
#include "graph.h"
#include "data_importer.h"
#include "markov_trainer.h"
#include "markov_engine.h"
#include "strategy_reporter.h"
#include <map>
#include <optional>
#include <string>

// Owns the full prediction pipeline. MarkovEngine holds a const& to
// MarkovTrainer's transition-count map, and StrategyReporter holds const&
// to both the engine and the driver-index map -- none of that is safe
// unless one owner controls every lifetime involved and never lets them
// outlive each other. PitWallService is that owner: graph_, trainer_,
// driver_indices_, importer_, engine_, and reporter_ are declared in that
// order, so members are destroyed in reverse -- reporter_ and engine_ are
// torn down before the trainer/indices they reference -- and the class is
// non-copyable/non-movable so those internal references can never end up
// bound to a different object's members. engine_ and reporter_ are
// std::optional because MarkovEngine can't be constructed until trainer_
// has run, and StrategyReporter can't be constructed until engine_ exists.
class PitWallService {
public:
    PitWallService();

    PitWallService(const PitWallService&) = delete;
    PitWallService& operator=(const PitWallService&) = delete;
    PitWallService(PitWallService&&) = delete;
    PitWallService& operator=(PitWallService&&) = delete;

    // Runs the entire pipeline: imports drivers/teams/circuits/results from
    // data_dir, trains the Markov model, computes and applies driver
    // indices, and builds the reporter. The service is fully ready after
    // this call returns.
    void load(const std::string& data_dir);

    std::string report(int grid_position, const std::string& driver_name) const;

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
    std::optional<StrategyReporter> reporter_;
};
