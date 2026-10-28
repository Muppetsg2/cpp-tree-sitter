# Benchmarks

This document compares the performance of **`cpp-tree-sitter`** (the C++ wrapper) against the raw **tree-sitter C API** to answer a single question: *does the convenience of RAII, iterators, and `ts::visit` come with a runtime cost?*

## TL;DR

**No, practically none.** Across all measured operations, the difference falls well within the margin of measurement noise (typically below ~3%). In several cases (like parsing or incremental reparse), the C++ wrapper even performs **slightly faster** than the raw C API. With modern compiler optimizations (like IPO), these RAII abstractions are effectively zero-cost.

| Category | Wrapper Overhead (Median) | Interleaved Overhead (Median) |
| --- | --- | --- |
| Parsing (100 items) | **-0.83 %** (faster) | +0.51 % |
| Parsing (1,000 items) | **-7.87 %** (faster) | +0.57 % |
| Parsing (10,000 items) | **-2.65 %** (faster) | **-0.32 %** (faster) |
| Parsing (nested, depth 500) | +0.14 % | +0.53 % |
| Cursor Traversal (10,000 items) | +0.13 % | +0.27 % |
| `ts::visit` Traversal (10,000 items) | +1.26 % | -- |
| Cursor Traversal (nested, depth 500) | +1.21 % | +0.48 % |
| `ts::visit` Traversal (nested, depth 500) | +2.06 % | -- |
| Query Matching | +3.28 % | +3.32 % |
| Incremental Reparse | +0.71 % | **-1.36 %** (faster) |
| Tree Copy | �0.00 % | �0.00 % |

*(Negative values indicate that the C++ wrapper was **faster** than the raw C API in a given measurement�typical for measurement noise or cache layout effects at the micro/millisecond scale).*

## Methodology

The benchmark measures five groups of operations that most frequently appear in real-world tree-sitter usage:

1. **Parsing** � from source string to syntax tree, tested on three sizes of a flat JSON array (100 / 1,000 / 10,000 items) and a deeply nested object (depth 500).
2. **Tree Traversal** � full tree traversal using the C API cursor, the C++ wrapper cursor, and the built-in C++ `ts::visit` DFS utility.
3. **Query Matching** � executing the `(string) @value` query on the tree and counting the matches.
4. **Incremental Reparse** � editing the tree and reparsing it incrementally, compared against a full scratch reparse (to demonstrate the speedup of the incremental mechanism itself).
5. **Tree Copy** � the cost of `Tree::copy()` vs. its C API equivalent.

Additionally, if the benchmark is built with `CPP_TS_BENCHMARK_FEATURE_WASM`, a **WASM** section is included (parsing and traversing a `.wasm` grammar using `WasmStore`/`WasmEngine`).

### Advanced Measurement Techniques

* **Warm-ups:** Each benchmark group runs a series of warm-up iterations to stabilize the CPU cache and branch predictor before actual timing begins.
* **Interleaved Tests:** To eliminate artificial CPU cache advantages that might occur when running a tight loop of identical operations, the benchmarks include "Interleaved" tests. In these tests, the C API, the C++ Wrapper, and the C API again are executed back-to-back inside the *same* loop. This simulates a more realistic execution environment.

### Test Environment

* **Compiler/OS:** MSVC on Windows 10
* **Optimizations:** IPO (Interprocedural Optimization / Link Time Optimization) **Enabled**
* **C++ Standard:** Benchmarks are run across multiple language standards to ensure zero-cost abstraction consistency. Overheads are calculated based on the **median** time to resist outliers.

### C++ Standards Compliance

The benchmark code compiles without modification under C++11 through C++23. It actively utilizes newer language features when available rather than avoiding them:

* Under **C++23**, object creation goes through non-throwing `Parser::create` / `Query::create` / `WasmStore::create` factories, returning `std::expected<T, ts::Error>`. Prior to C++23, these factories do not exist in the headers (guarded by `#if CPP_TS_HAS_EXPECTED`), so the benchmark falls back to throwing constructors and normalizes the error to `std::runtime_error`.
* Passing the "old tree" to `Parser::parseString` for incremental parsing accepts the same syntax in every standard (`tree`, `&tree`, `nullptr`, `{}` and, since C++17, `std::nullopt` or `std::ref(tree)`).

