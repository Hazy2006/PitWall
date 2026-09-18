#pragma once
#include "storage.h"
#include "results_importer.h"
#include "result_row.h"
#include "markov_trainer.h"
#include "markov_engine.h"
#include "strategy_reporter.h"
#include "championship_simulator.h"
#include "driver_adjustment.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

// Owns the prediction pipeline. Members hold const& into each other, so
// declaration order controls destruction; copy/move are disabled to
// protect those references.
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

    // See StrategyReporter::compare.
    std::string compare(int grid_a, const std::string& driver_a, int grid_b, const std::string& driver_b) const;

    // See ChampionshipSimulator::simulate_championship.
    std::map<std::string, double> simulate_championship(int from_race, int num_simulations = 10000) const;

    int race_count() const;

private:
    std::vector<ResultRow> loadResults(const std::string& data_dir);
    void trainModel(const std::vector<ResultRow>& results);
    void buildReporter();

    Storage storage_;
    ResultsImporter results_importer_;
    MarkovTrainer trainer_;
    std::map<std::string, DriverAdjustment> driver_indices_;
    std::optional<MarkovEngine> engine_;
    std::optional<ChampionshipSimulator> simulator_;
    std::optional<StrategyReporter> reporter_;
};
