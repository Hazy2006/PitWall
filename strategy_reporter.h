#pragma once
#include <map>
#include <string>
#include "markov_engine.h"

// Deterministic natural-language strategy reporter. Composes short prose
// summaries purely from MarkovEngine output and per-driver performance
// indices -- no LLM, no randomness, no network calls. Graph-free and
// Storage-free by design: it only ever sees the pooled/driver-adjusted
// finish distributions and the index map, never node or persistence types.
class StrategyReporter {
private:
    const MarkovEngine& engine;
    const std::map<std::string, double>& driver_indices;

public:
    StrategyReporter(const MarkovEngine& engine, const std::map<std::string, double>& driver_indices);

    // Reports on a single grid position for a single driver. States only
    // facts the pooled/driver-adjusted distributions support: the most
    // likely finish, the shape of the distribution, and -- if driver_name
    // has a computed performance index (>=10 races) -- how that index
    // shifts the likely finish. If driver_name has no index, the report
    // says so honestly rather than fabricating an adjustment. If
    // grid_position has no pooled data at all, returns a clear "no data"
    // message instead of a fabricated report.
    std::string report_single(int grid_position, const std::string& driver_name) const;
};
