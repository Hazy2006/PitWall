#include "service.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
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
}

int main() {
	PitWallService service;
	service.load("data");

	std::cout << "=== PitWall ===\n";

	std::cout << "\n-- Single driver reports --\n\n";
	std::cout << service.report(3, "Charles Leclerc") << "\n";
	std::cout << service.report(1, "Max Verstappen") << "\n";
	std::cout << service.report(10, "Oliver Bearman") << "\n";

	print_title_odds("Title odds after race 5", service.simulate_championship(5, 10000));
	print_title_odds("Title odds after race 10", service.simulate_championship(10, 10000));
	print_title_odds("Title odds after race 15", service.simulate_championship(15, 10000));
	print_title_odds("Title odds after race 20", service.simulate_championship(20, 10000));
	print_title_odds("Title odds after race 23", service.simulate_championship(23, 10000));

	return 0;
}
