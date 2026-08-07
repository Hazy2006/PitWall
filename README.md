# PitWall

---

A graph engine and Formula 1 race outcome predictor built from scratch in C++.

## What

---

PitWall trains a Markov model on historical Formula 1 race results to answer one question:

> Given a driver's starting grid position, where are they most likely to finish?

The project imports the 2024 Formula 1 season from the Jolpica F1 API, builds a historical grid → finish probability model, adjusts predictions using each driver's average positions gained or lost, and generates deterministic natural-language reports.

The current version predicts finishing position only. It does **not** model pit strategy, tire degradation, or lap-by-lap race evolution.

---

## Example Output

---

### Charles Leclerc — Starting P3

```
A car starting P3 most often finishes P2 (21%), with
P3 (21%) and P5 (17%) the next most likely outcomes.

Leclerc tends to gain 0.9 positions from his grid slot,
shifting the prediction toward P1.
```

### Oliver Bearman — Starting P10

```
A car starting P10 most often finishes P9 (13%), with
P10 (13%) and P12 (13%) the next most likely outcomes.

No driver-specific adjustment is available because
Bearman has fewer than 10 races in the dataset.
```

---

## Tech Stack

---

- **Language:** C++ (MSVC), Python
- **Persistence:** SQLite (raw C API)
- **JSON:** nlohmann/json
- **Dependency Manager:** vcpkg
- **Development:** Visual Studio
- **Testing:** Test-Driven Development (TDD)

---

## Architecture

---

PitWall follows a strict layered architecture.

```
Persistence stack:          Markov stack (standalone):
  Repository                   MarkovTrainer
     |                            |
  Storage                     MarkovEngine
     |                            |
  SQLite                      StrategyReporter

  Graph (domain) ← DataImporter fills it
```

### Domain

- Polymorphic `Node` hierarchy
  - `DriverNode`
  - `TeamNode`
  - `CircuitNode`
- `Edge`
- `Graph`
    - Dual adjacency maps
    - O(1) in/out degree lookups

### Repository

Translates between domain objects and SQLite rows.

### Storage

Executes raw SQLite statements with no knowledge of domain objects.

The Markov engine is intentionally independent of both the graph and persistence layers.

---

## Markov Model

---

Training consists of four stages.

1. Count every historical transition from grid position to finishing position.
2. Convert counts into probability distributions at query time.
3. Compute each driver's average positions gained (`grid - finish`) for drivers with at least ten races.
4. Shift the pooled probability distribution using that driver index.

The reporter converts these probabilities into deterministic natural-language output.

---

## Building

---

Requirements:

- Visual Studio
- MSVC
- vcpkg
- nlohmann/json

Build:

```text
PitWall.slnx
```

Target:

```text
PitWallEngine (Debug / x64)
```

To refresh the dataset:

```bash
python scripts/fetch_f1_data.py
```

This downloads:

```
data/
├── drivers.json
├── teams.json
├── circuits.json
└── results.json
```

---

## Limitations

---

- The model uses only the 2024 Formula 1 season.
- Driver adjustments require at least 10 races.
- Driver performance is represented by a single average position gain/loss.
- Large positive adjustments naturally saturate at P1.
- No lap-level simulation.
- No tire degradation model.
- No pit strategy optimization.
- No safety car simulation.

---

## Roadmap

---

- [x] Graph engine
- [x] SQLite persistence
- [x] Jolpica data import pipeline
- [x] Grid → finish Markov model
- [x] Driver performance adjustment
- [x] Natural-language prediction reports
- [x] Championship Monte Carlo simulator
- [ ] Driver vs. Driver comparison reports
- [ ] Lap-level race simulation
