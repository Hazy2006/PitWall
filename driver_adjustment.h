#pragma once

// A driver's grid-to-finish shift, plus where it came from. is_team_fallback
// is true when the driver has fewer than 10 personal races on record and
// this index is their team's pooled average instead of their own.
struct DriverAdjustment {
    double index;
    bool is_team_fallback;
};
