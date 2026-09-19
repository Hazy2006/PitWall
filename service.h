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

    // Debug/experimentation hook: runs one raw SQL statement against the
    // season's persistent database and returns any rows it produced (empty
    // for non-SELECT statements). Lets an interactive console edit data
    // without an external SQL tool.
    std::vector<std::map<std::string, std::string>> run_sql(const std::string& sql);

    // Recomputes every prediction from the database's current contents,
    // without touching JSON or re-importing. Call after run_sql() edits
    // something you want reflected in report()/compare()/simulate_championship().
    void reload();

private:
    std::vector<ResultRow> loadResults(const std::string& season_name, const std::string& resolved_dir);
    void trainModel(const std::vector<ResultRow>& results);
    void buildReporter();

    // One real file per season (see db_path_for() in service.cpp), not
    // :memory: -- results.json only gets imported the first time a season's
    // file doesn't exist yet; every run after that reuses whatever is
    // currently in that file, edits included.
    std::optional<Storage> storage_;
    std::optional<ResultsImporter> results_importer_;
    // Rebuilt fresh in trainModel() every load()/reload() -- MarkovTrainer
    // accumulates into its transition table across train() calls rather
    // than replacing it, so reusing one instance across reloads would blend
    // pre-edit and post-edit data together permanently.
    std::optional<MarkovTrainer> trainer_;
    std::map<std::string, DriverAdjustment> driver_indices_;
    std::optional<MarkovEngine> engine_;
    std::optional<ChampionshipSimulator> simulator_;
    std::optional<StrategyReporter> reporter_;
};
