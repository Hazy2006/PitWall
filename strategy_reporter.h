#pragma once
#include <map>
#include <string>
#include "markov_engine.h"
#include "driver_adjustment.h"

// Graph-free and Storage-free by design.
class StrategyReporter {
private:
    const MarkovEngine& engine;
    const std::map<std::string, DriverAdjustment>& driver_indices;

public:
    StrategyReporter(const MarkovEngine& engine, const std::map<std::string, DriverAdjustment>& driver_indices);

    std::string report_single(int grid_position, const std::string& driver_name) const;

    // Compares two drivers by expected finish (probability-weighted
    // average position, lower is better), using each driver's index if
    // available.
    std::string compare(int grid_a, const std::string& driver_a, int grid_b, const std::string& driver_b) const;
};
