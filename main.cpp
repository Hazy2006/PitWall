#include "service.h"
#include <iostream>

int main() {
	PitWallService service;
	service.load("data");

	std::cout << service.report(3, "Charles Leclerc") << "\n";
	std::cout << service.report(1, "Max Verstappen") << "\n";
	std::cout << service.report(10, "Oliver Bearman") << "\n";

	return 0;
}