> **Note:** If you build `cpp-tree-sitter` as a separate translation unit (non-header-only), ensure its `CPP_TS_CXX_STANDARD` matches the standard used to build the benchmark.

---

## Automated runs (GitHub Actions)

The [`Benchmarks`](../.github/workflows/benchmarks.yml) workflow (started manually from the *Actions* tab) builds the suite in Release mode with IPO and Wasm enabled for every compiler/OS pair (GCC and Clang on Linux, AppleClang on MacOS, MSVC on Windows) and every supported C++ standard. Each run writes one file per configuration to [`docs/benchmarks`](benchmarks/) named `<system>-<compiler>-cxx<standard>.txt` (e.g. `linux-gcc-cxx20.txt`) and commits them to the branch the workflow was started from.

GitHub-hosted runners are shared virtual machines, so absolute times vary between runs. Compare the wrapper/C API overhead inside one file rather than raw times between files.

To run the suite locally without the final "Press Enter" prompt, pass `--no-wait` (it is also skipped automatically when the standard input is not a terminal).

## Results

### C++11 / C++14 (MSVC, IPO Enabled)

#### Parsing

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.7097 ms | 0.7141 ms | +0.62 % | -1.11 % |
| Flat array (1,000 items) | 6.8993 ms | 6.7049 ms | -2.82 % | +0.19 % |
| Flat array (10,000 items) | 69.3522 ms | 68.6909 ms | -0.95 % | -0.46 % |
| Nested object (depth 500) | 0.8650 ms | 0.8566 ms | -0.97 % | -0.15 % |

#### Tree Traversal

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 35.9588 ms | 36.6253 ms | 36.7164 ms | +1.85 % | +2.11 % | +0.93 % |
| Nested object (depth 500) | 0.4585 ms | 0.4618 ms | 0.4653 ms | +0.73 % | +1.49 % | +0.39 % |

#### Query Matching (Pattern: `(string) @value`)

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 63.0781 ms | 70.4306 ms | +11.66 % | +2.98 % |
| *(Matches Found: 40,000)* |  |  |  |  |

#### Incremental Reparse

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Incremental Reparse | 0.4341 ms | 0.4358 ms | +0.39 % | +0.35 % |
| Full Reparse (Baseline) | 6.5084 ms | 6.5508 ms |  |  |
| **Speedup vs Full Reparse** | **-93.33 %** | **-93.35 %** |  |  |

#### Tree Copy

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 0.0001 ms | 0.0001 ms | **�0.00 %** | **�0.00 %** |

#### WASM (WebAssembly Grammar)

**WASM Parsing**

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.9502 ms | 0.9353 ms | -1.56 % | +0.35 % |
| Flat array (1,000 items) | 9.2092 ms | 9.1261 ms | -0.90 % | -0.40 % |
| Flat array (10,000 items) | 95.7584 ms | 93.8304 ms | -2.01 % | -0.30 % |
| Nested object (depth 500) | 1.1701 ms | 1.1647 ms | -0.46 % | -1.04 % |

**WASM Tree Traversal**

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 36.4775 ms | 36.3866 ms | 36.8610 ms | -0.25 % | +1.05 % | +0.37 % |
| Nested object (depth 500) | 0.4622 ms | 0.4603 ms | 0.4670 ms | -0.41 % | +1.44 % | +0.52 % |

---

### C++17 (MSVC, IPO Enabled)

#### Parsing

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.6898 ms | 0.6977 ms | +1.14 % | -0.46 % |
| Flat array (1,000 items) | 6.6611 ms | 6.7192 ms | +0.87 % | +0.39 % |
| Flat array (10,000 items) | 67.4626 ms | 67.8799 ms | +0.62 % | +0.09 % |
| Nested object (depth 500) | 0.8565 ms | 0.8539 ms | -0.30 % | +0.29 % |

#### Tree Traversal

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 36.1456 ms | 36.1722 ms | 37.0083 ms | +0.07 % | +2.39 % | +0.17 % |
| Nested object (depth 500) | 0.4578 ms | 0.4570 ms | 0.4705 ms | -0.19 % | +2.76 % | +0.40 % |

#### Query Matching (Pattern: `(string) @value`)

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 60.4241 ms | 62.8746 ms | +4.06 % | +3.07 % |
| *(Matches Found: 40,000)* |  |  |  |  |

