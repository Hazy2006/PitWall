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

// Owns the prediction pipeline. MarkovEngine/StrategyReporter/
// ChampionshipSimulator hold const& into other members, so declaration
// order controls destruction and copy/move are disabled to protect those
// references.
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
