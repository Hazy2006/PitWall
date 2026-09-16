#pragma once
#include <string>

// One imported race result row: the shared type every prediction model
// consumes instead of a JSON path.
struct ResultRow {
    std::string driver_name;
    std::string team_name;
    std::string circuit_name;
    int position;
    int grid;
    int race_order;
};
