# PitWall Architecture Log

> Living cheatsheet. Update this as the codebase evolves.
> Last updated: 2026-07-23 — Post Phase 1, mid Phase 2 (DFS pending).

---

## File Registry

| File | Layer | Status | Purpose |
|---|---|---|---|
| `node.h` | Domain | **Complete** | Abstract `Node` base + concrete `DriverNode`, `TeamNode`, `CircuitNode`. Polymorphic hierarchy managed via `shared_ptr`. |
| `edge.h` | Domain | **Complete** | `Edge` aggregate struct — 4 `double` fields: `winRate`, `affinityScore`, `performanceDelta`, `tirePreference`. |
| `graph.h` | Domain | **Complete** | `Graph` class declaration. Node registry, dual adjacency maps, edge map, traversal methods. |
| `graph.cpp` | Domain | **Partial** | `Graph` implementation. `add_node`, `add_edge`, `bfs` done. `dfs` declared but **not defined** — linker error. `remove_node` not yet implemented. |
| `main.cpp` | Entry | **Temporary** | Currently a test harness calling `run_domain_tests()`, `run_edge_tests()`, `test_bfs()`, `test_dfs()`. Will become the application entry point once a real test target is split out. |
| `test_runner.cpp` | Testing | **Active** | All test functions live here. Not a standalone executable — compiled together with `main.cpp`. |
| `graph_repository.h` | Repository | **Stub** | `#pragma once` only. No declarations. |
| `graph_repository.cpp` | Repository | **Empty** | No implementation. Exists in `.vcxproj` but contributes nothing. |
| `PitWallEngine.vcxproj` | Build | **Active** | MSVC project file. All source files wired in, including empty stubs. |
| `PitWall.slnx` | Build | **Active** | Solution file. Single project: `PitWallEngine`. |

---

## Architecture Overview

```
Service (Phase 5)
    │
    ├── Repository (Phase 3) ──→ Storage (SQLite, Phase 3)
    │
    └── MarkovEngine (Phase 5)
            │
            └── Graph (Domain, Phase 1-2) ──→ Node hierarchy + Edge struct
```

**Call chain**: `Service → Repository → Storage → SQLite`
**Invariant**: Graph never imports SQLite headers. Repository is the boundary.

---

## Class & Struct Breakdown

### `Node` (abstract base) — `node.h`

```
Node
├── name: std::string (protected)
├── Node(const std::string&)          — parameterized constructor
├── virtual ~Node() = default         — polymorphic destructor
├── get_name() const → string         — accessor
└── get_type_string() const → string  — pure virtual, identity for serialization
```

**Design rule**: Node subclasses hold only Markov-relevant mathematical coefficients. Display data (nationality, photos, logos) lives in SQLite and gets joined at render time.

### `DriverNode` : `Node` — `node.h`

| Field | Type | Purpose |
|---|---|---|
| `tire_management_modifier` | `double` | Markov coefficient — how well this driver preserves tires |
| `base_pace_delta` | `double` | Markov coefficient — raw pace offset from baseline |

Constructor: `DriverNode(const std::string& name, double tire_mod, double pace_delta)`

### `TeamNode` : `Node` — `node.h`

| Field | Type | Purpose |
|---|---|---|
| `pit_stop_variance` | `double` | Markov coefficient — pit stop time standard deviation |

Constructor: `TeamNode(const std::string& name, double pit_variance)`

### `CircuitNode` : `Node` — `node.h`

| Field | Type | Purpose |
|---|---|---|
| `base_degradation_rate` | `double` | Markov coefficient — tire wear rate per lap |

Constructor: `CircuitNode(const std::string& name, double deg_rate)`

### `Edge` (aggregate struct) — `edge.h`

| Field | Type | Purpose |
|---|---|---|
| `winRate` | `double` | Historical win probability for this node pair |
| `affinityScore` | `double` | Driver-circuit or driver-team compatibility |
| `performanceDelta` | `double` | Performance gap relative to baseline |
| `tirePreference` | `double` | Compound preference weight |

**No constructor, no destructor, no methods.** Pure aggregate — initialized via brace-init only.

### `Graph` — `graph.h` / `graph.cpp`

#### Data Members

| Member | Type | Purpose |
|---|---|---|
| `node_registry` | `unordered_map<int, shared_ptr<Node>>` | Polymorphic node storage. Owns all nodes via shared_ptr. |
| `successors` | `unordered_map<int, vector<int>>` | Outbound adjacency lists. O(1) to get all outgoing neighbors. |
| `predecessors` | `unordered_map<int, vector<int>>` | Inbound adjacency lists. O(1) to get all incoming neighbors. |
| `edges` | `map<pair<int,int>, Edge>` | Edge data keyed by (source, target). **Bottleneck** — O(log E) per lookup. Migrate to `unordered_map` with `PairHash` before Phase 5. |
| `next_id` | `int` | Auto-incrementing node ID generator. Maps cleanly to SQLite `INTEGER PRIMARY KEY`. |

#### Methods

