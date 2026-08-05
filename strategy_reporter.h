#pragma once
#include <map>
#include <string>
#include "markov_engine.h"

// Deterministic natural-language reporter: composes prose purely from
// MarkovEngine output and driver indices -- no LLM, no randomness, no
// network calls. Graph-free and Storage-free by design -- it only ever
// sees finish distributions and the index map, never node or persistence
// types.
class StrategyReporter {
private:
    const MarkovEngine& engine;
    const std::map<std::string, double>& driver_indices;

public:
    StrategyReporter(const MarkovEngine& engine, const std::map<std::string, double>& driver_indices);

    // Reports on grid_position for driver_name using only what the pooled/
    // driver-adjusted distributions support. Falls back honestly instead of
    // fabricating: no index (<10 races) or no pooled data both get a clear
    // message rather than a made-up adjustment.
    std::string report_single(int grid_position, const std::string& driver_name) const;
};
