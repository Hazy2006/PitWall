# Markov Chain Transition Model (Phase 5a) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a standalone, pooled grid-position → finish-position Markov transition model (trainer + query engine), trained from `data/results.json`, with unit tests and a real-data smoke test.

**Architecture:** Two new standalone classes with no dependency on `Graph`/`Storage`/`GraphRepository`: `MarkovTrainer` (reads `results.json` via nlohmann/json, accumulates raw integer transition counts `grid -> finish -> count`) and `MarkovEngine` (takes a const reference to trained counts, normalizes to probabilities at query time). No driver-specific logic — this is a pooled model across all drivers/races.

**Tech Stack:** C++20, MSVC (v145 toolset), nlohmann/json (vcpkg), existing `test_runner.cpp` / `main.cpp` test harness (no framework — plain asserts + std::cout PASS/FAIL prints).

## Global Constraints

- MarkovTrainer and MarkovEngine are standalone — they must NOT `#include "graph.h"`, `"storage.h"`, or `"graph_repository.h"`.
- MarkovTrainer stores RAW INTEGER COUNTS in `std::map<int, std::map<int, int>>` (outer key = grid, inner key = finish). No normalization at train time.
- MarkovEngine normalizes only at query time, returning `std::map<int, double>`.
- Every row with `grid == 0` must be filtered out during training (there is exactly one such row in `data/results.json`, a pit-lane start).
- MarkovEngine has NO dependency on nlohmann/json — it only reads the count matrix passed into its constructor.
- Float comparisons in tests use an epsilon (e.g. `1e-9` or `1e-6`), never `==`.
- All new files live in the repo root (sibling to `graph.h`, `graph.cpp`, etc.) and are referenced from `PitWallEngine.vcxproj`/`.filters` via `..\` paths, matching the existing pattern.
- All existing tests in `test_runner.cpp` must continue to pass unchanged.
- Build via MSBuild (`C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe`) targeting the `PitWallEngine` project, x64 configuration, must produce zero errors.

## Existing Codebase Reference

- Node/graph classes live in repo root: `graph.h`/`.cpp`, `node.h`, `edge.h`, `storage.h`/`.cpp`, `graph_repository.h`/`.cpp`, `data_importer.h`/`.cpp`.
- `data_importer.cpp` shows the established nlohmann/json usage pattern:
  ```cpp
  #include <nlohmann/json.hpp>
  using json = nlohmann::json;
  namespace {
      json load_json_array(const std::string& path) {
          std::ifstream file(path);
          if (!file.is_open()) {
              throw std::runtime_error("Cannot open file: " + path);
          }
          json data;
          file >> data;
          return data;
      }
  }
  ```
  Reuse this exact loading pattern (as a local static helper in `markov_trainer.cpp`, not shared — keep it standalone).
- `test_runner.cpp` test functions follow this shape: `std::cout << "--- Running X Test ---\n";` then asserts/manual `bool ok` accumulation, then a `[PASS]`/`[FAIL]` print. `test_data_importer()` (test_runner.cpp:272-349) shows the pattern for writing a synthetic JSON fixture to `std::filesystem::temp_directory_path()`, running the importer, asserting, then `fs::remove_all(temp_dir)`. Follow this same fixture pattern for `test_markov_trainer()`.
- `test_real_import()` (test_runner.cpp:351-376) shows the pattern for a real-data smoke test that prints rather than asserts.
- `main.cpp` declares each test function as an extern `void` and calls them in sequence inside `main()`, printing `"=================================\n"` separators between calls.
- `data/results.json` real shape (confirmed via inspection): array of 479 objects, fields `driver_name` (string), `circuit_name` (string), `position` (int), `grid` (int), `team_name` (string). Exactly 1 row has `grid == 0`.
- `PitWallEngine.vcxproj` and `.vcxproj.filters` list every source/header file explicitly with `..\` relative paths and two `<ItemGroup>`s (ClInclude / ClCompile) in the `.vcxproj`, plus `<Filter>Header Files</Filter>` / `<Filter>Source Files</Filter>` tags in `.filters`.
- vcpkg already has `nlohmann-json` and `sqlite3` as manifest dependencies (`PitWallEngine/vcpkg.json`) — no new dependency needed.
- Build tool: `MSBuild.exe` is on PATH at `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe`. Solution file: `PitWall.slnx`. Build command: `MSBuild.exe PitWall.slnx /p:Configuration=Debug /p:Platform=x64`.

## File Structure

- Create `markov_trainer.h` / `markov_trainer.cpp` (repo root): `MarkovTrainer` class — reads `results.json`, accumulates raw counts.
- Create `markov_engine.h` / `markov_engine.cpp` (repo root): `MarkovEngine` class — normalizes counts into probability distributions at query time.
- Modify `test_runner.cpp`: add `test_markov_trainer()`, `test_markov_engine()`, `test_markov_real()`.
- Modify `main.cpp`: declare and call the three new test functions, `test_markov_real()` wired in *after* `test_real_import()`.
- Modify `PitWallEngine/PitWallEngine.vcxproj` and `PitWallEngine/PitWallEngine.vcxproj.filters`: register the four new files.

---

### Task 1: MarkovTrainer

**Files:**
- Create: `markov_trainer.h`
- Create: `markov_trainer.cpp`
- Test: `test_runner.cpp` (add `test_markov_trainer()`, wired into `main.cpp`)

**Interfaces:**
- Consumes: nlohmann/json only (`#include <nlohmann/json.hpp>`).
- Produces:
  - `class MarkovTrainer` with:
    - `void train(const std::string& results_json_path)`
    - `const std::map<int, std::map<int,int>>& get_counts() const`
    - `int total_observations() const`
  - Later tasks (MarkovEngine, `test_markov_engine`, `test_markov_real`) consume `MarkovTrainer::get_counts()` (returns `const std::map<int, std::map<int,int>>&`) and `MarkovTrainer::total_observations()` (returns `int`).

