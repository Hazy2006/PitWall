#include "service.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
	void print_title_odds(const std::string& label, const std::map<std::string, double>& odds) {
		std::vector<std::pair<std::string, double>> sorted(odds.begin(), odds.end());
		std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
			return a.second > b.second;
			});

		std::cout << "\n-- " << label << " --\n\n";
		for (size_t i = 0; i < sorted.size() && i < 10; ++i) {
			std::cout << sorted[i].first << ": " << std::fixed << std::setprecision(1)
				<< sorted[i].second * 100.0 << "%\n";
		}
	}

	struct DemoSeason {
		std::string data_dir;
		std::string label;
		std::vector<std::pair<int, std::string>> reports;
		std::pair<int, std::string> compare_a;
		std::pair<int, std::string> compare_b;
	};

	const DemoSeason& season_2024() {
		static const DemoSeason s{
			"data", "2024",
			{ {3, "Charles Leclerc"}, {1, "Max Verstappen"}, {10, "Oliver Bearman"} },
			{3, "Charles Leclerc"}, {5, "Lando Norris"}
		};
		return s;
	}

	const DemoSeason& season_2012() {
		static const DemoSeason s{
			"data_2012", "2012",
			{ {1, "Sebastian Vettel"}, {3, "Fernando Alonso"}, {12, "Pastor Maldonado"} },
			{1, "Sebastian Vettel"}, {3, "Fernando Alonso"}
		};
		return s;
	}

	const DemoSeason& prompt_for_season() {
		std::cout << "Choose a season to run PitWall on:\n";
		std::cout << "  [1] 2024\n";
		std::cout << "  [2] 2012 (closest title fight in F1 history)\n";
		std::cout << "Enter 1 or 2 (default 1): ";

		std::string line;
		std::getline(std::cin, line);

		return line == "2" ? season_2012() : season_2024();
	}

	const DemoSeason& season_for_data_dir(const std::string& data_dir) {
		return data_dir == season_2012().data_dir ? season_2012() : season_2024();
	}

	// Evenly spread checkpoints across whatever length the season actually
	// is, instead of hardcoding race numbers that only make sense for a
	// specific season's race count.
	std::vector<int> checkpoints_for(int total_races) {
		std::vector<double> fractions = { 0.20, 0.40, 0.60, 0.80, 0.95 };
		std::vector<int> checkpoints;
		for (double frac : fractions) {
			int race = std::clamp(static_cast<int>(std::lround(frac * total_races)), 1, total_races);
			if (checkpoints.empty() || checkpoints.back() != race) {
				checkpoints.push_back(race);
			}
		}
		return checkpoints;
	}
}

int main(int argc, char** argv) {
	DemoSeason season = argc > 1 ? season_for_data_dir(argv[1]) : prompt_for_season();

	PitWallService service;
	service.load(season.data_dir);

	std::cout << "\n=== PitWall -- " << season.label << " season ===\n";

	std::cout << "\n-- Single driver reports --\n\n";
	for (const auto& [grid, driver] : season.reports) {
		std::cout << service.report(grid, driver) << "\n\n";
	}

	std::cout << "-- Driver comparison --\n\n";
	std::cout << service.compare(season.compare_a.first, season.compare_a.second,
	                              season.compare_b.first, season.compare_b.second) << "\n";

	int total_races = service.race_count();
	for (int race : checkpoints_for(total_races)) {
		print_title_odds("Title odds after race " + std::to_string(race), service.simulate_championship(race, 10000));
	}

	return 0;
}
