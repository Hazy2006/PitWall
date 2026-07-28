# PitWall Architecture Log

> Generated directly from the current source tree during the audit cleanup pass
> that dropped the unused `Edge` columns, added the 10-race minimum for driver
> indices, and hardened `test_real_import` against a locked database file.

---

## File Registry

| File | Layer | Purpose |
|---|---|---|
| `node.h` | Domain | Abstract `Node` base (`name`, `get_type_string()`) plus concrete `DriverNode` (`tire_management_modifier`, `base_pace_delta`), `TeamNode` (`pit_stop_variance`), `CircuitNode` (`base_degradation_rate`). Polymorphic hierarchy, owned via `shared_ptr<Node>`. |
| `edge.h` | Domain | `Edge` aggregate struct. One field: `winRate`. |
| `graph.h` / `graph.cpp` | Domain | `Graph` class — directed graph over integer node IDs. Owns a node registry, dual adjacency lists (successors/predecessors), and an edge map keyed by `(source, target)`. Provides traversal and shortest-path algorithms. No knowledge of SQLite, JSON, or the Markov layer. |
| `data_importer.h` / `data_importer.cpp` | Import | `DataImporter` — reads driver/team/circuit/results JSON into a `Graph`, adding nodes and driver→circuit win-rate edges. Holds the only `name → node id` mapping in the system. |
| `graph_repository.h` / `graph_repository.cpp` | Persistence | `GraphRepository` — serializes a `Graph` to/from SQLite via `Storage`. Owns the `CREATE TABLE` statements and the node/edge row mapping. |
| `storage.h` / `storage.cpp` | Persistence | `Storage` — thin RAII wrapper around a raw `sqlite3*` handle. `execute()` for statements without results, `query()` returning rows as `vector<map<string,string>>`. No SQL knowledge beyond executing what it's given. |
| `markov_trainer.h` / `markov_trainer.cpp` | Prediction | `MarkovTrainer` — reads race results JSON and builds pooled `grid → finish` transition counts (`map<int, map<int,int>>`), plus per-driver performance indices. **Graph-free**: takes a file path in, returns plain data structures out. |
| `markov_engine.h` / `markov_engine.cpp` | Prediction | `MarkovEngine` — turns pooled transition counts into finish-position probability distributions, and applies a per-driver scalar shift on top of the pooled distribution. **Graph-free**: holds only a `const&` to a transition-count map. |
| `test_runner.cpp` | Testing | All test functions, including the `apply_driver_indices` integration helper that is the *only* piece of code that bridges the Markov layer's output back into the `Graph` (writes into `DriverNode::base_pace_delta`). |
| `main.cpp` | Entry | Runs every test function in `test_runner.cpp` in sequence; not yet a real application entry point. |
| `PitWallEngine.vcxproj(.filters)` | Build | MSVC project, vcpkg manifest mode enabled (`VcpkgEnabled=true`). |
| `PitWallEngine/vcpkg.json` | Build | vcpkg manifest — dependencies `sqlite3`, `nlohmann-json`. |
| `PitWall.slnx` | Build | Solution file, single project (`PitWallEngine`). |
| `scripts/fetch_f1_data.py`, `scripts/requirements.txt` | Data | Python script that fetches F1 data and writes `data/*.json`. Not compiled; `data/` itself is gitignored (generated). |

---

## Domain layer: `Graph`

Adjacency is stored twice (`successors`, `predecessors`) plus a flat `edges` map keyed by `pair<int,int>`, so edge lookup, both traversal directions, and edge enumeration are all O(1)/O(log n) rather than requiring a scan.

| Method | Behavior |
|---|---|
| `Graph()` | Initializes empty graph, `next_id = 0`. |
| `count_vertices() const` | Size of the node registry. |
| `add_node(shared_ptr<Node>)` | Assigns the next auto-incrementing ID, registers empty adjacency lists, returns the ID. Throws on null. |
| `add_node_with_id(int id, shared_ptr<Node>)` | Same as above but with a caller-supplied ID (used by `GraphRepository::load_graph` to preserve IDs across a save/load round trip, including gaps left by `remove_node`). Throws if the ID is taken; advances `next_id` past it. |
| `get_node(int id) const` | Returns the node or throws `invalid_argument` if unknown. |
| `remove_node(int id)` | Removes the node, all edges touching it (both directions), and cleans up every other node's adjacency list. Throws if unknown. |
| `count_edges() const` | Size of the edge map. |
| `add_edge(int x, int y, const Edge&)` | Adds a directed edge x→y. No-op if the edge already exists (checked via `is_edge`). Throws if either endpoint is missing. |
| `is_edge(int x, int y) const` | Throws if either endpoint is missing; otherwise returns whether the edge exists. |
| `get_edge(int x, int y) const` | Throws if the edge doesn't exist. |
| `bfs(int start) const` | Standard breadth-first traversal, returns visit order. Throws if `start` is missing. |
| `dfs(int start) const` | Iterative depth-first traversal (explicit stack, neighbors pushed in reverse to preserve left-to-right order). Throws if `start` is missing. |
| `dijkstra(int start, int end) const` | Shortest path by edge weight, using `Edge::winRate` as the weight (the only numeric field left on `Edge`). Returns the path as a vector of IDs, or an empty vector if unreachable. Throws if either endpoint is missing. |
| `get_all_node_ids() const` | All registered node IDs, for repository/export use. |
| `get_all_edge_keys() const` | All `(source, target)` pairs, for repository/export use. |