- [ ] **Step 1: Write `markov_trainer.h`**

```cpp
#pragma once
#include <map>
#include <string>

class MarkovTrainer {
private:
    std::map<int, std::map<int, int>> counts;

public:
    void train(const std::string& results_json_path);
    const std::map<int, std::map<int, int>>& get_counts() const;
    int total_observations() const;
};
```

- [ ] **Step 2: Write `markov_trainer.cpp`**

```cpp
#include "markov_trainer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {
    json load_json_array(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        json data;
        file >> data;
        return data;
    }
}

void MarkovTrainer::train(const std::string& results_json_path) {
    json data = load_json_array(results_json_path);

    for (const auto& entry : data) {
        int grid = entry.at("grid").get<int>();
        if (grid == 0) {
            continue;
        }
        int finish = entry.at("position").get<int>();
        counts[grid][finish]++;
    }
}

const std::map<int, std::map<int, int>>& MarkovTrainer::get_counts() const {
    return counts;
}

int MarkovTrainer::total_observations() const {
    int total = 0;
    for (const auto& [grid, finishes] : counts) {
        for (const auto& [finish, count] : finishes) {
            total += count;
        }
    }
    return total;
}
```

- [ ] **Step 3: Write the failing test in `test_runner.cpp`**

Add near the top of `test_runner.cpp`, alongside the other `#include`s:

```cpp
#include "markov_trainer.h"
```

Add this function (place after `test_real_import()` in the file):

```cpp
void test_markov_trainer() {
    std::cout << "--- Running MarkovTrainer Test ---\n";

    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "pitwall_markov_trainer_test";
    fs::create_directories(temp_dir);
    fs::path results_path = temp_dir / "results.json";

    {
        std::ofstream f(results_path);
        f << R"([
            {"driver_name": "Driver One", "circuit_name": "Circuit One", "position": 1, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Two", "circuit_name": "Circuit One", "position": 2, "grid": 1, "team_name": "Team One"},
            {"driver_name": "Driver Three", "circuit_name": "Circuit One", "position": 1, "grid": 2, "team_name": "Team One"},
            {"driver_name": "Driver Four", "circuit_name": "Circuit One", "position": 5, "grid": 0, "team_name": "Team One"},
            {"driver_name": "Driver Five", "circuit_name": "Circuit One", "position": 3, "grid": 2, "team_name": "Team One"}
        ])";
    }

    MarkovTrainer trainer;
    trainer.train(results_path.string());

    fs::remove_all(temp_dir);

    const auto& counts = trainer.get_counts();

    bool ok = true;
    // grid 0 row must be filtered: only grid 1 and grid 2 keys should exist
    ok &= (counts.size() == 2);
    ok &= (counts.count(0) == 0);

    // grid 1 -> finish 1: count 1, finish 2: count 1
    ok &= (counts.at(1).at(1) == 1);
    ok &= (counts.at(1).at(2) == 1);

    // grid 2 -> finish 1: count 1, finish 3: count 1
    ok &= (counts.at(2).at(1) == 1);
    ok &= (counts.at(2).at(3) == 1);

    ok &= (trainer.total_observations() == 4);

    if (ok) {
        std::cout << "[PASS] MarkovTrainer filtered grid==0 and counted transitions correctly.\n";
    }
    else {
        std::cout << "[FAIL] MarkovTrainer counts did not match expectations.\n";
    }
}
```