| Method | Status | Complexity | Purpose |
|---|---|---|---|
| `Graph()` | ✅ Done | O(1) | Default constructor. Initializes `next_id = 0`. |
| `count_vertices() const` | ✅ Done | O(1) | Returns `node_registry.size()`. |
| `count_edges() const` | ✅ Done | O(1) | Returns `edges.size()`. |
| `add_node(shared_ptr<Node>)` | ✅ Done | O(1) amortized | Registers node, initializes empty adjacency lists, returns assigned ID. Throws on null pointer. |
| `get_node(int) const` | ✅ Done | O(1) amortized | Returns shared_ptr to node. Throws on invalid ID. |
| `add_edge(int, int, const Edge&)` | ✅ Done | O(log E) | Adds directed edge. Silent no-op if edge exists. Throws on invalid node IDs. |
| `is_edge(int, int) const` | ✅ Done | O(log E) | Checks edge existence. Throws on invalid node IDs. |
| `get_edge(int, int) const` | ✅ Done | O(log E) | Returns Edge copy. Throws if edge doesn't exist. |
| `bfs(int)` | ✅ Done | O(V+E) | Breadth-first traversal from start node. Returns visit-order vector. |
| `dfs(int)` | ❌ **Declared, not defined** | O(V+E) | Depth-first traversal. **Will cause LNK2019 unresolved external**. |
| `remove_node(int)` | ❌ Not yet declared | — | Next task. Must remove from registry, both adjacency maps, and all incident edges. |

---

## Traversal: BFS vs DFS — When and Why

| | BFS | DFS |
|---|---|---|
| **Data structure** | Queue (FIFO) | Stack (LIFO) or recursion |
| **Exploration pattern** | Level by level — all distance-1, then distance-2, etc. | Deep branch first, backtrack when stuck |
| **PitWall use case** | "Which nodes are reachable within N hops?" — e.g., find all circuits a driver has raced at through team connections | "Does a dependency cycle exist?" — e.g., circular team-driver-circuit relationships that would break Markov chain acyclicity assumptions |
| **Dijkstra prep** | BFS is the unweighted special case of Dijkstra. Implementing BFS first validates the adjacency structure before adding priority queue logic. | DFS provides topological ordering — needed if you ever model the Markov state space as a DAG for efficient forward computation. |

---

## Planned Components (Not Yet Implemented)

### `GraphRepository` — Phase 3

**Purpose**: Translates between `Graph` domain objects and SQLite rows. The only layer that imports both `graph.h` and `sqlite3.h`.

**Responsibilities**:
- `save_graph(const Graph&)` — serialize all nodes and edges to SQLite tables
- `load_graph() → Graph` — read SQL rows, reconstruct polymorphic nodes via factory, rebuild adjacency
- Node type dispatching: read `type` column → call correct constructor (`DriverNode`, `TeamNode`, etc.)

**Factory pattern for reconstruction**:
```cpp
std::shared_ptr<Node> make_node(const std::string& type,
                                 const std::string& name,
                                 double param1, double param2) {
    if (type == "Driver") return std::make_shared<DriverNode>(name, param1, param2);
    if (type == "Team")   return std::make_shared<TeamNode>(name, param1);
    if (type == "Circuit") return std::make_shared<CircuitNode>(name, param1);
    throw std::invalid_argument("Unknown node type: " + type);
}
```

**SQLite schema (planned)**:
```sql
CREATE TABLE nodes (
    id          INTEGER PRIMARY KEY,
    type        TEXT NOT NULL,       -- 'Driver', 'Team', 'Circuit'
    name        TEXT NOT NULL,
    param1      REAL,                -- tire_management_modifier / pit_stop_variance / base_degradation_rate
    param2      REAL                 -- base_pace_delta (Driver only, NULL for others)
);

CREATE TABLE edges (
    source_id        INTEGER NOT NULL REFERENCES nodes(id),
    target_id        INTEGER NOT NULL REFERENCES nodes(id),
    win_rate         REAL NOT NULL DEFAULT 0.0,
    affinity_score   REAL NOT NULL DEFAULT 0.0,
    performance_delta REAL NOT NULL DEFAULT 0.0,
    tire_preference  REAL NOT NULL DEFAULT 0.0,
    PRIMARY KEY (source_id, target_id)
);
```

### `Storage` — Phase 3

**Purpose**: Raw SQLite execution. Knows nothing about `Node` or `Edge` — only executes SQL strings and returns raw results.

```cpp
class Storage {
    sqlite3* db;
public:
    Storage(const std::string& db_path);
    ~Storage();
    void execute(const std::string& sql);
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
};
```

### `MarkovEngine` — Phase 5

**Purpose**: Operates on a separate Markov state graph (not the domain graph). Takes trained transition probabilities and answers probabilistic queries.

**State**: `(driver_id, circuit_id, lap, tire_compound, position)` — each unique combination is a `MarkovStateNode`.

**Transition edge**: probability of moving from one state to another.

---

## MSVC Strict Compliance Rules

