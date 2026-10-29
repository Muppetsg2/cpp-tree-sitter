# Benchmarks

This document compares the performance of **`cpp-tree-sitter`** (the C++ wrapper) with the raw **tree-sitter C API** to
answer a single question: *does the convenience of RAII, iterators and `ts::visit` come with a runtime cost?*

The numbers come from the [`Benchmarks`](../.github/workflows/benchmarks.yml) workflow run of 2026-10-28 (commit
`f1bc03f`): **19 configurations**, every supported C++ standard on Linux (GCC 14.2 and Clang 18.1), macOS (AppleClang
21.0, ARM64) and Windows (MSVC 19.51), all built in Release mode with IPO and the Wasm feature. The raw output of every
configuration is in [`docs/benchmarks`](benchmarks/).

## TL;DR

**No meaningful cost.** Parsing, cursor traversal (with native and Wasm grammars) and incremental reparse run at the
speed of the C API: the median difference over all configurations stays within **±1.5 %**, which is below the run-to-run
noise of the GitHub-hosted runners, and its sign changes between configurations. The only operations with a small,
repeatable cost are **`ts::visit`** (typically **+1–3 %** compared to a hand-written C cursor loop) and **query
matching** (about **+1.5 %** overall, **+4 %** with GCC).

Median wrapper overhead per platform (median over the C++ standards of each platform, measured interleaved, see [Reading
the numbers](#reading-the-numbers)). Negative values mean the wrapper was faster.

| Operation | Linux GCC 14.2 | Linux Clang 18.1 | macOS AppleClang 21.0 | Windows MSVC 19.51 | All 19 configurations |
| --- | ---: | ---: | ---: | ---: | ---:|
| Parsing, flat array (100 items) | -4.4 % | +0.8 % | +0.4 % | -0.4 % | **-0.2 %** |
| Parsing, flat array (1,000 items) | -1.1 % | -0.1 % | +0.1 % | -0.2 % | **-0.3 %** |
| Parsing, flat array (10,000 items) | -1.5 % | -0.7 % | -1.2 % | -0.4 % | **-1.2 %** |
| Parsing, nested object (depth 500) | -0.4 % | -0.3 % | +0.3 % | +1.7 % | **-0.1 %** |
| Cursor traversal, flat array (10,000 items) | -0.4 % | -0.3 % | -3.4 % | 0.0 % | **-0.3 %** |
| Cursor traversal, nested object (depth 500) | +0.8 % | +0.1 % | -0.8 % | +0.2 % | **+0.3 %** |
| `ts::visit`, flat array (10,000 items) ¹ | -6.9 % | +0.1 % | -5.7 % | +1.3 % | **-1.0 %** |
| `ts::visit`, nested object (depth 500) ¹ | +0.5 % | +0.8 % | +7.6 % | +3.3 % | **+1.7 %** |
| Query matching | +4.3 % | +1.6 % | -0.3 % | +0.2 % | **+1.6 %** |
| Incremental reparse | -0.1 % | -1.2 % | -1.2 % | 0.0 % | **-0.7 %** |
| Wasm parsing, flat array (10,000 items) | -0.3 % | -1.0 % | -2.3 % | -0.9 % | **-1.0 %** |
| Wasm cursor traversal, flat array (10,000 items) | +1.5 % | +0.3 % | -0.8 % | +0.4 % | **+0.5 %** |
| Wasm `ts::visit`, flat array (10,000 items) ¹ | +2.6 % | +2.4 % | 0.0 % | +4.4 % | **+2.5 %** |

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
  this much, with either sign.
* **macOS sequential results are very noisy.** The macOS runners are ARM64 machines with performance and efficiency
  cores; standard deviations reach 30–65 % of the median in some scenarios. This produces sequential "overheads" such as
  +61 % and +83 % (AppleClang C++20 parsing), −32 % (C++14 parsing) or +106 % (C++14 Wasm `ts::visit`), while the
  interleaved values of the same scenarios stay within ±3 % and the minimal times of both APIs are close.
* **Fixed order in interleaved loops.** The C API iteration always runs first and the wrapper second, right after a tree
  of the same size has been freed. With GCC this gives the wrapper a small but systematic advantage in interleaved
  parsing (−4 % to −7 % in some files), most likely an allocator/cache effect, not a faster wrapper.
* **Absolute times cannot be compared between files.** Each configuration runs on its own virtual machine, and the
  hardware differs between jobs: GCC parses the 10,000-item array in about 30 ms in the C++17 job and about 60 ms in the
  C++20 job. Compare the wrapper with the C API inside one file only.
* **Tree copy is not reported for this run.** `ts_tree_copy` only increments a reference count and allocates a small
  handle; one copy takes about 25 ns, below the resolution of the printed times (0.0001 ms), so the percentages of this
  run (e.g. +25 % or −49 %) are rounding artefacts. The benchmark now measures batches of 10,000 copies per sample, so
  the next run of the workflow reports a meaningful tree copy overhead.

## Results

### Example run: Linux, Clang 18.1, C++23

The full output of the steadiest configuration of the run (lowest spread of all overheads), to show the absolute scale
of every operation.

| Operation | C API (median) | C++ wrapper (median) | Overhead | Interleaved overhead |
| --- | ---: | ---: | ---: | ---: |
| Parsing, flat array (100 items) | 0.6254 ms | 0.6207 ms | -0.75 % | -0.34 % |
| Parsing, flat array (1,000 items) | 6.3467 ms | 6.3023 ms | -0.70 % | -0.73 % |
| Parsing, flat array (10,000 items) | 74.4722 ms | 73.3727 ms | -1.48 % | -1.45 % |
| Parsing, nested object (depth 500) | 0.8169 ms | 0.8152 ms | -0.21 % | -0.42 % |
| Cursor traversal, flat array (10,000 items) | 29.3565 ms | 29.1519 ms | -0.70 % | -0.31 % |
| `ts::visit`, flat array (10,000 items) ¹ | 29.3565 ms | 29.0688 ms | -0.98 % | – |
| Cursor traversal, nested object (depth 500) | 0.3536 ms | 0.3548 ms | +0.35 % | +0.48 % |
| `ts::visit`, nested object (depth 500) ¹ | 0.3536 ms | 0.3513 ms | -0.66 % | – |
| Query matching (40,000 matches) | 47.7472 ms | 48.3966 ms | +1.36 % | +1.24 % |
| Incremental reparse | 0.4572 ms | 0.4681 ms | +2.38 % | -1.34 % |
| Full reparse (baseline) | 6.2794 ms | 6.2966 ms | – | – |
| Tree copy ² | < 0.0001 ms | < 0.0001 ms | – | – |
| Wasm parsing, flat array (100 items) | 0.8771 ms | 0.8856 ms | +0.97 % | +0.20 % |
| Wasm parsing, flat array (1,000 items) | 9.0327 ms | 9.0104 ms | -0.25 % | +0.29 % |
| Wasm parsing, flat array (10,000 items) | 90.5193 ms | 90.9480 ms | +0.47 % | -1.00 % |
| Wasm parsing, nested object (depth 500) | 1.1124 ms | 1.1280 ms | +1.41 % | +0.01 % |
| Wasm cursor traversal, flat array (10,000 items) | 29.4653 ms | 29.5537 ms | +0.30 % | +0.33 % |
| Wasm `ts::visit`, flat array (10,000 items) ¹ | 29.4653 ms | 29.2785 ms | -0.63 % | – |
| Wasm cursor traversal, nested object (depth 500) | 0.3544 ms | 0.3549 ms | +0.14 % | -0.09 % |
| Wasm `ts::visit`, nested object (depth 500) ¹ | 0.3544 ms | 0.3521 ms | -0.64 % | – |

¹ Compared with the C API cursor loop (no interleaved measurement). ² Below the resolution of the printed times, see
[Reading the numbers](#reading-the-numbers).

### All configurations

Wrapper overhead in percent, **sequential / interleaved** (only sequential for `ts::visit`).

#### Parsing

| Configuration | Flat, 100 items | Flat, 1,000 items | Flat, 10,000 items | Nested, depth 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | -2.52 / -5.21 | +0.15 / -0.45 | +2.73 / -2.98 | +1.51 / -0.83 |
| GCC C++14 | -0.84 / -0.48 | -0.79 / -1.05 | -0.62 / -1.52 | -3.03 / -0.17 |
| GCC C++17 | +0.44 / 0.00 | +1.89 / 0.00 | -1.12 / +0.23 | -3.15 / -0.41 |
| GCC C++20 | +0.46 / -7.15 | +0.23 / -5.75 | -2.01 / -6.35 | -4.02 / -0.05 |
| GCC C++23 | +0.90 / -4.44 | +0.81 / -4.03 | -10.58 / -1.26 | -1.06 / -2.03 |
| Clang C++11 | -3.57 / -0.19 | +8.41 / -0.32 | -14.78 / +0.99 | +0.90 / -0.34 |
| Clang C++14 | -0.08 / +0.89 | -0.33 / -0.06 | -2.35 / -0.69 | +0.47 / -23.53 |
| Clang C++17 | +0.19 / +0.98 | -0.05 / +0.43 | -3.06 / -1.96 | +0.44 / +0.52 |
| Clang C++20 | +0.65 / +0.76 | -0.06 / +0.35 | -3.04 / +0.13 | +0.39 / +0.23 |
| Clang C++23 | -0.75 / -0.34 | -0.70 / -0.73 | -1.48 / -1.45 | -0.21 / -0.42 |
| AppleClang C++11 | +4.96 / +0.73 | +3.13 / -2.66 | -7.00 / -0.40 | -0.45 / +1.69 |
| AppleClang C++14 | -11.19 / +3.35 | -4.34 / +0.13 | -32.45 / -2.59 | +8.36 / +0.49 |
| AppleClang C++17 | +7.18 / +0.21 | +0.58 / +1.77 | -8.90 / +0.71 | -0.28 / -0.66 |
| AppleClang C++20 | +61.21 / +0.40 | +43.51 / -1.79 | -13.20 / -1.24 | +82.72 / +0.33 |
| AppleClang C++23 | -2.30 / -0.58 | +9.35 / +0.06 | +15.63 / -1.73 | +0.70 / -2.76 |
| MSVC C++14 | +1.28 / -0.69 | -0.88 / -0.29 | -0.23 / -0.58 | +0.59 / -0.09 |
| MSVC C++17 | +0.46 / +2.83 | -0.38 / -0.15 | -0.46 / -0.15 | +0.14 / +1.30 |
| MSVC C++20 | -0.17 / -0.25 | -0.99 / -0.04 | -0.77 / -1.32 | -1.05 / +2.09 |
| MSVC C++23 | -2.52 / -0.57 | -0.18 / -0.68 | -0.67 / +0.26 | +0.54 / +2.46 |

#### Tree traversal

| Configuration | Cursor, flat 10,000 | `ts::visit`, flat 10,000 | Cursor, nested 500 | `ts::visit`, nested 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | -6.94 / -0.22 | -7.91 | +1.34 / +0.75 | +0.53 |
| GCC C++14 | +0.32 / -0.37 | -1.00 | +1.49 / +0.69 | +1.02 |
| GCC C++17 | -1.48 / -3.95 | -8.53 | -0.08 / -0.13 | +0.54 |
| GCC C++20 | +0.44 / +1.76 | +2.31 | +2.37 / +2.32 | +2.81 |
| GCC C++23 | -0.44 / -7.22 | -6.86 | +0.94 / +1.99 | +0.49 |
| Clang C++11 | -6.62 / -3.01 | +0.05 | +0.04 / +0.06 | +3.11 |
| Clang C++14 | +4.53 / -0.15 | +0.65 | +1.81 / -0.73 | +1.67 |
| Clang C++17 | +0.89 / -0.31 | +1.53 | +0.18 / +0.42 | +0.85 |
| Clang C++20 | -5.27 / -0.14 | -5.05 | -0.01 / -0.19 | +0.37 |
| Clang C++23 | -0.70 / -0.31 | -0.98 | +0.35 / +0.48 | -0.66 |
| AppleClang C++11 | +1.38 / -4.63 | -6.40 | +2.90 / +1.39 | +10.40 |
| AppleClang C++14 | +0.05 / -2.39 | -6.95 | +0.77 / +0.34 | -1.72 |
| AppleClang C++17 | +24.16 / -5.26 | -3.18 | +5.79 / -1.82 | +7.59 |
| AppleClang C++20 | -8.64 / +0.90 | -5.74 | +8.91 / -0.80 | +80.04 |
| AppleClang C++23 | -4.58 / -3.35 | +1.77 | -5.39 / -4.51 | -0.83 |
| MSVC C++14 | +9.86 / +0.07 | +1.24 | +0.15 / +0.11 | +3.53 |
| MSVC C++17 | -0.88 / -0.69 | +0.94 | +5.13 / +1.25 | +3.15 |
| MSVC C++20 | -4.81 / +0.04 | +6.24 | -1.07 / +0.32 | +2.78 |
| MSVC C++23 | -1.10 / -0.01 | +1.27 | -0.27 / +0.11 | +3.36 |

#### Query matching and incremental reparse

| Configuration | Query matching | Incremental reparse | Incremental vs full reparse (C API) | Incremental vs full reparse (wrapper) |
| --- | ---:| ---:| ---:| ---:|
| GCC C++11 | +3.63 / +4.39 | +1.21 / +0.86 | -92.8 % | -92.7 % |
| GCC C++14 | +2.95 / +4.32 | +0.36 / -1.46 | -92.9 % | -92.9 % |
| GCC C++17 | +1.11 / +6.07 | +5.35 / -0.14 | -93.3 % | -92.6 % |
| GCC C++20 | +2.03 / +4.29 | -0.27 / -0.62 | -92.7 % | -92.7 % |
| GCC C++23 | +5.40 / +3.48 | +0.49 / -0.12 | -92.7 % | -92.7 % |
| Clang C++11 | -10.93 / +1.30 | -2.23 / -0.69 | -92.5 % | -92.7 % |
| Clang C++14 | +1.02 / +1.72 | -0.57 / -1.17 | -92.7 % | -92.7 % |
| Clang C++17 | +1.30 / +1.92 | -0.84 / -1.17 | -92.6 % | -92.7 % |
| Clang C++20 | -0.06 / +1.62 | +0.59 / -0.78 | -92.8 % | -92.7 % |
| Clang C++23 | +1.36 / +1.24 | +2.38 / -1.34 | -92.7 % | -92.6 % |
| AppleClang C++11 | -9.55 / -0.35 | +6.53 / +0.38 | -92.8 % | -92.7 % |
| AppleClang C++14 | +2.76 / -0.71 | +3.57 / -1.37 | -93.8 % | -93.1 % |
| AppleClang C++17 | -19.89 / -1.54 | -3.15 / -3.32 | -93.2 % | -92.9 % |
| AppleClang C++20 | -6.18 / +1.69 | -12.86 / +16.67 | -95.0 % | -95.4 % |
| AppleClang C++23 | -4.62 / +5.03 | -0.17 / -1.20 | -93.2 % | -94.3 % |
| MSVC C++14 | -0.28 / +0.21 | +1.48 / -0.18 | -93.2 % | -93.1 % |
| MSVC C++17 | +0.73 / -0.43 | +1.78 / -2.19 | -92.9 % | -92.8 % |
| MSVC C++20 | +0.99 / +0.26 | +0.58 / +4.27 | -92.9 % | -92.8 % |
| MSVC C++23 | +0.69 / +0.21 | -0.84 / +0.26 | -92.9 % | -93.0 % |

#### WebAssembly grammar: parsing

| Configuration | Flat, 100 items | Flat, 1,000 items | Flat, 10,000 items | Nested, depth 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | +1.60 / +2.99 | -0.28 / +0.13 | -12.97 / +0.80 | +1.15 / -0.61 |
| GCC C++14 | -0.30 / +3.48 | +3.19 / -1.10 | +10.23 / -0.31 | +2.66 / +1.02 |
| GCC C++17 | -5.57 / +1.82 | +0.49 / -0.94 | +16.06 / -1.85 | +4.84 / +0.44 |
| GCC C++20 | -0.60 / -2.17 | +3.26 / -0.35 | +13.87 / +0.75 | -0.21 / -0.85 |
| GCC C++23 | -0.39 / -0.04 | +2.66 / -0.73 | +3.17 / -0.75 | +0.76 / 0.00 |
| Clang C++11 | -2.25 / -0.91 | -0.99 / +0.88 | -3.28 / +0.25 | -1.58 / +0.47 |
| Clang C++14 | -1.12 / -1.37 | -0.16 / -0.44 | +9.45 / -0.98 | +0.81 / -0.63 |
| Clang C++17 | -0.28 / -0.14 | -0.78 / -0.98 | +9.17 / -3.09 | -0.58 / -0.01 |
| Clang C++20 | -0.06 / -0.57 | -1.35 / -0.05 | +10.41 / +5.58 | +12.41 / -2.92 |
| Clang C++23 | +0.97 / +0.20 | -0.25 / +0.29 | +0.47 / -1.00 | +1.41 / +0.01 |
| AppleClang C++11 | +3.15 / -2.86 | -5.01 / -1.93 | +3.90 / -0.24 | +4.00 / -2.07 |
| AppleClang C++14 | +3.16 / +0.05 | +4.36 / +2.45 | -4.42 / -2.50 | -4.07 / +0.56 |
| AppleClang C++17 | -6.17 / +1.68 | +11.94 / +0.47 | -20.20 / -1.51 | -2.59 / +0.86 |
| AppleClang C++20 | +19.69 / +14.78 | -12.96 / -5.30 | -6.92 / -2.28 | +4.48 / +0.79 |
| AppleClang C++23 | -5.13 / -3.07 | +3.38 / -5.74 | -8.50 / -2.98 | -12.34 / -1.94 |
| MSVC C++14 | -0.25 / +3.92 | -1.47 / -0.16 | -0.09 / +2.50 | +2.79 / +0.02 |
| MSVC C++17 | +2.43 / -2.46 | +0.93 / -2.31 | +2.02 / -2.36 | +1.86 / -0.79 |
| MSVC C++20 | +0.68 / -2.24 | +3.54 / +0.25 | -1.14 / -0.51 | +3.37 / -0.85 |
| MSVC C++23 | +6.21 / -2.69 | -0.85 / -1.36 | -4.94 / -1.33 | -0.30 / -0.98 |

#### WebAssembly grammar: tree traversal

| Configuration | Cursor, flat 10,000 | `ts::visit`, flat 10,000 | Cursor, nested 500 | `ts::visit`, nested 500 |
| --- | ---: | ---: | ---: | ---: |
| GCC C++11 | +4.35 / -1.92 | +4.57 | +2.35 / +0.55 | +3.01 |
| GCC C++14 | +1.69 / +1.58 | +2.22 | +2.62 / +0.92 | +3.13 |
| GCC C++17 | -1.72 / +0.99 | -1.06 | +0.75 / +0.59 | +0.01 |
| GCC C++20 | +2.39 / +1.47 | +2.56 | +3.31 / +2.97 | +3.47 |
| GCC C++23 | +5.16 / +8.20 | +11.32 | +1.77 / +1.61 | +1.22 |
| Clang C++11 | +0.56 / -2.33 | +2.52 | -1.18 / +0.13 | +0.41 |
| Clang C++14 | +0.78 / +0.22 | +0.34 | +1.99 / -0.25 | +0.90 |
| Clang C++17 | +0.59 / +0.54 | +2.44 | +0.28 / +0.08 | +0.83 |
| Clang C++20 | -0.51 / +0.65 | +3.28 | -0.06 / -0.23 | +0.34 |
| Clang C++23 | +0.30 / +0.33 | -0.63 | +0.14 / -0.09 | -0.64 |
| AppleClang C++11 | -10.69 / -0.77 | +1.31 | -14.35 / +1.27 | -36.62 |
| AppleClang C++14 | -0.21 / +0.72 | -1.66 | +93.18 / -0.16 | +105.88 |
| AppleClang C++17 | -4.54 / -1.54 | -2.16 | -12.10 / +0.68 | -12.04 |
| AppleClang C++20 | -5.29 / +6.66 | +13.76 | +99.63 / +1.27 | -12.44 |
| AppleClang C++23 | -6.12 / -3.66 | -0.04 | -19.80 / +1.63 | -18.65 |
| MSVC C++14 | +0.45 / +0.34 | +4.18 | -0.50 / -3.93 | +3.08 |
| MSVC C++17 | +1.87 / -0.46 | +4.67 | +1.41 / -0.07 | +3.98 |
| MSVC C++20 | -3.07 / +0.53 | +8.05 | +1.98 / +0.76 | +4.22 |
| MSVC C++23 | -2.42 / +0.64 | +3.25 | -0.77 / -0.38 | +2.80 |

The incremental reparse columns compare the incremental reparse with a full reparse of the same edited document: the
incremental mechanism saves 92.5–95.4 % of the time (13–22× faster) with both APIs, the wrapper does not change it.

## Conclusions

* **Parsing and incremental reparse are free.** The wrapper only forwards the call and owns the result; the overhead is
  centred on zero on every platform and standard, also with Wasm grammars.
* **Cursor traversal is free.** `ts::TreeCursor` is a thin owner of `TSTreeCursor`: the median interleaved overhead is
  between −0.3 % and +0.5 %.
* **`ts::visit` costs typically 1–3 %** compared to a hand-written cursor loop (up to 4–5 % with MSVC on the Wasm tree).
  The cost is most visible on the deep nested tree and with Wasm grammars, and is the price of the callback abstraction
  (calling the visitor, handling its `VisitAction`): an easy trade-off for shorter and safer code.
* **Query matching costs about +1.6 % with Clang and +4 % with GCC**, nothing measurable with MSVC and AppleClang. For
  every match the wrapper copies the captures into the `ts::QueryMatch` (reusing its storage, without allocating) and
  checks for an exception thrown by the progress callback; with 40,000 matches this becomes a small, measurable cost.
* **The C++ standard does not matter.** The overheads of C++11 builds (fallbacks such as the built-in `Span` and
  `StringView`) are the same as those of C++20/23 builds (`std::span`, `std::string_view`, ranges, `std::expected`).
* **Wasm grammars behave like native ones:** the wrapper adds nothing measurable on top of the Wasm C API.

In short: **RAII and iterators in `cpp-tree-sitter` are effectively free**; `ts::visit` and query matching cost a few
percent at most.

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
5. **Tree copy**: `Tree::copy()` and `ts_tree_copy`, measured in batches of 10,000 copies per sample (a single copy is
   too fast for the stopwatch); the copies are released outside the measured time.

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