Declare it in `main.cpp` alongside the other `void test_...();` declarations:

```cpp
void test_markov_trainer();
```

Call it in `main()` after the `test_real_import();` call, following the existing separator pattern:

```cpp
	std::cout << "==================================\n";
	test_markov_trainer();
```

- [ ] **Step 4: Register the two new files in the vcxproj**

In `PitWallEngine/PitWallEngine.vcxproj`, add to the `ClInclude` ItemGroup:

```xml
    <ClInclude Include="..\markov_trainer.h" />
```

and to the `ClCompile` ItemGroup:

```xml
    <ClCompile Include="..\markov_trainer.cpp" />
```

In `PitWallEngine/PitWallEngine.vcxproj.filters`, add to the `ClInclude` ItemGroup:

```xml
    <ClInclude Include="..\markov_trainer.h">
      <Filter>Header Files</Filter>
    </ClInclude>
```

and to the `ClCompile` ItemGroup:

```xml
    <ClCompile Include="..\markov_trainer.cpp">
      <Filter>Source Files</Filter>
    </ClCompile>
```

- [ ] **Step 5: Build and run to verify the test passes**

Run: `MSBuild.exe PitWall.slnx /p:Configuration=Debug /p:Platform=x64` from the repo root, then run the built executable.
Expected: build succeeds with zero errors; console output includes `[PASS] MarkovTrainer filtered grid==0 and counted transitions correctly.`

- [ ] **Step 6: Commit**

```bash
git add markov_trainer.h markov_trainer.cpp test_runner.cpp main.cpp PitWallEngine/PitWallEngine.vcxproj PitWallEngine/PitWallEngine.vcxproj.filters
git commit -m "Add MarkovTrainer for pooled grid->finish transition counts"
```

---

### Task 2: MarkovEngine

**Files:**
- Create: `markov_engine.h`
- Create: `markov_engine.cpp`
- Test: `test_runner.cpp` (add `test_markov_engine()`, wired into `main.cpp`)

**Interfaces:**
- Consumes: `const std::map<int, std::map<int,int>>&` (the type produced by `MarkovTrainer::get_counts()` in Task 1 — no direct dependency on `MarkovTrainer` itself, just the same map type).
- Produces:
  - `class MarkovEngine` with:
    - Constructor: `explicit MarkovEngine(const std::map<int, std::map<int,int>>& counts)`
    - `std::map<int, double> predict_finish_distribution(int grid_position) const`
    - `int most_likely_finish(int grid_position) const`
  - Task 3 (`test_markov_real`) consumes `MarkovEngine` constructed from `MarkovTrainer::get_counts()`.

- [ ] **Step 1: Write `markov_engine.h`**

```cpp
#pragma once
#include <map>

class MarkovEngine {
private:
    const std::map<int, std::map<int, int>>& counts;

public:
    explicit MarkovEngine(const std::map<int, std::map<int, int>>& transition_counts);

    std::map<int, double> predict_finish_distribution(int grid_position) const;
    int most_likely_finish(int grid_position) const;
};
```

- [ ] **Step 2: Write `markov_engine.cpp`**

```cpp
#include "markov_engine.h"

MarkovEngine::MarkovEngine(const std::map<int, std::map<int, int>>& transition_counts)
    : counts(transition_counts) {
}

std::map<int, double> MarkovEngine::predict_finish_distribution(int grid_position) const {
    std::map<int, double> distribution;

    auto row_it = counts.find(grid_position);
    if (row_it == counts.end()) {
        return distribution;
    }

    const std::map<int, int>& row = row_it->second;
    int row_total = 0;
    for (const auto& [finish, count] : row) {
        row_total += count;
    }

    if (row_total == 0) {
        return distribution;
    }

    for (const auto& [finish, count] : row) {
        distribution[finish] = static_cast<double>(count) / static_cast<double>(row_total);
    }

    return distribution;
}

int MarkovEngine::most_likely_finish(int grid_position) const {
    std::map<int, double> distribution = predict_finish_distribution(grid_position);
    if (distribution.empty()) {
        return -1;
    }

    int best_finish = -1;
    double best_prob = -1.0;
    for (const auto& [finish, prob] : distribution) {
        if (prob > best_prob) {
            best_prob = prob;
            best_finish = finish;
        }
    }
    return best_finish;
}
```

