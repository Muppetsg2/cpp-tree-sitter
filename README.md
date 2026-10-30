# cpp-tree-sitter

[![Tests](https://github.com/Muppetsg2/cpp-tree-sitter/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/Muppetsg2/cpp-tree-sitter/actions/workflows/tests.yml)

`cpp-tree-sitter` is a modern, lightweight **C++11 (and newer)** wrapper around the [tree-sitter](https://github.com/tree-sitter/tree-sitter/) parsing library. 
It provides a clean, RAII-compliant interface and robust CMake integration to simplify working with syntax trees in C++ projects.
> **Note on this Fork:** This project is a significant evolution of the [original repository by Nick Sumner](https://github.com/nsumner/cpp-tree-sitter). It supports legacy C++ standards (back to C++11), full Query/Wasm support, and advanced tree traversal utilities.

## Key Features
* **Modern RAII API:** Automatic memory management for `Parsers`, `Trees`, `Nodes`, and `Cursors` using standard smart pointers.
* **Broad Compatibility:** Supports **C++11, C++14, C++17, C++20 and C++23**. Modern features (`std::string_view`, `std::span`, concepts, `<ranges>`, `operator<=>`, `std::expected`, deducing `this`) are enabled with **feature-test macros**, so every toolchain gets exactly what its standard library really provides, with fallbacks for older standards.
* **STL-style Iterators & Ranges:** `for` loops over children (`getChildren()`, `getNamedChildren()`), lazy pre-order walks (`ts::preorder`) and query results (`cursor.matches()`, `cursor.captures()`). In C++20 they are `std::ranges` views/input ranges and compose with `std::views`.
* **Tree Visitor & Fold:** A cursor-based DFS visitor (`ts::visit`) accepting callbacks (`bool`, `void` or `ts::VisitAction`) or visitor objects with `onEnter`/`onLeave`, plus a bottom-up `ts::fold<R>`.
* **Exception Safety:** Exceptions thrown from callbacks (read/decode, logger, progress) never cross tree-sitter's C frames: the operation is stopped and the exception is rethrown to the caller.
* **Wasm Support:** High-level wrappers for WebAssembly-based grammars.
* **Query Engine:** Full support for tree-sitter queries (patterns and captures).
* **CMake Integration:** Seamless dependency management via [CPM.cmake](https://github.com/cpm-cmake/cpm.cmake).

## Requirements
* **Compiler:** C++11 compatible or newer (C++20/23 recommended).
* **Build System:** [CMake](https://cmake.org/) 3.30 or newer.

## Using in a CMake Project
The easiest way to integrate `cpp-tree-sitter` is via CPM. Adding this wrapper automatically makes the core `tree-sitter` library available as well.

```cmake
cmake_minimum_required(VERSION 3.30)

project(MyParser)

set(CMAKE_CXX_STANDARD 20) # Works with 11, 14, 17, 20, 23
set(CPP_TS_CXX_STANDARD 20 CACHE STRING "") # Force the static library to build in C++20

include(cmake/CPM.cmake)

# Download the wrapper and tree-sitter core
CPMAddPackage(
    NAME cpp-tree-sitter
    GIT_REPOSITORY https://github.com/Muppetsg2/cpp-tree-sitter.git
    VERSION 0.27.1
)

# Download a grammar (e.g., JSON) and make it a CMake target
CPPTSAddGrammar(
    NAME tree-sitter-json
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json.git
    VERSION 0.24.8
)

add_executable(demo main.cpp)
# Link the compiled static library
target_link_libraries(demo PRIVATE cpp-tree-sitter::cpp-tree-sitter tree-sitter-json)

# OR use the header-only version to automatically inherit your project's C++ standard:
# target_link_libraries(demo PRIVATE cpp-tree-sitter::header-only tree-sitter-json)
```

## CMake Configuration

`cpp-tree-sitter` provides several CMake options to customize the build process and manage dependencies.

### Build Options

You can set these options using `-DOPTION=VALUE` during the CMake configuration phase.

|             Option            |      Default      |                                  Description                                   |
|:------------------------------|:-----------------:|:-------------------------------------------------------------------------------|
|     `CPP_TS_BUILD_TESTS`      | ON (if top-level) |                       Build unit tests for the library.                        |
| `CPP_TS_TEST_BUILTIN_FRAMEWORK` |      OFF        | Run the tests with the built-in framework instead of Catch2 (always used in C++11), see [tests/README.md](tests/README.md). |
|    `CPP_TS_BUILD_BENCHMARK`   | ON (if top-level) |                       Build benchmark for the library.                         |
|     `CPP_TS_FEATURE_WASM`     |        OFF        |               Enable WebAssembly support (requires `wasmtime`).                |
|     `CPP_TS_AMALGAMATED`      |        ON         | Build `tree-sitter` core using the amalgamated `lib.c` for faster compilation. |
| `CPP_TS_MSVC_STATIC_RUNTIME`  |        OFF        |                    Link static MSVC runtime (/MT or /MTd).                     |

### Path and Version Variables

These variables control dependency versions and where the library looks for external tools or grammars.

* `CPP_TS_GRAMMAR_PATH`: A global directory where you store your Tree-sitter grammars. If set, `CPPTSAddGrammar` will first look here before attempting to download from Git.

* `CPP_TS_CXX_STANDARD`: Specifies the C++ standard used to compile the `cpp-tree-sitter` static library. Valid options are `11`, `14`, `17`, `20`, or `23` (Default: `20`). **Note:** This setting does not affect the `cpp-tree-sitter-header-only` target, which automatically adapts to the C++ standard of the consuming project.

* `CPP_TS_WASMTIME_VERSION`: Defines the version of `wasmtime` to fetch if WASM support is enabled (Default: `48.0.1`, the version pinned by tree-sitter `0.27.1`). You can override it if you require a different version.

* `CPP_TS_WASMTIME_PATH`: Path to a local `wasmtime` installation. If not set and WASM is enabled, CMake will automatically download the appropriate binary for your system.

* `CPM_SOURCE_CACHE` (variable or environment variable): Where CPM keeps downloaded dependencies. When `cpp-tree-sitter` is the top-level project and it is not set, the cache is `cpm_cache` in the build directory, so nothing is written outside of it. `CMakePresets.json` sets `out/cpm_cache`, shared by all presets (each dependency, e.g. `wasmtime`, is downloaded once); deleting `out` removes all builds and the cache. `CMAKE_DISABLE_SOURCE_CHANGES` (also set by the presets) stays active, it is only suspended while CPM adds a package, because CPM writes its bookkeeping files into the cache.

* `CPP_TS_WASM_DIR`: (Default: `${CMAKE_BINARY_DIR}/wasm_files`) Directory where downloaded `.wasm` grammar files are stored. It is only valid if you pass option `FIND_ALSO_WASM_FILE` or `FIND_ONLY_WASM_FILE` to `CPPTSAddGrammar`

## Grammar Management

The `CPPTSAddGrammar` function automates fetching and building grammars.

|       Argument        |                              Description                              |
|:----------------------|:----------------------------------------------------------------------|
|        `NAME`         |          The name of the grammar (e.g., `tree-sitter-json`).          |
|   `GIT_REPOSITORY`    |                    URL to the grammar repository.                     |
|  `VERSION`/`GIT_TAG`  |                 Specific version or tag to download.                  |
|     `SOURCE_DIR`      |       Path to a local grammar source (overrides Git download).        |
| `FIND_ALSO_WASM_FILE` | If set, attempts to download/find the `.wasm` binary for the grammar. |
| `FIND_ONLY_WASM_FILE` | Skip building the static library and only look for the `.wasm` file.  |

### Helper Functions

* `CPPTSCopyWasmtime(TARGET <target>)`: (Windows) Copies `wasmtime.dll` to the target's output directory.

## Quick Start Example

This example demonstrates parsing a JSON string and using the visitor to inspect nodes.

```cpp
#include <iostream>
#include <string>
#include <functional>
#include <cpp-tree-sitter.hpp>

// Extern declaration for the grammar function
extern "C" TSLanguage* tree_sitter_json();

int main() {
    // Initialize language and parser
    ts::Language language(tree_sitter_json());
    ts::Parser parser(language);

    // Parse source code into a syntax tree
    std::string code = "[1, null, \"example\"]";
    ts::Tree tree = parser.parseString(code);
    ts::Node root = tree.getRootNode();

    // Use the Visitor for easy traversal (New in this fork)
    ts::visit(root, [](ts::Node node) {
        if (node.isNamed()) {
            std::cout << "Node: " << node.getType() << " at "
                      << node.getByteRange().start << "\n";
        }
        // `true` / ts::VisitAction::Stop ends the walk, ts::VisitAction::SkipChildren skips the subtree
        return ts::VisitAction::Continue;
    });

    // Or use STL-style iteration
    for (const ts::Node child : root.getNamedChild(0).getNamedChildren()) {
        std::cout << "Child type: " << child.getType() << "\n";
    }

    // Or a lazy pre-order walk (a std::ranges view in C++20)
    for (const ts::Node node : ts::preorder(root)) {
        std::cout << node.getType() << "\n";
    }

    return 0; // Resources are cleaned up automatically via RAII
}
```

## Major Improvements in this Fork

### Memory & Safety
This fork implements a **Full RAII** architecture. Every C resource is owned by a `std::unique_ptr` with a stateless deleter (`details::FreeHelper`), so owners are as small as a raw pointer. `Language` copies use `ts_language_copy`, `Tree` copies `ts_tree_copy` and `Query` copies `ts_query_copy`. Moved-from objects stay safe to use, and state referenced by tree-sitter during an operation (logger, query progress callback) lives on the heap, so it survives moves and temporaries.

### Compatibility Layer
The library features a custom `StringView` and `Span` for older environments and enables `std::string_view`, `std::optional`, `std::span`, concepts, `<ranges>`, `operator<=>` and `std::expected` through feature-test macros (`CPP_TS_HAS_STRING_VIEW`, `CPP_TS_HAS_OPTIONAL`, `CPP_TS_HAS_SPAN`, `CPP_TS_HAS_RANGES`, `CPP_TS_HAS_EXPECTED`, ...). For the compiled library the enabled API is capped by `CPP_TS_CXX_STANDARD`, so the library and its consumers always see the same declarations. Optional arguments (`old_tree`, options) accept the same syntax in every standard: an object, a pointer, `nullptr`, `{}` and, since C++17, `std::nullopt`.

### Error Handling
Every standard has the same throwing API. Since C++23 (when `CPP_TS_HAS_EXPECTED` is set) every fallible operation also has a non-throwing variant returning `std::expected<T, ts::Error>`; the two variants share one implementation, so they report exactly the same failures with the same messages.

| Throwing (every standard) | Non-throwing (C++23) |
|:--|:--|
| `Parser(language)`, `Parser(language, store)` | `Parser::create(language)`, `Parser::create(language, store)` |
| `setLanguage(language)` (returns `bool`) | `trySetLanguage(language)` (reports why the language was rejected) |
| `parse`, `parseString`, `parseStringEncoded` | `tryParse`, `tryParseString`, `tryParseStringEncoded` |
| `Query(language, source)` | `Query::create(language, source)` (`Error::offset` = position of the error) |
| `QueryCursor::exec` | `QueryCursor::tryExec` |
| `LookaheadIterator(language, state)`, `LanguageView::getLookaheadIterator` | `LookaheadIterator::create`, `LanguageView::tryGetLookaheadIterator` |
| `Tree/Node/Range/Point::edit` | `tryEdit` |
| `WasmStore(engine)`, `loadLanguage`, `getLanguageCount` | `WasmStore::create`, `tryLoadLanguage`, `tryGetLanguageCount` |
| `enableDotGraphs(fd)`, `ts::fold<R>` | `tryEnableDotGraphs(fd)`, `ts::tryFold<R>` |

`ts::Error` holds an `ErrorKind` (e.g. `NoLanguage`, `OldTreeLanguage`, `QuerySyntax`, `WasmCompile`), a `message` and an `offset`. Each kind maps to one exception type of the throwing API (`std::logic_error` for usage errors, `std::invalid_argument`, `std::length_error`, `std::runtime_error`), see the documentation of `ts::ErrorKind`. Exceptions thrown by your own callbacks (read, decode, logger, progress, visitors) are always rethrown, also by the `try*` variants: they are not errors of the wrapper.

```cpp
auto parser = ts::Parser::create(tree_sitter_json());
if (!parser)
{
    std::cerr << ts::getErrorKindName(parser.error().kind) << ": " << parser.error().message << '\n';
    return;
}
auto tree = parser->tryParseString(source);
```

### Extended API
* **Queries:** `Query` (copyable) and `QueryCursor` with `nextMatch`/`nextCapture`, single-pass `matches()`/`captures()` ranges, `getCaptureIndexForName`, `QueryMatch::getNodeFor` and cancellable progress callbacks (`wasCancelled()`).
* **Wasm:** Added `WasmStore` and `WasmEngine` for WebAssembly environments.
* **Cross-Platform:** Fixed Windows-specific issues regarding file descriptors for Dot graph generation.

### Comparison Table
|      Feature     |              Original (v0.1.0)             |                   This Fork                   |
|:-----------------|:-------------------------------------------|:----------------------------------------------|
| **C++ Standard** |               C++23 only                   |      C++11 through C++23 (feature-tested)     |
|   **Traversal**  | Ranges, visitor (`onEnter`/`onLeave`), fold, typed visitors | Iterators/ranges, visitor (callbacks or `onEnter`/`onLeave`), preorder, fold |
|    **Queries**   |  Yes, with text predicate evaluation       |      Yes (full API, predicates as raw steps)  |
| **Wasm Support** |                    No                      |                     Yes                       |
|  **RAII Scope**  |                 Complete                   |                   Complete                    |
|  **Error model** |        `std::expected` everywhere          | Exceptions in every standard, plus `std::expected<T, ts::Error>` variants of every fallible operation in C++23 |
|  **Unit Tests**  |                   Yes                      | Yes, one suite run in every standard (Catch2, built-in framework in C++11) |
|  **Benchmarks**  |                    No                      |                     Yes                       |

## Benchmarks
Does the RAII wrapper actually cost you anything at runtime compared to using the raw tree-sitter C API? Short answer: **no**. Measured on 19 configurations (GCC, Clang, AppleClang and MSVC, C++11 to C++23), parsing, cursor traversal, query matching and incremental reparse run at the speed of the C API; only `ts::visit` and tree copies cost about one percent.

|           Category           | Wrapper overhead (median of all configurations) |
|:-----------------------------|:------------------------------------------------|
|           Parsing            |                 -0.1 % to +0.2 %                |
|   Tree traversal (cursor)    |                 -0.3 % to +0.3 %                |
| Tree traversal (`ts::visit`) |       +0.7 % to +0.8 % (up to ~4 % with MSVC)   |
|        Query matching        |           +0.4 % (about +2 % with GCC)          |
|     Incremental reparse      |                     -0.6 %                      |
|          Tree copy           |          +1.3 % (below 1 ns per copy)           |

Differences below ~2 % are within the noise of the GitHub-hosted runners. Full methodology, per-configuration results and instructions for reproducing them live in [`BENCHMARK.md`](docs/BENCHMARK.md).

To build and run the benchmark suite yourself:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCPP_TS_BUILD_BENCHMARK=ON   # add -DCPP_TS_FEATURE_WASM=ON for the Wasm section
cmake --build build --config Release
./build/benchmark/cpp-tree-sitter-benchmark            # add --no-wait to skip the final prompt
```

The `Benchmarks` GitHub Actions workflow runs the suite for every compiler, system and C++ standard and stores the results in [`docs/benchmarks`](docs/benchmarks/) (`<system>-<compiler>-cxx<standard>.txt`). The `Tests` workflow runs every test configuration.

## License
This project is licensed under the [**MIT License**](LICENSE).\
Copyright (c) 2023 [Nick Sumner](https://github.com/nsumner)\
Copyright (c) 2026 [Muppetsg2](https://github.com/Muppetsg2)