## Import layer: `DataImporter`

- `import_drivers/import_teams/import_circuits(path)`: one JSON array each, one node per entry, recording `name → id` as it goes.
- `import_results(path)`: aggregates `(driver, circuit)` win/total counts across every race result row, then adds one `driver → circuit` edge per pair that appeared, with `Edge{ win_rate }` (`wins / total`, unweighted by anything else). Pairs whose driver or circuit wasn't separately imported are silently skipped.
- `get_node_id(name)`: reverse lookup, `-1` if unknown.

## Prediction layer: `MarkovTrainer` / `MarkovEngine`

Both classes are deliberately **graph-free** — they never see a `Graph`, `Node`, or `Edge`. `test_runner.cpp::apply_driver_indices` is the sole integration point that writes their output into the domain graph.

- `MarkovTrainer::train(path)`: reads results JSON, increments `counts[grid][finish]` for every row where `grid != 0`.
- `MarkovTrainer::get_counts() / total_observations()`: read access to the pooled table.
- `MarkovTrainer::compute_driver_indices(path)`: for each driver, averages `(grid - finish)` over all `grid != 0` rows. **Drivers with fewer than 10 valid rows are omitted from the returned map entirely** — not present, not `0.0`.
- `MarkovEngine::predict_finish_distribution(grid)`: normalizes one pooled row into a probability distribution; empty map if the grid position was never observed.
- `MarkovEngine::most_likely_finish(grid)`: argmax of the above, `-1` if unseen.
- `MarkovEngine::predict_finish_distribution_for_driver(grid, driver_index)`: shifts the pooled distribution by the full fractional `driver_index` (not rounded). Each bucket's mass lands on a fractional target position and is split between the two straddling integer positions, proportional to distance; targets are clamped to `[1, max finish observed for that grid]` before mass is accumulated, so total probability is conserved.

## Persistence layer: final schema

```sql
CREATE TABLE IF NOT EXISTS nodes (
    id INTEGER PRIMARY KEY,
    type TEXT NOT NULL,
    name TEXT NOT NULL,
    param1 REAL,
    param2 REAL
);

CREATE TABLE IF NOT EXISTS edges (
    source_id INTEGER NOT NULL REFERENCES nodes(id),
    target_id INTEGER NOT NULL REFERENCES nodes(id),
    win_rate REAL NOT NULL DEFAULT 0.0,
    PRIMARY KEY (source_id, target_id)
);
```

`edges` was trimmed from 4 numeric columns to 1 (`win_rate` only) — `affinity_score`, `performance_delta`, and `tire_preference` were dead columns with no producer.

`nodes.param1` / `nodes.param2` are generic slots whose meaning depends on `type`, mapped in `GraphRepository::save_graph` / the local `make_node()` helper in `graph_repository.cpp`:

| Node type | `param1` | `param2` |
|---|---|---|
| `Driver` | `tire_management_modifier` — **reserved for the future pit-strategy layer** | `base_pace_delta` — holds the driver performance index once `apply_driver_indices` writes it; `0.0` for drivers below the 10-race threshold or before that step runs |
| `Team` | `pit_stop_variance` | unused, `0.0` |
| `Circuit` | `base_degradation_rate` | unused, `0.0` |

No `.db` files ship in the repo (`*.db` is gitignored); a fresh `pitwall_f1.db` regenerates from `data/*.json` on the next run of `test_real_import`, no migration path needed.

---

## Known limitations

- **`win_rate` is effectively boolean.** `DataImporter::import_results` aggregates wins/total per `(driver, circuit)` pair, but the current single-season data has at most one race per driver-circuit combination, so `win_rate` in practice only ever takes the values `0.0` or `1.0` — the "rate" framing doesn't have multi-race data to average over yet.
- **Driver index is unweighted above the 10-race threshold.** `compute_driver_indices` treats a driver with exactly 10 valid rows the same as one with 200 — there's no confidence weighting or recency weighting once the minimum is cleared.
- **The handicap shift is a single scalar.** `predict_finish_distribution_for_driver` shifts the whole pooled distribution by one `driver_index`-sized amount (fractional, split proportionally between straddling positions); it can't express that a driver over- or under-performs differently depending on grid position, circuit, or car. A driver starting P1 in a dominant car can never show a "gain," since there's no room above P1.
- **Large positive shifts saturate at P1.** When a bucket's interpolated destination interval falls partly or entirely below position 1, every fractional endpoint in that interval clamps to the P1 boundary, so overperformance mass piles up there instead of spreading further "ahead" of first place. This is correct boundary behavior given there's no finish better than P1 to hold that mass, not a defect in the clamp.
- **`MarkovEngine` holds a `const&` to the trainer's counts.** `MarkovEngine(const map<int,map<int,int>>& transition_counts)` stores a reference, not a copy. This is only safe as long as the `MarkovTrainer` (or whatever counts map was passed in) outlives the `MarkovEngine` instance — nothing enforces that lifetime relationship at compile time.