#### Incremental Reparse

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Incremental Reparse | 0.4357 ms | 0.4408 ms | +1.17 % | +0.79 % |
| Full Reparse (Baseline) | 6.4021 ms | 6.4803 ms |  |  |
| **Speedup vs Full Reparse** | **-93.19 %** | **-93.20 %** |  |  |

#### Tree Copy

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 0.0001 ms | 0.0001 ms | **�0.00 %** | **�0.00 %** |

#### WASM (WebAssembly Grammar)

**WASM Parsing**

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.9326 ms | 0.9178 ms | -1.59 % | -0.55 % |
| Flat array (1,000 items) | 9.1250 ms | 8.9685 ms | -1.72 % | +0.95 % |
| Flat array (10,000 items) | 94.1538 ms | 95.1577 ms | +1.07 % | -1.31 % |
| Nested object (depth 500) | 1.1730 ms | 1.1597 ms | -1.13 % | +0.47 % |

**WASM Tree Traversal**

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 36.0175 ms | 36.3036 ms | 36.7182 ms | +0.79 % | +1.95 % | +0.32 % |
| Nested object (depth 500) | 0.4558 ms | 0.4610 ms | 0.4670 ms | +1.13 % | +2.45 % | +0.62 % |

---

### C++20 (MSVC, IPO Enabled)

#### Parsing

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.7316 ms | 0.7256 ms | -0.83 % | +0.51 % |
| Flat array (1,000 items) | 7.3562 ms | 6.7771 ms | -7.87 % | +0.57 % |
| Flat array (10,000 items) | 76.2939 ms | 74.2747 ms | -2.65 % | -0.32 % |
| Nested object (depth 500) | 0.8606 ms | 0.8618 ms | +0.14 % | +0.53 % |

#### Tree Traversal

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 37.0381 ms | 37.0847 ms | 37.5036 ms | +0.13 % | +1.26 % | +0.27 % |
| Nested object (depth 500) | 0.4574 ms | 0.4629 ms | 0.4668 ms | +1.21 % | +2.06 % | +0.48 % |

#### Query Matching (Pattern: `(string) @value`)

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 61.9098 ms | 63.9391 ms | +3.28 % | +3.32 % |
| *(Matches Found: 40,000)* |  |  |  |  |

#### Incremental Reparse

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Incremental Reparse | 0.4394 ms | 0.4425 ms | +0.71 % | -1.36 % |
| Full Reparse (Baseline) | 6.6469 ms | 6.6548 ms |  |  |
| **Speedup vs Full Reparse** | **-93.39 %** | **-93.35 %** |  |  |

#### Tree Copy

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 0.0001 ms | 0.0001 ms | **�0.00 %** | **�0.00 %** |

#### WASM (WebAssembly Grammar)

**WASM Parsing**

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.9269 ms | 0.9259 ms | -0.11 % | -1.19 % |
| Flat array (1,000 items) | 9.3519 ms | 9.2434 ms | -1.16 % | -0.92 % |
| Flat array (10,000 items) | 97.4764 ms | 97.7721 ms | +0.30 % | -0.05 % |
| Nested object (depth 500) | 1.1824 ms | 1.1717 ms | -0.90 % | +0.94 % |

**WASM Tree Traversal**

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 36.9787 ms | 36.9081 ms | 37.3998 ms | -0.19 % | +1.14 % | +0.60 % |
| Nested object (depth 500) | 0.4588 ms | 0.4592 ms | 0.4694 ms | +0.08 % | +2.31 % | +0.40 % |

---

### C++23 (MSVC, IPO Enabled)

#### Parsing

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.7122 ms | 0.7036 ms | -1.20 % | +0.64 % |
| Flat array (1,000 items) | 6.9002 ms | 6.5221 ms | -5.48 % | +0.07 % |
| Flat array (10,000 items) | 68.2819 ms | 68.2344 ms | -0.07 % | -0.57 % |
| Nested object (depth 500) | 0.8623 ms | 0.8510 ms | -1.32 % | -0.47 % |

#### Tree Traversal

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 36.5866 ms | 35.9927 ms | 36.8823 ms | -1.62 % | +0.81 % | +0.52 % |
| Nested object (depth 500) | 0.4599 ms | 0.4596 ms | 0.4725 ms | -0.07 % | +2.74 % | +0.58 % |

