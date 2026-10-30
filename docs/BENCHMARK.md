# Benchmarks

This document compares the performance of **`cpp-tree-sitter`** (the C++ wrapper) with the raw **tree-sitter C API** to
answer a single question: *does the convenience of RAII, iterators and `ts::visit` come with a runtime cost?*

The numbers come from the [`Benchmarks`](../.github/workflows/benchmarks.yml) workflow run of 2026-10-29 (commit
`25a3cc9`): **19 configurations**, every supported C++ standard on Linux (GCC 14.2 and Clang 18.1), macOS (AppleClang
21.0, ARM64) and Windows (MSVC 19.51), all built in Release mode with IPO and the Wasm feature. The raw output of every
configuration is in [`docs/benchmarks`](benchmarks/).

## TL;DR

**No meaningful cost.** Parsing, cursor traversal (with native and Wasm grammars), query matching and incremental
reparse run at the speed of the C API: the median difference over all configurations stays within **±0.6 %**, below the
run-to-run noise of the GitHub-hosted runners, and its sign changes between configurations. Two operations have a small,
repeatable cost: **`ts::visit`** (about **+1 %** compared to a hand-written C cursor loop, up to **+4 %** with MSVC) and
**tree copy** (about **+1.3 %**, which is less than a nanosecond per copy). Query matching with GCC is the only other
case above noise (**+2 %**).

