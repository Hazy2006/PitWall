#include "service.h"
#include "node.h"
#include <memory>

PitWallService::PitWallService() : importer_(graph_) {}

void PitWallService::load(const std::string& data_dir) {
    importData(data_dir);
    trainModel(data_dir);
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
    driver_indices_ = trainer_.compute_driver_indices(data_dir + "/results.json");
    engine_.emplace(trainer_.get_counts());
    simulator_.emplace(*engine_, data_dir + "/results.json");
}

void PitWallService::applyIndices() {
    for (const auto& [name, index] : driver_indices_) {
        int id = importer_.get_node_id(name);
        if (id < 0) {
            continue;
        }
        auto driver_node = std::dynamic_pointer_cast<DriverNode>(graph_.get_node(id));
        if (driver_node) {
            driver_node->base_pace_delta = index;
        }
    }
}

void PitWallService::buildReporter() {
    reporter_.emplace(*engine_, driver_indices_);
}

std::string PitWallService::report(int grid_position, const std::string& driver_name) const {
    return reporter_->report_single(grid_position, driver_name);
}

std::map<std::string, double> PitWallService::simulate_championship(int from_race, int num_simulations) const {
    return simulator_->simulate_championship(from_race, num_simulations);
}
