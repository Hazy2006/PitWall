# PitWall

A graph database engine and F1 strategic intelligence system built from scratch in C++.

## What

PitWall models Formula 1 as a graph — drivers, teams, and circuits as nodes; historical relationships, win rates, and performance data as edges. On top of this, a Markov chain engine estimates race outcomes and computes optimal strategies in natural language.

The end goal: a system that tells you *why* Ferrari should pit at lap 34, not just that they should.

## Tech Stack

- **Language**: C++ (primary), Python
- **Persistence**: SQLite via raw C API (`sqlite3.h`)
- **Config/lightweight data**: JSON
- **IDE**: Visual Studio
- **Testing**: TDD from day one — tests written before implementation, driving the design forward

## Core Architecture

### Layering

PitWall follows a strict layered architecture:

- **Domain** — `Node` hierarchy (polymorphic: `DriverNode`, `TeamNode`, `CircuitNode`), `Edge` struct, `Graph` class (hash maps + adjacency lists with both successors and predecessors maps)
- **Repository** — domain-aware data access layer, translates between domain objects and raw storage
- **Storage** — raw SQLite execution, no domain awareness — Repository is the only caller
- **Service** — orchestrates Repository and the Markov engine; never touches Storage directly

Call chain: `Service → Repository → Storage → SQLite`

### Graph Engine

- Dual adjacency maps (successors + predecessors) for O(1) in/out-degree lookups
- `Node` subclasses hold only Markov-relevant mathematical coefficients — display data (nationality, logos, photos) lives in SQLite and gets joined at render time by the UI layer
- `Edge` struct with named weight fields: `winRate`, `affinityScore`, `performanceDelta`, `tirePreference`
- Nodes are **immutable** once constructed — populated via constructor when the Repository builds the graph
- Runtime-changing variables (current tire age, track temperature, fuel load) live in a separate `SimulationState` object passed through the Markov engine, never mutating the base graph

### Circuit Database

- Per-circuit data with layout characteristics and historical race results stored in SQLite
- Circuit photos stored as files on disk; paths referenced in the database

### Markov Chain Layer

- **Trainer** — normalizes historical race data into transition probabilities
- **Engine** — runs queries against the trained model at runtime
- Pipeline: `Raw data source → Trainer (normalize counts) → Storage (JSON/DB) → Load → Engine → Client request → Output`
- Transition probability estimation from historical data; models driver performance, tire degradation windows, championship scenarios

### Strategy Report Generator

- LLM layer translates numerical output into actionable team recommendations
- **Observer pattern**: race state changes trigger automatic recomputation of strategy recommendations

## Example Outputs

```
> Ferrari should pit Leclerc at lap 34 — medium degradation on this circuit
  historically drops off after lap 33, and Hamilton is 4s behind on fresher tires.

> McLaren must win Barcelona or lose ~15% championship probability.
  Current trajectory puts Norris P3 with no buffer against Verstappen.

> Red Bull should run hard compounds at Abu Dhabi — Tsunoda's lap consistency
  on this circuit improves significantly on harder compounds after lap 20.

> Mercedes defensive strategy recommended — attack probability of success
  given current gap and tire delta is 23%. Hold position.
```

## Graph Model

| Node type | Examples | Markov-relevant fields |
|---|---|---|
| Driver | Leclerc, Verstappen, Norris | `tire_management_modifier`, `base_pace_delta` |
| Team | Ferrari, Red Bull, McLaren | `strategy_aggressiveness` |
| Circuit | Monaco, Interlagos, Abu Dhabi | `base_degradation_rate`, `pit_lane_time_loss` |

Edges encode: historical win rates, driver-circuit affinity scores, team performance deltas, tire compound preferences.

> Example: Leclerc → Monaco has a higher win-transition probability than Leclerc → Interlagos,
> reflecting both familiarity and historical performance.

## Markov Chain Model

State = `(driver, circuit, lap, tire compound, position)`

Transition probabilities estimated from historical F1 data (Jolpica-F1 API, OpenF1). The model answers:
- At which lap does switching compounds maximize P(win)?
- Given current state, what is each driver's championship probability?
- How does a safety car at lap X shift the outcome distribution?

**Scope**: simplified state space — cognitive and FIA regulation variables are out of scope for V1.

## Data Sources

- Jolpica-F1 API, OpenF1 — full historical race data, free
- Circuit photos and layout files on disk, paths stored in SQLite

## Roadmap

- [x] Core graph engine — polymorphic node hierarchy, edge struct, add/remove nodes and edges
- [x] TDD test suite — domain layer tests passing
- [x] Graph traversal — BFS, DFS, Dijkstra query support
- [x] SQLite integration — circuit database, persistence layer
- [x] Jolpica/OpenF1 data import pipeline
- [ ] Markov chain transition probability estimation
- [ ] Championship scenario simulator
- [ ] Pit stop window optimizer
- [ ] Observer pattern — strategy recomputation on state change
- [ ] LLM strategy report generator
- [ ] Markov chain extension for Semester 1 probability course

## Timeline

July 9th, 2026 onwards

## Future

PitWall's graph + probabilistic query pattern extends naturally to other domains —
including artifact relationship modeling for interdisciplinary research platforms.
