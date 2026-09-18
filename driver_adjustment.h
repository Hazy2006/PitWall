#pragma once
#include <map>

// A driver's personalization: a distribution over grid-minus-finish
// deltas (used for the shift), its mean (used for narration), and
// whether it's the driver's own data or a team-level fallback.
struct DriverAdjustment {
    double index;
    std::map<int, double> delta_distribution;
    bool is_team_fallback;
};
