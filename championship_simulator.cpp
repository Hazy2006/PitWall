#include "championship_simulator.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <set>

using json = nlohmann::json;

namespace {
    json load_json_array(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        json data;
        file >> data;
        return data;
    }

    double points_for_position(int position) {
        static const std::map<int, double> table = {
            {1, 25.0}, {2, 18.0}, {3, 15.0}, {4, 12.0}, {5, 10.0},
            {6, 8.0}, {7, 6.0}, {8, 4.0}, {9, 2.0}, {10, 1.0}
        };
        auto it = table.find(position);
        return it == table.end() ? 0.0 : it->second;
    }
}

ChampionshipSimulator::ChampionshipSimulator(const MarkovEngine& engine,
                                              const std::string& results_json_path,
                                              unsigned int seed)
    : engine_(engine), rng_(seed) {
    load_results(results_json_path);
}

void ChampionshipSimulator::load_results(const std::string& results_json_path) {
    json data = load_json_array(results_json_path);

    std::map<std::string, int> race_index_by_circuit;
    std::set<std::string> seen_drivers;
    std::map<std::string, double> grid_sum;
    std::map<std::string, int> grid_count;

    for (const auto& entry : data) {
        RaceResult r;
        r.driver_name = entry.at("driver_name").get<std::string>();
        r.circuit_name = entry.at("circuit_name").get<std::string>();
        r.position = entry.at("position").get<int>();
        r.grid = entry.at("grid").get<int>();
        results_.push_back(r);

        if (race_index_by_circuit.find(r.circuit_name) == race_index_by_circuit.end()) {
            race_index_by_circuit[r.circuit_name] = static_cast<int>(race_order_.size());
            race_order_.push_back(r.circuit_name);
        }

        if (seen_drivers.insert(r.driver_name).second) {
            all_drivers_.push_back(r.driver_name);
        }

        if (r.grid != 0) {
            grid_sum[r.driver_name] += r.grid;
            grid_count[r.driver_name] += 1;
        }
    }

    // Second pass, after results_ has stopped growing: races_ stores pointers
    // into results_ elements, which would be invalidated by any reallocation.
    races_.assign(race_order_.size(), {});
    for (const auto& r : results_) {
        int race_index = race_index_by_circuit.at(r.circuit_name);
        races_[race_index].push_back(&r);
    }

    for (const auto& driver : all_drivers_) {
        auto count_it = grid_count.find(driver);
        if (count_it == grid_count.end() || count_it->second == 0) {
            continue;  // never had a valid (non-pit-lane) grid slot
        }
        double mean = grid_sum.at(driver) / static_cast<double>(count_it->second);
        avg_grid_rounded_[driver] = static_cast<int>(std::lround(mean));
    }
}

int ChampionshipSimulator::race_count() const {
    return static_cast<int>(race_order_.size());
}

std::map<std::string, double> ChampionshipSimulator::points_through_race(int through_race) const {
    std::map<std::string, double> points;
    for (const auto& driver : all_drivers_) {
        points[driver] = 0.0;
    }

    int race_limit = std::min(through_race, static_cast<int>(races_.size()));
    for (int i = 0; i < race_limit; ++i) {
        for (const RaceResult* r : races_[i]) {
            points[r->driver_name] += points_for_position(r->position);
        }
    }
    return points;
}

std::map<std::string, double> ChampionshipSimulator::simulate_championship(int from_race, int num_simulations) const {
    struct DriverSampler {
        std::vector<int> finishes;  // finish positions, in engine's map order
        std::discrete_distribution<int> dist;  // samples an index into finishes
    };

    // Built once per call, not once per simulated race: a driver's average
    // grid (and thus their pooled distribution) never changes across races.
    std::map<std::string, DriverSampler> samplers;
    for (const auto& driver : all_drivers_) {
        auto grid_it = avg_grid_rounded_.find(driver);
        if (grid_it == avg_grid_rounded_.end()) {
            continue;  // no valid average grid -- skipped in every remaining race
        }
        std::map<int, double> distribution = engine_.predict_finish_distribution(grid_it->second);
        if (distribution.empty()) {
            continue;  // engine has no pooled data for this grid -- skipped
        }

        DriverSampler sampler;
        std::vector<double> weights;
        for (const auto& [finish, prob] : distribution) {
            sampler.finishes.push_back(finish);
            weights.push_back(prob);
        }
        sampler.dist = std::discrete_distribution<int>(weights.begin(), weights.end());
        samplers.emplace(driver, std::move(sampler));
    }

    std::map<std::string, double> starting_points = points_through_race(from_race);
    int total_races = race_count();

    std::map<std::string, double> win_credit;
    for (const auto& driver : all_drivers_) {
        win_credit[driver] = 0.0;
    }

    for (int sim = 0; sim < num_simulations; ++sim) {
        std::map<std::string, double> sim_points = starting_points;

        for (int race = from_race; race < total_races; ++race) {
            std::vector<std::pair<std::string, int>> sampled;
            sampled.reserve(samplers.size());
            for (auto& [driver, sampler] : samplers) {
                int idx = sampler.dist(rng_);
                sampled.emplace_back(driver, sampler.finishes[idx]);
            }

            // Shuffle first, then a *stable* sort by sampled finish: ties in
            // the sampled finish keep the shuffled (i.e. random) relative
            // order instead of insertion order, which is how tie-breaking is
            // randomized without violating strict-weak-ordering during sort.
            std::shuffle(sampled.begin(), sampled.end(), rng_);
            std::stable_sort(sampled.begin(), sampled.end(),
                [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                    return a.second < b.second;
                });

            for (size_t i = 0; i < sampled.size(); ++i) {
                int assigned_position = static_cast<int>(i) + 1;
                sim_points[sampled[i].first] += points_for_position(assigned_position);
            }
        }

        double best_points = -1.0;
        std::vector<std::string> leaders;
        for (const auto& [driver, pts] : sim_points) {
            if (pts > best_points) {
                best_points = pts;
                leaders.clear();
                leaders.push_back(driver);
            }
            else if (pts == best_points) {
                leaders.push_back(driver);
            }
        }

        // An exact tie for the season lead splits this simulation's win
        // credit evenly, so the returned fractions always sum to 1.0.
        double credit = 1.0 / static_cast<double>(leaders.size());
        for (const auto& driver : leaders) {
            win_credit[driver] += credit;
        }
    }

    std::map<std::string, double> result;
    for (const auto& driver : all_drivers_) {
        result[driver] = win_credit[driver] / static_cast<double>(num_simulations);
    }
    return result;
}
