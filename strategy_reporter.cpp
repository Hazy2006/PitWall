#include "strategy_reporter.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {
    // Tie-break matches MarkovEngine::most_likely_finish: ascending finish
    // order, first strict maximum wins.
    int best_finish_of(const std::map<int, double>& dist) {
        int best_finish = -1;
        double best_prob = -1.0;
        for (const auto& [finish, prob] : dist) {
            if (prob > best_prob) {
                best_prob = prob;
                best_finish = finish;
            }
        }
        return best_finish;
    }

    // Descending by probability; ties broken by ascending finish position
    // so the report reads the same way on every call.
    std::vector<std::pair<int, double>> ranked_by_prob(const std::map<int, double>& dist) {
        std::vector<std::pair<int, double>> ranked(dist.begin(), dist.end());
        std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second) {
                return a.second > b.second;
            }
            return a.first < b.first;
            });
        return ranked;
    }

    std::string pct_str(double prob) {
        return std::to_string(static_cast<long long>(std::lround(prob * 100.0))) + "%";
    }

    // Magnitude, rounded to 1 decimal, formatted without sign.
    std::string magnitude_str(double index) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(1) << std::fabs(index);
        return out.str();
    }
}

StrategyReporter::StrategyReporter(const MarkovEngine& engine, const std::map<std::string, double>& driver_indices)
    : engine(engine), driver_indices(driver_indices) {
}

std::string StrategyReporter::report_single(int grid_position, const std::string& driver_name) const {
    std::map<int, double> pooled = engine.predict_finish_distribution(grid_position);
    if (pooled.empty()) {
        return "No historical data is available for cars starting P" + std::to_string(grid_position) + ".";
    }

    std::vector<std::pair<int, double>> ranked = ranked_by_prob(pooled);
    int top_finish = ranked[0].first;
    double top_prob = ranked[0].second;

    std::ostringstream out;
    out << "A car starting P" << grid_position << " most often finishes P" << top_finish
        << " (" << pct_str(top_prob) << " of the time historically)";

    if (ranked.size() >= 2) {
        size_t next_count = std::min<size_t>(ranked.size() - 1, 2);
        out << ", with P" << ranked[1].first << " (" << pct_str(ranked[1].second) << ")";
        if (next_count >= 2) {
            out << " and P" << ranked[2].first << " (" << pct_str(ranked[2].second) << ")";
        }
        out << " the next most likely";
    }
    out << ".";

    auto it = driver_indices.find(driver_name);
    if (it == driver_indices.end()) {
        out << " No driver-specific adjustment is available for " << driver_name
            << " (fewer than 10 races in the dataset).";
        return out.str();
    }

    double index = it->second;
    double rounded_index = std::round(index * 10.0) / 10.0;

    std::map<int, double> shifted = engine.predict_finish_distribution_for_driver(grid_position, index);
    int shifted_top = shifted.empty() ? top_finish : best_finish_of(shifted);

    out << " " << driver_name << " tends to ";
    if (rounded_index > 0.0) {
        out << "gain " << magnitude_str(rounded_index) << " positions from their grid slot";
    }
    else if (rounded_index < 0.0) {
        out << "lose " << magnitude_str(rounded_index) << " positions from their grid slot";
    }
    else {
        out << "finish in line with their grid slot historically";
    }

    if (shifted_top != top_finish) {
        out << ", shifting the likely finish toward P" << shifted_top << ".";
    }
    else {
        out << ", with the likely finish remaining at P" << shifted_top << ".";
    }

    return out.str();
}
