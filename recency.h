#pragma once

// Exponential recency decay shared by both models: a race k races back
// contributes lambda^k of evidence instead of a flat 1.0. 0.85 balances
// responsiveness to recent form against overreacting to a single race.
constexpr double RECENCY_LAMBDA = 0.85;
