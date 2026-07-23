#include <iostream>

void run_domain_tests();
void run_edge_tests();
void test_bfs();
void test_dfs();
void test_remove_node();
void test_dijkstra();
void test_save_and_load();
void test_save_and_load_with_id_gap();
void test_data_importer();
void test_real_import();

int main() {
	std::cout << "=== Running PitWall Test Suite ===\n";
	run_domain_tests();
	std::cout << "==================================\n";
	std::cout << "=== Running PitWall Test Suite ===\n";
	run_edge_tests();
	std::cout << "==================================\n";
	std::cout << "==================================\n";
	test_bfs();
	std::cout << "==================================\n";
	test_dfs();
	std::cout << "==================================\n";
	test_remove_node();
	std::cout << "==================================\n";
	test_dijkstra();
	std::cout << "==================================\n";
	test_save_and_load();
	std::cout << "==================================\n";
	test_save_and_load_with_id_gap();
	std::cout << "==================================\n";
	test_data_importer();
	std::cout << "==================================\n";
	test_real_import();
	return 0;
}