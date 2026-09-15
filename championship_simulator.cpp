#include "championship_simulator.h"
#include "json_utils.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <set>

using json = nlohmann::json;

namespace {
    double points_for_position(int position) {
        static const std::map<int, double> table = {
            {1, 25.0}, {2, 18.0}, {3, 15.0}, {4, 12.0}, {5, 10.0},
            {6, 8.0}, {7, 6.0}, {8, 4.0}, {9, 2.0}, {10, 1.0}
        };
        auto it = table.find(position);
        return it == table.end() ? 0.0 : it->second;
    }
}

ChampionshipSimulator::DriverSampler ChampionshipSimulator::make_sampler(const std::map<int, double>& distribution) {
    DriverSampler sampler;
    std::vector<double> weights;
    weights.reserve(distribution.size());
    for (const auto& [finish, prob] : distribution) {
        sampler.finishes.push_back(finish);
        weights.push_back(prob);
    }
    sampler.dist = std::discrete_distribution<int>(weights.begin(), weights.end());
    return sampler;
}

ChampionshipSimulator::ChampionshipSimulator(const std::string& results_json_path,
                                              unsigned int seed)
    : dirichlet_model_(results_json_path), rng_(seed) {
    load_results(results_json_path);
}

void ChampionshipSimulator::load_results(const std::string& results_json_path) {
    json data = load_json_array(results_json_path);

    std::map<std::string, int> race_index_by_circuit;
    std::set<std::string> seen_drivers;

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
    }

    // Second pass, after results_ has stopped growing: races_ stores pointers
    // into results_ elements, which would be invalidated by any reallocation.
    races_.assign(race_order_.size(), {});
    for (const auto& r : results_) {
        int race_index = race_index_by_circuit.at(r.circuit_name);
        races_[race_index].push_back(&r);
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

std::map<std::string, ChampionshipSimulator::DriverSampler> ChampionshipSimulator::build_bayesian_samplers(int from_race) const {
    std::map<std::string, DriverSampler> samplers;
    for (const auto& driver : all_drivers_) {
        std::map<int, double> distribution = dirichlet_model_.driver_finish_distribution(driver, from_race);
        if (distribution.empty()) {
            continue;
        }
        samplers.emplace(driver, make_sampler(distribution));
    }
    return samplers;
}

std::map<std::string, double> ChampionshipSimulator::simulate_championship(int from_race, int num_simulations) const {
    std::map<std::string, double> starting_points = points_through_race(from_race);
    int total_races = race_count();
    double max_remaining_points = static_cast<double>(total_races - from_race) * 25.0;

    std::map<std::string, double> result;
    for (const auto& driver : all_drivers_) {
        result[driver] = 0.0;
    }

    // Leader = highest current points; first driver in first-seen order wins
    // ties, matching the tie-handling used for simulated seasons below.
    std::string leader;
    double leader_points = -1.0;
    for (const auto& driver : all_drivers_) {
        double pts = starting_points.at(driver);
        if (pts > leader_points) {
            leader_points = pts;
            leader = driver;
        }
    }
    if (leader.empty()) {
        return result;
    }

    // Best of everyone else, not just whoever happens to sort second.
    double best_chaser_points = -1.0;
    for (const auto& driver : all_drivers_) {
        if (driver == leader) {
            continue;
        }
        best_chaser_points = std::max(best_chaser_points, starting_points.at(driver));
    }

    // CLINCH: the leader's lead already exceeds the best chaser's maximum
    // possible remaining haul -- no simulation can change the outcome.
    if (leader_points > best_chaser_points + max_remaining_points) {
        result[leader] = 1.0;
        return result;
    }

    // ELIMINATION: a driver who can't catch the leader even with a perfect
    // remaining season can never legitimately win a simulated one either.
    std::set<std::string> eliminated;
    for (const auto& driver : all_drivers_) {
        if (starting_points.at(driver) + max_remaining_points < leader_points) {
            eliminated.insert(driver);
        }
    }

    // Built once per call, not once per simulated race: each driver's finish
    // distribution (Dirichlet posterior through from_race) never changes
    // across the remaining races being simulated.
    std::map<std::string, DriverSampler> named_samplers = build_bayesian_samplers(from_race);

    // Flatten into index-aligned vectors so the inner simulation loop never
    // allocates or compares through a string-keyed container.
    size_t n = named_samplers.size();
    std::vector<std::string> sim_drivers;
    std::vector<DriverSampler> samplers;
    std::vector<double> sim_points_base;
    std::vector<char> is_eliminated;
    sim_drivers.reserve(n);
    samplers.reserve(n);
    sim_points_base.reserve(n);
    is_eliminated.reserve(n);

    for (auto& [driver, sampler] : named_samplers) {
        sim_drivers.push_back(driver);
        samplers.push_back(std::move(sampler));
        sim_points_base.push_back(starting_points.at(driver));
        is_eliminated.push_back(eliminated.count(driver) != 0);
    }

    std::vector<double> win_credit(n, 0.0);
    std::vector<double> sim_points(n);
    std::vector<int> finish_order(n);

    for (int sim = 0; sim < num_simulations; ++sim) {
        sim_points = sim_points_base;

        for (int race = from_race; race < total_races; ++race) {
            std::vector<int> sampled_finish(n);
            for (size_t i = 0; i < n; ++i) {
                int idx = samplers[i].dist(rng_);
                sampled_finish[i] = samplers[i].finishes[idx];
            }

            // Shuffle, then stable-sort by finish: ties keep the shuffled
            // (random) order instead of insertion order -- randomizes
            // tie-breaks without violating strict-weak-ordering.
            for (size_t i = 0; i < n; ++i) {
                finish_order[i] = static_cast<int>(i);
            }
            std::shuffle(finish_order.begin(), finish_order.end(), rng_);
            std::stable_sort(finish_order.begin(), finish_order.end(),
                [&sampled_finish](int a, int b) {
                    return sampled_finish[a] < sampled_finish[b];
                });

            for (size_t pos = 0; pos < finish_order.size(); ++pos) {
                int assigned_position = static_cast<int>(pos) + 1;
                sim_points[finish_order[pos]] += points_for_position(assigned_position);
            }
        }

        double best_points = -1.0;
        std::vector<size_t> leaders;
        for (size_t i = 0; i < n; ++i) {
            // Guards against ever crediting an eliminated driver, though it's
            // mathematically impossible for one to hold the best total here.
            if (is_eliminated[i]) {
                continue;
            }
            if (sim_points[i] > best_points) {
                best_points = sim_points[i];
                leaders.clear();
                leaders.push_back(i);
            }
            else if (sim_points[i] == best_points) {
                leaders.push_back(i);
            }
        }

        // An exact tie for the season lead splits this simulation's win
        // credit evenly, so the returned fractions always sum to 1.0.
        double credit = 1.0 / static_cast<double>(leaders.size());
        for (size_t i : leaders) {
            win_credit[i] += credit;
        }
    }

    for (size_t i = 0; i < n; ++i) {
        result[sim_drivers[i]] = win_credit[i] / static_cast<double>(num_simulations);
    }
    return result;
}