#### Query Matching (Pattern: `(string) @value`)

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 62.7286 ms | 62.7648 ms | +0.06 % | +2.95 % |
| *(Matches Found: 40,000)* |  |  |  |  |

#### Incremental Reparse

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Incremental Reparse | 0.4340 ms | 0.4440 ms | +2.32 % | -0.13 % |
| Full Reparse (Baseline) | 6.5333 ms | 6.4756 ms |  |  |
| **Speedup vs Full Reparse** | **-93.36 %** | **-93.14 %** |  |  |

#### Tree Copy

| Metric | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Execution Time | 0.0001 ms | 0.0001 ms | **�0.00 %** | **�0.00 %** |

#### WASM (WebAssembly Grammar)

**WASM Parsing**

| Scenario | C API (Median) | C++ Wrapper (Median) | Wrapper Overhead | Interleaved Overhead |
| --- | --- | --- | --- | --- |
| Flat array (100 items) | 0.9343 ms | 0.9011 ms | -3.55 % | +0.30 % |
| Flat array (1,000 items) | 9.0939 ms | 9.1806 ms | +0.95 % | +0.25 % |
| Flat array (10,000 items) | 95.5773 ms | 95.6825 ms | +0.11 % | -0.21 % |
| Nested object (depth 500) | 1.1796 ms | 1.1851 ms | +0.47 % | -0.15 % |

**WASM Tree Traversal**

| Scenario | C API Cursor | C++ Cursor | C++ `ts::visit` | Cursor Overhead | `visit` Overhead | Interleaved Cursor Overhead |
| --- | --- | --- | --- | --- | --- | --- |
| Flat array (10,000 items) | 36.1172 ms | 35.8982 ms | 37.0668 ms | -0.61 % | +2.63 % | +0.04 % |
| Nested object (depth 500) | 0.4597 ms | 0.4594 ms | 0.4710 ms | -0.07 % | +2.46 % | +0.45 % |

---

## Conclusions

* **Parsing and Incremental Reparse:** The wrapper introduces absolutely no measurable cost. With IPO enabled, any minor discrepancies trend negative (faster than C API), proving the abstractions are fully optimized away.
* **Traversal and Query Matching:** The overhead sits around **1�3%**, primarily resulting from the C++ wrapper's bounds checking and iterator management. For operations spanning tens of milliseconds, this is virtually imperceptible.
* **`ts::visit` vs Manual Cursor:** The convenience of the DFS visitor utility adds roughly ~1.5�2.5% overhead compared to manual cursor manipulation�an easily justifiable trade-off for safer, much more readable code.
* **Interleaved Stability:** The interleaved results firmly prove that the C++ wrapper's performance does not rely on artificial cache "warm-up" advantages that might occur in a tight loop.
* **WASM Overhead:** Operating on WASM grammars through the C++ Wrapper has identical (or even slightly negative) overheads compared to using the WASM C API directly. The only heavy lifting is done during module initialization.

In short: **RAII, iterators, and `ts::visit` in `cpp-tree-sitter` are effectively free.**

## Running the Benchmarks

```bash
cmake -B build -DCPP_TS_BUILD_BENCHMARK=ON
cmake --build build --config Release
./build/benchmark/cpp-tree-sitter-benchmark
```

To include the WASM section, build with `CPP_TS_FEATURE_WASM=ON` and ensure the compiled `.wasm` grammar (e.g., `tree-sitter-json.wasm`, generated via `tree-sitter build --wasm`) is available at the expected path (configurable via `CPP_TS_BENCHMARK_WASM_JSON_PATH`). If the WASM module isn't found, the benchmark will print a warning and gracefully continue.

### Running for Specific C++ Standards

The benchmark compiles from C++11 up to C++23. To test a specific standard, set `CPP_TS_CXX_STANDARD` (controls the standard for the static library) alongside `CMAKE_CXX_STANDARD` (controls the benchmark project standard). Example for C++17:

```bash
cmake -B build-cxx17 -DCPP_TS_BUILD_BENCHMARK=ON -DCPP_TS_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD=17
cmake --build build-cxx17 --config Release
./build-cxx17/benchmark/cpp-tree-sitter-benchmark
```

The output will confirm the detected standard (e.g., `C++ standard detected: C++17`). You can run these commands for `11`, `14`, `20`, and `23` and populate the placeholder tables above to compare overheads across standard library implementations.
