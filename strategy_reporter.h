#pragma once
#include <map>
#include <string>
#include "markov_engine.h"

// Graph-free and Storage-free by design.
class StrategyReporter {
private:
    const MarkovEngine& engine;
    const std::map<std::string, double>& driver_indices;

public:
    StrategyReporter(const MarkovEngine& engine, const std::map<std::string, double>& driver_indices);

    std::string report_single(int grid_position, const std::string& driver_name) const;

    // Compares two drivers starting from (possibly different) grid
    // positions by expected finish (probability-weighted average finishing
    // position, lower is better). Uses each driver's driver-aware
    // distribution if they have an index, pooled otherwise -- same honest
    // fallback as report_single for drivers with fewer than 10 races.
    std::string compare(int grid_a, const std::string& driver_a, int grid_b, const std::string& driver_b) const;
};