Median wrapper overhead per platform (median over the C++ standards of each platform, measured interleaved, see [Reading
the numbers](#reading-the-numbers)). Negative values mean the wrapper was faster.

| Operation | Linux GCC 14.2 | Linux Clang 18.1 | macOS AppleClang 21.0 | Windows MSVC 19.51 | All 19 configurations |
| --- | ---: | ---: | ---: | ---: | ---: |
| Parsing, flat array (100 items) | -0.1 % | +0.6 % | -0.5 % | +1.1 % | **+0.2 %** |
| Parsing, flat array (1,000 items) | -0.1 % | +0.1 % | 0.0 % | 0.0 % | **0.0 %** |
| Parsing, flat array (10,000 items) | -1.8 % | -0.7 % | +0.4 % | +0.2 % | **-0.1 %** |
| Parsing, nested object (depth 500) | 0.0 % | +0.2 % | -0.7 % | -0.4 % | **-0.1 %** |
| Cursor traversal, flat array (10,000 items) | -1.1 % | -0.3 % | -0.9 % | +0.1 % | **-0.3 %** |
| Cursor traversal, nested object (depth 500) | +0.8 % | +0.1 % | +0.5 % | +0.4 % | **+0.3 %** |
| `ts::visit`, flat array (10,000 items) ¹ | -0.6 % | +0.9 % | -3.2 % | +1.2 % | **+0.8 %** |
| `ts::visit`, nested object (depth 500) ¹ | +1.1 % | +0.6 % | -3.1 % | +3.1 % | **+0.7 %** |
| Query matching | +2.2 % | +0.9 % | +0.1 % | +0.1 % | **+0.4 %** |
| Incremental reparse | -1.2 % | -0.5 % | -0.7 % | -0.5 % | **-0.6 %** |
| Tree copy | +1.3 % | +1.3 % | +2.1 % | 0.0 % | **+1.3 %** |
| Wasm parsing, flat array (10,000 items) | -0.2 % | -0.5 % | -0.7 % | -1.2 % | **-0.5 %** |
| Wasm cursor traversal, flat array (10,000 items) | +3.0 % | 0.0 % | -0.3 % | -0.3 % | **0.0 %** |
| Wasm `ts::visit`, flat array (10,000 items) ¹ | +3.5 % | -0.8 % | +0.6 % | +5.6 % | **+1.5 %** |

¹ `ts::visit` has no interleaved measurement: the values compare the sequential medians of `ts::visit` and of the C API
cursor loop, so they are noisier (the macOS values in particular, see below).

## Reading the numbers

* **Overhead** is `(wrapper median − C API median) / C API median`. Every operation is measured twice:
  * **sequential**: all iterations of the C API, then all iterations of the wrapper;
  * **interleaved**: one C API iteration and one wrapper iteration alternately in the same loop.

  Sequential blocks run seconds apart, so they are exposed to everything that changes during a run on a shared virtual
  machine (frequency scaling, other virtual machines on the same host, the scheduler moving the process to another
  core). Interleaved iterations see the same conditions, which makes them the more reliable comparison. When the two
  disagree, trust the interleaved value.
* **Differences below ~2 % are noise.** Builds of the same code in different C++ standards differ from each other by
  this much, with either sign, and the same configuration moves by a similar amount between two runs of the workflow.
* **macOS sequential results are very noisy.** The macOS runners are ARM64 machines with performance and efficiency
  cores, and single iterations are sometimes several times slower than the rest (the standard deviation exceeds the
  median in some scenarios). This produces sequential "overheads" such as −41 % (AppleClang C++23, parsing 100 items),
  +32 % (C++23 cursor traversal), +44 % (C++23 Wasm cursor traversal) or −23 % (C++14 query matching), while the
  interleaved values of the same scenarios are −2.2 %, −3.0 %, −0.5 % and +0.1 %.
* **Fixed order in interleaved loops.** The C API iteration always runs first and the wrapper second, right after a
  structure of the same size has been freed. This can favour one side by a few percent in some files (it gave GCC −4 %
  to −7 % in interleaved parsing in an earlier run); it is an allocator/cache effect, not a faster or slower wrapper.
* **Absolute times cannot be compared between files.** Each configuration runs on its own virtual machine, and the
  hardware differs between jobs: Clang parses the 10,000-item array in about 35 ms in the C++17 job and about 73 ms in
  the C++11 job. Compare the wrapper with the C API inside one file only.

## Results

### Example run: Linux, Clang 18.1, C++17

The full output of the steadiest configuration of the run (lowest spread of all overheads), to show the absolute scale
of every operation.

| Operation | C API (median) | C++ wrapper (median) | Overhead | Interleaved overhead |
| --- | ---: | ---: | ---: | ---: |
| Parsing, flat array (100 items) | 0.3386 ms | 0.3384 ms | -0.07 % | +0.61 % |
| Parsing, flat array (1,000 items) | 3.4346 ms | 3.4354 ms | +0.02 % | +0.35 % |
| Parsing, flat array (10,000 items) | 34.8605 ms | 34.8861 ms | +0.07 % | +0.15 % |
| Parsing, nested object (depth 500) | 0.4347 ms | 0.4349 ms | +0.03 % | +0.33 % |
| Cursor traversal, flat array (10,000 items) | 15.6047 ms | 15.6415 ms | +0.24 % | -0.98 % |
| `ts::visit`, flat array (10,000 items) ¹ | 15.6047 ms | 15.7491 ms | +0.93 % | – |
| Cursor traversal, nested object (depth 500) | 0.2108 ms | 0.2112 ms | +0.19 % | +0.12 % |
| `ts::visit`, nested object (depth 500) ¹ | 0.2108 ms | 0.2124 ms | +0.73 % | – |
| Query matching (40,000 matches) | 24.2125 ms | 24.3480 ms | +0.56 % | -0.32 % |
| Incremental reparse | 0.2554 ms | 0.2558 ms | +0.17 % | -0.23 % |
| Full reparse (baseline) | 3.4246 ms | 3.4633 ms | – | – |
| Tree copy (10,000 copies) | 0.1261 ms | 0.1294 ms | +2.64 % | +2.92 % |
| Wasm parsing, flat array (100 items) | 0.4618 ms | 0.4559 ms | -1.28 % | +1.19 % |
| Wasm parsing, flat array (1,000 items) | 4.6668 ms | 4.8078 ms | +3.02 % | +0.57 % |
| Wasm parsing, flat array (10,000 items) | 47.5753 ms | 47.6254 ms | +0.11 % | -0.44 % |
| Wasm parsing, nested object (depth 500) | 0.5778 ms | 0.5841 ms | +1.08 % | -0.29 % |
| Wasm cursor traversal, flat array (10,000 items) | 15.5894 ms | 15.6374 ms | +0.31 % | -0.22 % |
| Wasm `ts::visit`, flat array (10,000 items) ¹ | 15.5894 ms | 15.7266 ms | +0.88 % | – |
| Wasm cursor traversal, nested object (depth 500) | 0.2108 ms | 0.2111 ms | +0.10 % | +0.09 % |
| Wasm `ts::visit`, nested object (depth 500) ¹ | 0.2108 ms | 0.2124 ms | +0.76 % | – |

¹ Compared with the C API cursor loop (no interleaved measurement).

### All configurations

Wrapper overhead in percent, **sequential / interleaved** (only sequential for `ts::visit`).

#### Parsing

| Configuration | Flat, 100 items | Flat, 1,000 items | Flat, 10,000 items | Nested, depth 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | +0.64 / -0.26 | +0.41 / -0.19 | +0.20 / -0.72 | +0.64 / -0.04 |
| GCC C++14 | +0.59 / -0.20 | +0.06 / +0.22 | 0.00 / +0.14 | +1.06 / +0.58 |
| GCC C++17 | -0.11 / -0.07 | -0.46 / -0.06 | -0.81 / -2.46 | -0.50 / +0.09 |
| GCC C++20 | +0.58 / +0.32 | +0.08 / -0.19 | +0.70 / -2.46 | +0.21 / 0.00 |
| GCC C++23 | -1.40 / +0.25 | -1.99 / +0.53 | 0.00 / -1.75 | -2.15 / -0.14 |
| Clang C++11 | +0.48 / +0.57 | +0.41 / -0.15 | -1.98 / -0.73 | +0.43 / +0.37 |
| Clang C++14 | -0.26 / +0.58 | -0.31 / +0.09 | +2.14 / -1.60 | +0.59 / +0.21 |
| Clang C++17 | -0.07 / +0.61 | +0.02 / +0.35 | +0.07 / +0.15 | +0.03 / +0.33 |
| Clang C++20 | +0.28 / +0.33 | -0.14 / +0.14 | -2.78 / -1.52 | +0.28 / -0.06 |
| Clang C++23 | -3.03 / +1.04 | -0.55 / +1.09 | +3.68 / -0.11 | +1.65 / +0.05 |
| AppleClang C++11 | +1.95 / -0.04 | +6.77 / -0.87 | +5.57 / +2.07 | -2.13 / -0.74 |
| AppleClang C++14 | -3.62 / -0.46 | +1.59 / +3.24 | -2.28 / +0.38 | -5.04 / -0.74 |
| AppleClang C++17 | +2.09 / -0.32 | -2.94 / 0.00 | +1.15 / -1.97 | +14.43 / -2.30 |
| AppleClang C++20 | -2.86 / -1.87 | +2.42 / +0.04 | +4.69 / -0.01 | -0.36 / -1.18 |
| AppleClang C++23 | -41.48 / -2.23 | -3.60 / -0.27 | +8.08 / +0.63 | -1.56 / -0.61 |
| MSVC C++14 | -1.25 / -0.49 | -1.51 / -0.61 | -0.47 / -0.67 | +0.36 / -0.79 |
| MSVC C++17 | -1.00 / +1.77 | -1.19 / +0.09 | -1.05 / +0.49 | -1.18 / -0.10 |
| MSVC C++20 | +1.75 / +3.28 | -0.27 / +2.13 | +3.11 / +2.26 | -0.27 / +6.50 |
| MSVC C++23 | +1.77 / +0.37 | -0.20 / +0.01 | +1.70 / -0.13 | +0.24 / -0.65 |

#### Tree traversal

| Configuration | Cursor, flat 10,000 | `ts::visit`, flat 10,000 | Cursor, nested 500 | `ts::visit`, nested 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | -0.55 / -4.08 | -0.59 | +1.97 / -6.70 | +1.92 |
| GCC C++14 | -1.86 / +1.39 | +2.43 | -1.64 / +0.03 | -0.68 |
| GCC C++17 | +1.49 / -1.11 | -0.55 | +0.37 / +0.77 | +1.08 |
| GCC C++20 | +8.06 / +0.58 | +1.55 | +3.18 / +1.83 | +1.98 |
| GCC C++23 | -4.55 / -5.23 | -11.53 | +4.70 / +1.80 | -0.13 |
| Clang C++11 | +1.60 / -0.29 | +2.16 | +0.92 / -0.23 | +0.55 |
| Clang C++14 | +0.36 / -0.12 | +3.30 | +1.03 / +0.29 | +0.65 |
| Clang C++17 | +0.24 / -0.98 | +0.93 | +0.19 / +0.12 | +0.73 |
| Clang C++20 | -1.40 / -0.33 | -2.95 | +0.82 / +0.45 | +0.39 |
| Clang C++23 | -4.01 / -2.68 | -4.12 | +0.06 / 0.00 | -0.40 |
| AppleClang C++11 | -4.93 / +0.03 | -3.16 | -0.65 / -0.75 | +4.08 |
| AppleClang C++14 | -2.24 / +3.51 | -9.85 | +7.44 / +0.35 | -3.09 |
| AppleClang C++17 | +7.69 / -1.48 | +13.47 | +4.34 / +0.45 | -8.38 |
| AppleClang C++20 | +0.86 / -0.92 | -12.48 | -0.73 / +0.94 | -0.83 |
| AppleClang C++23 | +31.52 / -3.04 | +3.48 | -5.54 / +0.82 | -5.68 |
| MSVC C++14 | -15.33 / +0.41 | -7.15 | -1.00 / +0.91 | +3.02 |
| MSVC C++17 | -1.56 / +3.40 | +1.49 | -1.46 / +0.52 | +2.08 |
| MSVC C++20 | -3.38 / -0.17 | +5.51 | -0.19 / +0.27 | +3.23 |
| MSVC C++23 | -4.36 / -1.49 | +0.82 | +0.06 / 0.00 | +3.57 |

#### Query matching, incremental reparse and tree copy

| Configuration | Query matching | Incremental reparse | Tree copy | Incremental vs full reparse (C API / wrapper) |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | -0.03 / +1.47 | -0.37 / -1.48 | +3.67 / +1.33 | -92.5 % / -92.6 % |
| GCC C++14 | +3.13 / +5.11 | +0.14 / +0.33 | +0.41 / -1.78 | -93.0 % / -93.0 % |
| GCC C++17 | +0.95 / +2.77 | +0.44 / -1.25 | -0.70 / -2.37 | -93.1 % / -93.1 % |
| GCC C++20 | +3.67 / +2.16 | +0.37 / -1.16 | +1.95 / +2.58 | -93.1 % / -93.1 % |
| GCC C++23 | +3.25 / +1.11 | -1.58 / -0.11 | +0.15 / +1.56 | -92.6 % / -92.6 % |
| Clang C++11 | +0.26 / +1.48 | +0.53 / -1.20 | +3.98 / +0.95 | -92.9 % / -92.8 % |
| Clang C++14 | +0.63 / +1.08 | +0.35 / -0.59 | +3.11 / +1.34 | -92.9 % / -92.9 % |
| Clang C++17 | +0.56 / -0.32 | +0.17 / -0.23 | +2.64 / +2.92 | -92.5 % / -92.6 % |
| Clang C++20 | -0.50 / -0.46 | +0.55 / -0.50 | -1.34 / -1.49 | -92.0 % / -91.9 % |
| Clang C++23 | +2.61 / +0.94 | -0.26 / +0.11 | +92.50 / +2.28 | -92.4 % / -92.5 % |
| AppleClang C++11 | -0.93 / +0.35 | -4.37 / -0.67 | +1.05 / +2.11 | -92.9 % / -92.9 % |
| AppleClang C++14 | -23.19 / +0.07 | -8.48 / +2.06 | +3.10 / +4.55 | -92.0 % / -93.1 % |
| AppleClang C++17 | -4.60 / -0.08 | +0.10 / +2.17 | -2.99 / +2.04 | -92.9 % / -92.5 % |
| AppleClang C++20 | +7.07 / +2.82 | -15.68 / -0.89 | +4.32 / +2.27 | -92.2 % / -94.6 % |
| AppleClang C++23 | +4.41 / -1.10 | +0.54 / -1.29 | +3.32 / +1.73 | -93.0 % / -92.1 % |
| MSVC C++14 | -1.21 / +0.36 | +1.43 / -1.00 | +1.25 / +0.16 | -92.8 % / -92.6 % |
| MSVC C++17 | +0.35 / +0.22 | +1.57 / -0.62 | -1.23 / -0.39 | -92.7 % / -93.1 % |
| MSVC C++20 | -1.16 / +0.01 | +0.29 / +1.43 | +0.87 / -0.13 | -92.8 % / -92.8 % |
| MSVC C++23 | +0.42 / -0.22 | -0.40 / -0.42 | +0.89 / +0.50 | -93.0 % / -93.1 % |

#### WebAssembly grammar: parsing

| Configuration | Flat, 100 items | Flat, 1,000 items | Flat, 10,000 items | Nested, depth 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | -0.48 / -0.23 | +0.84 / -0.18 | +1.96 / -0.23 | +0.47 / -0.30 |
| GCC C++14 | -0.90 / -1.34 | +0.21 / -0.67 | +0.08 / -5.56 | -1.06 / -0.99 |
| GCC C++17 | -0.15 / +0.47 | +0.88 / +1.64 | +1.85 / +1.43 | -0.80 / +1.24 |
| GCC C++20 | +0.16 / -0.23 | -1.57 / +1.62 | +10.85 / -0.53 | -4.73 / +1.46 |
| GCC C++23 | -1.58 / -0.61 | +1.88 / -1.98 | -1.31 / -0.09 | +0.84 / +1.29 |
| Clang C++11 | +0.03 / -0.48 | -1.35 / -0.46 | -0.72 / -1.43 | +1.09 / -0.19 |
| Clang C++14 | -0.71 / -2.24 | -1.04 / +0.13 | +0.10 / -0.46 | +0.28 / -0.73 |
| Clang C++17 | -1.28 / +1.19 | +3.02 / +0.57 | +0.11 / -0.44 | +1.08 / -0.29 |
| Clang C++20 | +0.01 / -0.92 | +0.03 / -0.28 | -1.49 / -1.28 | +0.68 / -0.41 |
| Clang C++23 | -0.72 / +0.14 | -2.10 / +1.09 | -2.05 / -0.13 | +1.18 / -0.85 |
| AppleClang C++11 | -2.41 / -0.30 | +4.43 / +0.44 | +14.66 / -1.10 | -1.83 / +1.32 |
| AppleClang C++14 | -2.49 / -1.65 | -6.55 / +0.89 | +3.02 / -0.71 | -0.95 / +1.03 |
| AppleClang C++17 | -0.46 / -0.26 | -3.31 / -3.16 | -3.12 / -0.38 | +0.25 / -0.36 |
| AppleClang C++20 | -11.40 / +1.69 | +1.52 / -0.32 | -2.30 / +0.64 | +7.60 / +0.53 |
| AppleClang C++23 | +1.83 / -0.73 | +9.34 / -0.48 | +9.12 / -0.81 | +6.62 / +0.41 |
| MSVC C++14 | +1.49 / +0.87 | -1.67 / +2.40 | +2.73 / -1.70 | -0.59 / -0.30 |
| MSVC C++17 | -0.98 / +0.83 | -0.62 / -0.22 | -2.03 / +0.02 | -0.34 / +0.19 |
| MSVC C++20 | -2.52 / -1.36 | -4.60 / -1.14 | -1.28 / -1.75 | -0.10 / -2.40 |
| MSVC C++23 | +0.84 / -0.89 | +0.39 / -5.32 | -1.77 / -0.67 | +2.71 / +6.96 |

#### WebAssembly grammar: tree traversal

| Configuration | Cursor, flat 10,000 | `ts::visit`, flat 10,000 | Cursor, nested 500 | `ts::visit`, nested 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | -2.39 / -3.43 | -1.20 | -0.34 / -6.56 | +0.38 |
| GCC C++14 | +7.94 / +4.40 | +1.98 | +3.04 / +1.88 | +1.84 |
| GCC C++17 | +7.51 / +3.03 | +5.31 | +3.40 / -0.15 | +1.17 |
| GCC C++20 | +14.72 / +5.08 | +17.11 | +2.29 / +1.60 | +1.02 |
| GCC C++23 | +7.12 / +2.33 | +3.55 | +1.39 / +2.74 | +1.67 |
| Clang C++11 | -1.28 / +0.23 | -1.79 | +1.03 / -0.27 | +1.08 |
| Clang C++14 | +1.59 / +1.29 | +5.07 | +1.45 / -0.32 | +1.10 |
| Clang C++17 | +0.31 / -0.22 | +0.88 | +0.10 / +0.09 | +0.76 |
| Clang C++20 | -0.29 / -0.01 | -0.82 | +0.50 / +0.48 | +0.13 |
| Clang C++23 | -0.92 / -1.66 | -3.51 | -0.34 / +0.29 | +0.39 |
| AppleClang C++11 | -3.37 / -0.27 | -3.49 | +4.46 / +1.72 | +6.61 |
| AppleClang C++14 | -1.84 / +0.73 | +1.51 | -2.41 / +0.41 | -2.86 |
| AppleClang C++17 | -4.14 / -0.34 | -2.24 | -1.93 / +0.44 | -1.87 |
| AppleClang C++20 | -1.82 / -1.37 | +0.60 | +0.34 / +0.46 | -3.66 |
| AppleClang C++23 | -1.72 / +0.39 | +3.92 | +44.45 / -0.46 | +17.38 |
| MSVC C++14 | +3.06 / -0.09 | +6.72 | +2.16 / +0.29 | +5.14 |
| MSVC C++17 | +1.55 / -0.81 | +4.51 | +2.14 / +0.23 | +5.56 |
| MSVC C++20 | +12.35 / +0.12 | +14.08 | +0.21 / +0.24 | +3.68 |
| MSVC C++23 | -2.78 / -0.48 | -1.80 | +0.06 / -0.13 | +3.76 |

The last column compares the incremental reparse with a full reparse of the same edited document: the incremental
mechanism saves 91.9–94.6 % of the time (12–18× faster) with both APIs, the wrapper does not change it.

## Conclusions

* **Parsing and incremental reparse are free.** The wrapper only forwards the call and owns the result; the overhead is
  centred on zero on every platform and standard, also with Wasm grammars.
* **Cursor traversal is free.** `ts::TreeCursor` is a thin owner of `TSTreeCursor`: the median interleaved overhead is
  −0.3 % on the flat tree and +0.3 % on the nested one. GCC shows +2–5 % in the interleaved Wasm traversal of the C++14
  to C++23 builds, but the native traversal of the same builds (identical cursor code, only the grammar differs) ranges
  from −5 % to +1.4 %, so this is code layout, not the wrapper.
* **`ts::visit` costs about 1 %** compared to a hand-written cursor loop. On the nested tree, where this sequential-only
  measurement is steadiest, it is 0–2 % with GCC and Clang and 2–4 % with MSVC; the flat-tree values scatter more (from
  −12 % to +17 %). This is the price of the callback abstraction (calling the visitor, handling its `VisitAction`) and
  an easy trade-off for shorter and safer code.
* **Query matching costs below 1 % with Clang, AppleClang and MSVC and about 2 % with GCC.** For every match the wrapper
  copies the captures into the `ts::QueryMatch` (reusing its storage, without allocating) and checks for an exception
  thrown by the progress callback.
* **Tree copy costs about 1.3 %.** A copy takes 13–58 ns depending on the platform (it only increments a reference count
  and allocates a small handle); wrapping the handle in a `ts::Tree` adds well below one nanosecond per copy.
* **The C++ standard does not matter.** The overheads of C++11 builds (fallbacks such as the built-in `Span` and
  `StringView`) are the same as those of C++20/23 builds (`std::span`, `std::string_view`, ranges, `std::expected`).
* **Wasm grammars behave like native ones:** the wrapper adds nothing measurable on top of the Wasm C API.

In short: **RAII and iterators in `cpp-tree-sitter` are effectively free**; `ts::visit` and tree copies cost about one
percent.

## Methodology

The benchmark measures five groups of operations that most frequently appear in real-world tree-sitter usage, each with
the C API and with the wrapper:

1. **Parsing**: from a source string to a syntax tree, on a flat JSON array of 100, 1,000 and 10,000 objects and on a
   nested object of depth 500.
2. **Tree traversal**: a full depth-first walk of the 10,000-item and the nested tree with the C API cursor, the wrapper
   cursor and `ts::visit`.
3. **Query matching**: the `(string) @value` query executed on the 10,000-item tree (40,000 matches), counting the
   matches.
4. **Incremental reparse**: an edit appending an element to the 1,000-item array followed by an incremental reparse,
   compared with a full reparse of the edited document.
5. **Tree copy**: `Tree::copy()` and `ts_tree_copy` of the 10,000-item tree, measured in batches of 10,000 copies per
   sample (a single copy is too fast for the stopwatch); the copies are released outside the measured time.

When the benchmark is built with the Wasm feature (`CPP_TS_FEATURE_WASM=ON`), parsing and traversal are repeated with
the JSON grammar compiled to WebAssembly, loaded through a `WasmStore`.

* **Warm-up:** every measurement starts with 20 warm-up iterations that are not timed.
* **Iterations:** 200 timed iterations for parsing, 100 for traversal, query matching, incremental reparse and tree
  copy.
* **Statistics:** average, median, minimum, maximum and standard deviation are printed; all overheads use the
  **median**, which resists outliers.
* **Interleaved measurements:** see [Reading the numbers](#reading-the-numbers).

### C++ standards compliance

The benchmark code compiles without modification under C++11 through C++23. It uses newer language features when they
are available rather than avoiding them:

* Under **C++23**, objects are created through the non-throwing `Parser::create` / `Query::create` / `WasmStore::create`
  factories, which return `std::expected<T, ts::Error>`. Before C++23 these factories do not exist (guarded by `#if
  CPP_TS_HAS_EXPECTED`), so the benchmark uses the throwing constructors.
* Passing the old tree to `Parser::parseString` for incremental parsing accepts the same syntax in every standard
  (`tree`, `&tree`, `nullptr`, `{}` and, since C++17, `std::nullopt` or `std::ref(tree)`).

> **Note:** If you build `cpp-tree-sitter` as a compiled library (not header-only), its standard is
> `CPP_TS_CXX_STANDARD`, the same one used for the benchmark.

## Automated runs (GitHub Actions)

The [`Benchmarks`](../.github/workflows/benchmarks.yml) workflow (started manually from the *Actions* tab) builds the
suite in Release mode with IPO and Wasm enabled for every compiler/OS pair (GCC and Clang on Linux, AppleClang on macOS,
MSVC on Windows) and every supported C++ standard (MSVC has no C++11 mode). Each run writes one file per configuration
to [`docs/benchmarks`](benchmarks/), named `<system>-<compiler>-cxx<standard>.txt` (e.g. `linux-gcc-cxx20.txt`), and
commits them to the branch the workflow was started from. Every file starts with the configuration, the runner image,
the commit and the date of the run.

GitHub-hosted runners are shared virtual machines, so absolute times vary between jobs and runs. Compare the wrapper/C
API overhead inside one file rather than raw times between files.

## Running the benchmarks

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCPP_TS_BUILD_BENCHMARK=ON
cmake --build build --config Release
./build/benchmark/cpp-tree-sitter-benchmark
```

Pass `--no-wait` to skip the final "Press Enter" prompt (it is also skipped automatically when the standard input is not
a terminal).

To include the Wasm section, configure with `-DCPP_TS_FEATURE_WASM=ON`. The JSON grammar compiled to WebAssembly is
downloaded by CMake from the release of the grammar; another `.wasm` file can be used by defining
`CPP_TS_BENCHMARK_WASM_JSON_PATH`. If the module cannot be loaded, the benchmark prints a warning and continues without
the Wasm section.

### Running for a specific C++ standard

The benchmark is built in the standard selected by `CPP_TS_CXX_STANDARD` (C++11 to C++23), the same one used for the
library. Example for C++17:

```bash
cmake -B build-cxx17 -DCMAKE_BUILD_TYPE=Release -DCPP_TS_BUILD_BENCHMARK=ON -DCPP_TS_CXX_STANDARD=17
cmake --build build-cxx17 --config Release
./build-cxx17/benchmark/cpp-tree-sitter-benchmark
```

The header of the output confirms the detected standard (e.g. `C++ standard detected: C++17`).