- [ ] **Step 3: Write the failing test in `test_runner.cpp`**

Add near the top of `test_runner.cpp`:

```cpp
#include "markov_engine.h"
#include <cmath>
```

Add this function (place after `test_markov_trainer()`):

```cpp
void test_markov_engine() {
    std::cout << "--- Running MarkovEngine Test ---\n";

    std::map<int, std::map<int, int>> counts;
    // grid 1: finish 1 x3, finish 2 x1  (total 4)
    counts[1][1] = 3;
    counts[1][2] = 1;
    // grid 2: finish 1 x1, finish 3 x1  (total 2)
    counts[2][1] = 1;
    counts[2][3] = 1;

    MarkovEngine engine(counts);

    bool ok = true;
    const double epsilon = 1e-9;

    std::map<int, double> dist1 = engine.predict_finish_distribution(1);
    ok &= (dist1.size() == 2);
    ok &= (std::fabs(dist1.at(1) - 0.75) < epsilon);
    ok &= (std::fabs(dist1.at(2) - 0.25) < epsilon);

    double sum1 = 0.0;
    for (const auto& [finish, prob] : dist1) {
        sum1 += prob;
    }
    ok &= (std::fabs(sum1 - 1.0) < epsilon);

    std::map<int, double> dist2 = engine.predict_finish_distribution(2);
    double sum2 = 0.0;
    for (const auto& [finish, prob] : dist2) {
        sum2 += prob;
    }
    ok &= (std::fabs(sum2 - 1.0) < epsilon);

    ok &= (engine.most_likely_finish(1) == 1);

    // unseen grid position
    std::map<int, double> dist_unseen = engine.predict_finish_distribution(99);
    ok &= dist_unseen.empty();
    ok &= (engine.most_likely_finish(99) == -1);

    if (ok) {
        std::cout << "[PASS] MarkovEngine normalized distributions and predicted correctly.\n";
    }
    else {
        std::cout << "[FAIL] MarkovEngine output did not match expectations.\n";
    }
}
```

Declare it in `main.cpp`:

```cpp
void test_markov_engine();
```

Call it in `main()` after `test_markov_trainer();`:

```cpp
	std::cout << "==================================\n";
	test_markov_engine();
```

- [ ] **Step 4: Register the two new files in the vcxproj**

In `PitWallEngine/PitWallEngine.vcxproj`, add to the `ClInclude` ItemGroup:

```xml
    <ClInclude Include="..\markov_engine.h" />
```

and to the `ClCompile` ItemGroup:

```xml
    <ClCompile Include="..\markov_engine.cpp" />
```

In `PitWallEngine/PitWallEngine.vcxproj.filters`, add to the `ClInclude` ItemGroup:

```xml
    <ClInclude Include="..\markov_engine.h">
      <Filter>Header Files</Filter>
    </ClInclude>
```

and to the `ClCompile` ItemGroup:

```xml
    <ClCompile Include="..\markov_engine.cpp">
      <Filter>Source Files</Filter>
    </ClCompile>
```

- [ ] **Step 5: Build and run to verify the test passes**

Run: `MSBuild.exe PitWall.slnx /p:Configuration=Debug /p:Platform=x64` from the repo root, then run the built executable.
Expected: build succeeds with zero errors; console output includes `[PASS] MarkovEngine normalized distributions and predicted correctly.`

- [ ] **Step 6: Commit**

```bash
git add markov_engine.h markov_engine.cpp test_runner.cpp main.cpp PitWallEngine/PitWallEngine.vcxproj PitWallEngine/PitWallEngine.vcxproj.filters
git commit -m "Add MarkovEngine to normalize transition counts into finish-position probabilities"
```

---

### Task 3: Real-data smoke test

**Files:**
- Modify: `test_runner.cpp` (add `test_markov_real()`)
- Modify: `main.cpp` (declare + call `test_markov_real()` after `test_real_import()`)

