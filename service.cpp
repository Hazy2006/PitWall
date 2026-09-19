#include "service.h"
#include "path_utils.h"

namespace {
    double mean_of(const std::map<int, double>& distribution) {
        double mean = 0.0;
        for (const auto& [delta, prob] : distribution) {
            mean += delta * prob;
        }
        return mean;
    }

    // One real file per season name (e.g. "data" -> pitwall_data.db,
    // "data_2012" -> pitwall_data_2012.db), so editing one season's data can
    // never leak into another's.
    std::string db_path_for(const std::string& season_name) {
        std::string sanitized = season_name;
        for (char& c : sanitized) {
            if (c == '/' || c == '\\') {
                c = '_';
            }
        }
        return "pitwall_" + sanitized + ".db";
    }
}

PitWallService::PitWallService() {}

void PitWallService::load(const std::string& data_dir) {
    std::string resolved_dir = resolve_repo_path(data_dir);
    std::vector<ResultRow> results = loadResults(data_dir, resolved_dir);
    trainModel(results);
    buildReporter();
}

std::vector<ResultRow> PitWallService::loadResults(const std::string& season_name, const std::string& resolved_dir) {
    storage_.emplace(db_path_for(season_name));
    results_importer_.emplace(*storage_);

    results_importer_->create_tables();
    results_importer_->import_from_json(resolved_dir);
    return results_importer_->load_results();
}

void PitWallService::trainModel(const std::vector<ResultRow>& results) {
    trainer_.emplace();
    trainer_->train(results);

    std::map<std::string, double> driver_only = trainer_->compute_driver_indices(results);
    std::map<std::string, double> team_only = trainer_->compute_team_indices(results);
    std::map<std::string, std::string> driver_teams = trainer_->compute_driver_teams(results);
    std::map<std::string, std::map<int, double>> driver_deltas = trainer_->compute_driver_delta_distributions(results);
    std::map<std::string, std::map<int, double>> team_deltas = trainer_->compute_team_delta_distributions(results);

    driver_indices_.clear();
    for (const auto& [driver, team] : driver_teams) {
        auto driver_it = driver_only.find(driver);
        if (driver_it != driver_only.end()) {
            const auto& dist = driver_deltas.at(driver);
            driver_indices_[driver] = { mean_of(dist), dist, false };
            continue;
        }
        // Driver has fewer than 10 personal races -- fall back to their
        // team's pooled distribution instead of leaving them unpersonalized.
        auto team_it = team_only.find(team);
        if (team_it != team_only.end()) {
            const auto& dist = team_deltas.at(team);
            driver_indices_[driver] = { mean_of(dist), dist, true };
        }
    }

    engine_.emplace(trainer_->get_counts());
    simulator_.emplace(results);
}

void PitWallService::buildReporter() {
    reporter_.emplace(*engine_, driver_indices_);
}

std::string PitWallService::report(int grid_position, const std::string& driver_name) const {
    return reporter_->report_single(grid_position, driver_name);
}

std::string PitWallService::compare(int grid_a, const std::string& driver_a, int grid_b, const std::string& driver_b) const {
    return reporter_->compare(grid_a, driver_a, grid_b, driver_b);
}

std::map<std::string, double> PitWallService::simulate_championship(int from_race, int num_simulations) const {
    return simulator_->simulate_championship(from_race, num_simulations);
}

int PitWallService::race_count() const {
    return simulator_->race_count();
}

std::vector<std::map<std::string, std::string>> PitWallService::run_sql(const std::string& sql) {
    return storage_->query(sql);
}

void PitWallService::reload() {
    std::vector<ResultRow> results = results_importer_->load_results();
    trainModel(results);
    buildReporter();
}