### Rule 1: Always fully initialize `Edge` structs

```cpp
// ✅ CORRECT — all 4 fields explicit
Edge{ 0.15, 0.90, -0.1, 1.0 }

// ❌ WRONG — partial init, zero-fills silently, unclear intent
Edge{ static_cast<double>(b), 1.0 }

// ❌ WRONG — narrowing from int
Edge{ b, 1.0, 0.0, 0.0 }  // if b is int, MSVC C2397
```

### Rule 2: Never brace-initialize derived classes

```cpp
// ❌ WRONG — DriverNode has a base class, not an aggregate
DriverNode{"N1", 10.0, 1.0}

// ✅ CORRECT — use constructor syntax or make_shared
DriverNode("N1", 10.0, 1.0)
std::make_shared<DriverNode>("N1", 10.0, 1.0)
```

### Rule 3: Always use `std::make_shared` for node creation in tests

```cpp
// ✅ Standard pattern for all tests
auto node = std::make_shared<DriverNode>("Leclerc", 0.87, -0.2);
int id = g.add_node(node);

// ✅ Also fine — inline
int id = g.add_node(std::make_shared<DriverNode>("Leclerc", 0.87, -0.2));
```

### Rule 4: Use explicit `double` literals, never implicit int-to-double

```cpp
// ❌ Triggers C2397 in strict mode
Edge{ 0, 1, 0, 0 }      // int literals in double fields

// ✅ All values are double literals
Edge{ 0.0, 1.0, 0.0, 0.0 }
```

### Rule 5: Mark non-mutating methods `const`

`bfs()` and `dfs()` do not modify the graph. They must be:
```cpp
std::vector<int> bfs(int start_node_id) const;
std::vector<int> dfs(int start_node_id) const;
```

---

## Known Issues (as of 2026-07-23)

| # | Severity | Description | Fix |
|---|---|---|---|
| 1 | **Blocker** | `dfs()` declared in `graph.h` but not defined in `graph.cpp`. Linker error LNK2019. | Implement `dfs()` in `graph.cpp`. |
| 2 | **Warning** | `test_dfs()` uses 2-field Edge init `Edge{ static_cast<double>(b), 1.0 }` — remaining fields silently zero-initialized. | Use full 4-field init: `Edge{ val, 1.0, 0.0, 0.0 }`. |
| 3 | **Design** | `bfs()` and `dfs()` not marked `const`. | Add `const` qualifier. |
| 4 | **Design** | `edges` uses `std::map` — O(log E) per lookup. | Migrate to `unordered_map<pair<int,int>, Edge, PairHash>` before Phase 5. |
| 5 | **Design** | No `remove_node()` or `remove_edge()`. | Next implementation task. |
| 6 | **Design** | No graph-wide iteration (`get_all_node_ids()`, `get_all_edge_keys()`). | Required for Repository serialization in Phase 3. |
| 7 | **Hygiene** | `graph_repository.h/cpp` are empty stubs wired into `.vcxproj`. | Either implement or remove from build until Phase 3. |
| 8 | **Hygiene** | `main.cpp` and `test_runner.cpp` are fused into one compile target. | Separate into app target + test target when build system matures. |
| 9 | **Testing** | `assert(retrieved.affinityScore == 0.90)` — floating-point equality. | Works for exact literals but will break with computed values. Use epsilon comparison for Phase 5 Markov outputs. |

---

## Immediate TODO Stack

1. **Implement `dfs()`** in `graph.cpp` — unblock the test suite
2. **Fix `test_dfs()` Edge init** — use full 4-field brace-init with double literals
3. **Add `const` to `bfs()` and `dfs()`** signatures
4. **Implement `remove_node(int id)`** — remove from registry, both adjacency maps, and all incident edges
5. **Add `get_all_node_ids()` and `get_all_edge_keys()`** — needed for Phase 3
6. **Implement Dijkstra** — completes Phase 2

---

## DFS Implementation Reference

```cpp
std::vector<int> Graph::dfs(int start_node_id) const {
    if (node_registry.count(start_node_id) == 0) {
        throw std::invalid_argument("Start node does not exist!");
    }

    std::vector<int> result;
    std::stack<int> stk;
    std::unordered_set<int> seen;

    stk.push(start_node_id);

    while (!stk.empty()) {
        int cur = stk.top();
        stk.pop();

        if (!seen.insert(cur).second) continue;
        result.push_back(cur);

        auto it = successors.find(cur);
        if (it != successors.end()) {
            // Push in reverse order so leftmost neighbor is visited first
            const auto& nbrs = it->second;
            for (auto rit = nbrs.rbegin(); rit != nbrs.rend(); ++rit) {
                if (seen.count(*rit) == 0) {
                    stk.push(*rit);
                }
            }
        }
    }

    return result;
}
```

**Note**: Requires `#include <stack>` in `graph.cpp`.

The reverse-iteration push order ensures DFS visits neighbors in the same order they were inserted (matching `test_dfs()` expectations: a→b→d→c, where b was added before c in successors[a]).
