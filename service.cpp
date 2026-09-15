#include "service.h"
#include "node.h"
#include "path_utils.h"
#include <memory>

PitWallService::PitWallService() : importer_(graph_) {}

void PitWallService::load(const std::string& data_dir) {
    std::string resolved_dir = resolve_repo_path(data_dir);
    importData(resolved_dir);
    trainModel(resolved_dir);
    applyIndices();
    buildReporter();
}

void PitWallService::importData(const std::string& data_dir) {
    importer_.import_drivers(data_dir + "/drivers.json");
    importer_.import_teams(data_dir + "/teams.json");
    importer_.import_circuits(data_dir + "/circuits.json");
    importer_.import_results(data_dir + "/results.json");
}

void PitWallService::trainModel(const std::string& data_dir) {
    trainer_.train(data_dir + "/results.json");

    std::map<std::string, double> driver_only = trainer_.compute_driver_indices(data_dir + "/results.json");
    std::map<std::string, double> team_only = trainer_.compute_team_indices(data_dir + "/results.json");
    std::map<std::string, std::string> driver_teams = trainer_.compute_driver_teams(data_dir + "/results.json");

    driver_indices_.clear();
    for (const auto& [driver, team] : driver_teams) {
        auto driver_it = driver_only.find(driver);
        if (driver_it != driver_only.end()) {
            driver_indices_[driver] = { driver_it->second, false };
            continue;
        }
        // Driver has fewer than 10 personal races -- fall back to their
        // team's pooled index instead of leaving them unpersonalized.
        auto team_it = team_only.find(team);
        if (team_it != team_only.end()) {
            driver_indices_[driver] = { team_it->second, true };
        }
    }

    engine_.emplace(trainer_.get_counts());
    simulator_.emplace(data_dir + "/results.json");
}

void PitWallService::applyIndices() {
    for (const auto& [name, adjustment] : driver_indices_) {
        int id = importer_.get_node_id(name);
        if (id < 0) {
            continue;
        }
        auto driver_node = std::dynamic_pointer_cast<DriverNode>(graph_.get_node(id));
        if (driver_node) {
            driver_node->base_pace_delta = adjustment.index;
        }
    }
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
