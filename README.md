# PitWall

A graph database engine and F1 strategic intelligence system built from scratch in C++.

## What

PitWall models Formula 1 as a graph — drivers, teams, circuits, and strategists as nodes; historical relationships, win rates, and performance data as edges. On top of this, a Markov chain engine estimates race outcomes and computes optimal strategies in natural language.

The end goal: a system that tells you *why* Ferrari should pit at lap 34, not just that they should.

## Core Architecture

- **Graph engine** — hash maps + adjacency lists (predecessors, successors, edge weights) with full file persistence
- **Circuit database** — per-circuit data with photos, layout characteristics, historical race results
- **Markov chain layer** — transition probability estimation from historical data; models driver performance, tire degradation windows, championship scenarios
- **Strategy report generator** — LLM layer translates numerical output into actionable team recommendations

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

| Node type | Examples |
|---|---|
| Driver | Leclerc, Verstappen, Norris |
| Team | Ferrari, Red Bull, McLaren |
| Circuit | Monaco, Interlagos, Abu Dhabi |
| Strategist | TBD |

Edges encode: historical win rates, driver-circuit affinity scores, team performance deltas, tire compound preferences.

> Example: Leclerc → Monaco has a higher win-transition probability than Leclerc → Interlagos,
> reflecting both familiarity and historical performance.

## Markov Chain Model

State = `(driver, circuit, lap, tire compound, position)`

Transition probabilities estimated from historical F1 data (Ergast API). The model answers:
- At which lap does switching compounds maximize P(win)?
- Given current state, what is each driver's championship probability?
- How does a safety car at lap X shift the outcome distribution?

**Scope**: simplified state space — cognitive and FIA regulation variables are out of scope for V1.

## Data Sources

- Jolpica-F1 API, OpenF1 — full historical race data, free
- Circuit photos and layout files — stored with file persistence in PitWall DB

## Roadmap

- [ ] Core graph engine — add/remove nodes, edges, file I/O
- [ ] BFS, DFS, Dijkstra query support
- [ ] Circuit database with photo storage
- [ ] Jolpica/OpenF1 data import pipeline
- [ ] Markov chain transition probability estimation
- [ ] Championship scenario simulator
- [ ] Pit stop window optimizer
- [ ] LLM strategy report generator
- [ ] Markov chain extension for Semester 1 probability course

## Timeline

July 9th, 2026 onwards

## Future

PitWall's graph + probabilistic query pattern extends naturally to other domains —
including artifact relationship modeling for interdisciplinary research platforms.