**Interfaces:**
- Consumes: `MarkovTrainer` (Task 1) and `MarkovEngine` (Task 2) exactly as defined above; `data/results.json` (real file, already present in the repo, confirmed to have 479 rows and exactly 1 `grid == 0` row).
- Produces: print-only output, no new symbols consumed by later tasks (this is the last task in the plan).

- [ ] **Step 1: Write `test_markov_real()` in `test_runner.cpp`**

Place after `test_markov_engine()`:

```cpp
void test_markov_real() {
    std::cout << "--- Running Real Data MarkovTrainer Smoke Test ---\n";

    MarkovTrainer trainer;
    trainer.train("data/results.json");

    std::cout << "Total observations: " << trainer.total_observations() << "\n";

    MarkovEngine engine(trainer.get_counts());

    for (int grid : {1, 5, 10, 15}) {
        int best = engine.most_likely_finish(grid);
        std::cout << "Grid " << grid << " -> most likely finish: " << best << "\n";

        std::map<int, double> distribution = engine.predict_finish_distribution(grid);

        std::vector<std::pair<int, double>> sorted_by_prob(distribution.begin(), distribution.end());
        std::sort(sorted_by_prob.begin(), sorted_by_prob.end(),
            [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                return a.second > b.second;
            });

        std::cout << "  Top " << (sorted_by_prob.size() < 3 ? sorted_by_prob.size() : 3) << " finishes:\n";
        for (size_t i = 0; i < sorted_by_prob.size() && i < 3; ++i) {
            std::cout << "    finish " << sorted_by_prob[i].first
                       << " : " << sorted_by_prob[i].second << "\n";
        }
    }
}
```

This requires `<algorithm>` — add `#include <algorithm>` to the top of `test_runner.cpp` if not already present (check first; `<vector>` is already included).

- [ ] **Step 2: Wire into `main.cpp`**

Declare:

```cpp
void test_markov_real();
```

Call in `main()`, directly after the existing `test_real_import();` call (per the spec's requirement that this comes after the existing real-import test):

```cpp
	test_real_import();
	std::cout << "==================================\n";
	test_markov_real();
```

(Note: `test_real_import()` in the current `main.cpp` has no trailing separator before `return 0;` — add one before calling `test_markov_real()`.)

- [ ] **Step 3: Build and run**

Run: `MSBuild.exe PitWall.slnx /p:Configuration=Debug /p:Platform=x64` from the repo root, then run the built executable from the repo root (so the relative path `data/results.json` resolves).
Expected: build succeeds with zero errors; console prints `Total observations: 478` (479 rows minus 1 filtered `grid==0` row) followed by the most-likely-finish and top-3 blocks for grid positions 1, 5, 10, 15.

- [ ] **Step 4: Run the full suite and confirm all prior tests still pass**

Run the built executable and visually confirm every test block prints `[PASS]` (or the appropriate pass indicator) with no `[FAIL]` lines anywhere in the output, including all pre-existing tests (`run_domain_tests`, `run_edge_tests`, `test_bfs`, `test_dfs`, `test_remove_node`, `test_dijkstra`, `test_save_and_load`, `test_save_and_load_with_id_gap`, `test_data_importer`) plus the three new Markov tests.

- [ ] **Step 5: Commit**

```bash
git add test_runner.cpp main.cpp
git commit -m "Add real-data smoke test for pooled Markov transition model"
```

---

## Self-Review Notes

- Spec coverage: Part A (MarkovTrainer: train, get_counts, total_observations, grid==0 filter, nlohmann/json only) → Task 1. Part B (MarkovEngine: predict_finish_distribution, most_likely_finish, no Graph/Storage/json dependency) → Task 2. Part C (test_markov_trainer, test_markov_engine, wiring into main.cpp) → Tasks 1 & 2. Part D (test_markov_real printing total_observations + top-3 for grid 1/5/10/15, wired after test_real_import) → Task 3. Strict rules (standalone, raw counts vs normalize-at-query, grid==0 filter, epsilon float comparisons, vcxproj/filters registration) are embedded in each task's steps.
- All code blocks are complete and buildable — no placeholders.
- Type consistency checked: `MarkovTrainer::get_counts()` returns `const std::map<int, std::map<int,int>>&`; `MarkovEngine`'s constructor takes exactly that type by const reference and stores it as a reference member (safe here since `MarkovTrainer trainer` outlives `MarkovEngine engine` in `test_markov_real` — same scope, trainer declared first).
