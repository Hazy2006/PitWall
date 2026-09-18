#include "markov_trainer.h"
#include "recency.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace {
    // Total race count, used to anchor recency decay to the most recent race.
    int total_race_count(const std::vector<ResultRow>& results) {
        int count = 0;
        for (const auto& row : results) {
            count = std::max(count, row.race_order + 1);
        }
        return count;
    }

    double recency_weight(int total_races, int race_order) {
        int k = total_races - 1 - race_order;
        return std::pow(RECENCY_LAMBDA, k);
    }

    // Mean(grid - finish) over grid != 0 rows, grouped by whichever ResultRow
    // field the caller points at -- shared by compute_driver_indices and
    // compute_team_indices.
    std::map<std::string, double> compute_indices_by(const std::vector<ResultRow>& results, std::string ResultRow::* key) {
        constexpr int kMinValidRows = 10;

        std::map<std::string, double> sum_by_key;
        std::map<std::string, int> count_by_key;

        for (const auto& row : results) {
            if (row.grid == 0) {
                continue;
            }
            const std::string& k = row.*key;
            sum_by_key[k] += static_cast<double>(row.grid - row.position);
            count_by_key[k] += 1;
        }

        std::map<std::string, double> indices;
        for (const auto& [k, count] : count_by_key) {
            if (count < kMinValidRows) {
                continue;
            }
            indices[k] = sum_by_key.at(k) / static_cast<double>(count);
        }
        return indices;
    }

    // Fixed total prior mass (not "1 per bucket"): evidence caps out around
    // 1/(1-lambda) regardless of how wide the observed delta range is, so a
    // flat per-bucket prior would dilute more the wider that range gets.
    constexpr double DELTA_PRIOR_STRENGTH = 3.0;

    // Unweighted frequency of every grid-minus-finish delta observed
    // anywhere in results -- the shape of the "assume field average"
    // prior for an entity with no evidence of its own.
    std::map<int, double> pooled_delta_frequencies(const std::vector<ResultRow>& results) {
        std::map<int, int> raw_counts;
        int total = 0;
        for (const auto& row : results) {
            if (row.grid == 0) {
                continue;
            }
            raw_counts[row.grid - row.position]++;
            total++;
        }

        std::map<int, double> frequencies;
        for (const auto& [delta, count] : raw_counts) {
            frequencies[delta] = static_cast<double>(count) / static_cast<double>(total);
        }
        return frequencies;
    }

    // Prior-plus-recency-weighted-evidence posterior, shared by both
    // delta-distribution builders below.
    std::map<std::string, std::map<int, double>> build_delta_distributions(
        const std::set<std::string>& entities,
        const std::map<int, double>& pooled_freq,
        const std::map<std::string, std::vector<std::pair<int, double>>>& evidence) {

        std::map<std::string, std::map<int, double>> distributions;
        for (const auto& entity : entities) {
            std::map<int, double> counts_for_entity;
            for (const auto& [delta, freq] : pooled_freq) {
                counts_for_entity[delta] = DELTA_PRIOR_STRENGTH * freq;
            }

            auto it = evidence.find(entity);
            if (it != evidence.end()) {
                for (const auto& [delta, weight] : it->second) {
                    counts_for_entity[delta] += weight;
                }
            }

            double total = 0.0;
            for (const auto& [delta, count] : counts_for_entity) {
                total += count;
            }

            std::map<int, double> distribution;
            for (const auto& [delta, count] : counts_for_entity) {
                distribution[delta] = count / total;
            }
            distributions[entity] = std::move(distribution);
        }
        return distributions;
    }
}

void MarkovTrainer::train(const std::vector<ResultRow>& results) {
    int total_races = total_race_count(results);

    for (const auto& row : results) {
        if (row.grid == 0) {
            continue;
        }
        counts[row.grid][row.position] += recency_weight(total_races, row.race_order);
    }
}

const std::map<int, std::map<int, double>>& MarkovTrainer::get_counts() const {
    return counts;
}

double MarkovTrainer::total_observations() const {
    double total = 0.0;
    for (const auto& [grid, finishes] : counts) {
        for (const auto& [finish, weight] : finishes) {
            total += weight;
        }
    }
    return total;
}

std::map<std::string, double> MarkovTrainer::compute_driver_indices(const std::vector<ResultRow>& results) const {
    return compute_indices_by(results, &ResultRow::driver_name);
}

std::map<std::string, double> MarkovTrainer::compute_team_indices(const std::vector<ResultRow>& results) const {
    return compute_indices_by(results, &ResultRow::team_name);
}

std::map<std::string, std::string> MarkovTrainer::compute_driver_teams(const std::vector<ResultRow>& results) const {
    std::map<std::string, std::string> driver_team;
    for (const auto& row : results) {
        driver_team[row.driver_name] = row.team_name;  // last occurrence wins
    }
    return driver_team;
}

std::map<std::string, std::map<int, double>> MarkovTrainer::compute_driver_delta_distributions(const std::vector<ResultRow>& results) const {
    int total_races = total_race_count(results);
    std::map<int, double> pooled_freq = pooled_delta_frequencies(results);

    std::set<std::string> all_drivers;
    std::map<std::string, std::vector<std::pair<int, double>>> evidence;

    for (const auto& row : results) {
        all_drivers.insert(row.driver_name);

        if (row.grid == 0) {
            continue;
        }
        int delta = row.grid - row.position;
        evidence[row.driver_name].emplace_back(delta, recency_weight(total_races, row.race_order));
    }

    return build_delta_distributions(all_drivers, pooled_freq, evidence);
}

std::map<std::string, std::map<int, double>> MarkovTrainer::compute_team_delta_distributions(const std::vector<ResultRow>& results) const {
    int total_races = total_race_count(results);
    std::map<int, double> pooled_freq = pooled_delta_frequencies(results);

    std::set<std::string> all_teams;
    std::map<std::string, std::vector<std::pair<int, double>>> evidence;

    for (const auto& row : results) {
        all_teams.insert(row.team_name);

        if (row.grid == 0) {
            continue;
        }
        int delta = row.grid - row.position;
        evidence[row.team_name].emplace_back(delta, recency_weight(total_races, row.race_order));
    }

    return build_delta_distributions(all_teams, pooled_freq, evidence);
}
