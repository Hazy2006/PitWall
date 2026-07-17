#include <iostream>

void run_domain_tests();
void run_edge_tests();

int main() {
	std::cout << "=== Running PitWall Test Suite ===\n";
	run_domain_tests();
	std::cout << "==================================\n";
	std::cout << "=== Running PitWall Test Suite ===\n";
	run_edge_tests();
	std::cout << "==================================\n";
	return 0;
}