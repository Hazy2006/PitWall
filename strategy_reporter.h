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
};
