# cpp-tree-sitter

`cpp-tree-sitter` is a modern, lightweight **C++11 (and newer)** wrapper around the [tree-sitter](https://github.com/tree-sitter/tree-sitter/) parsing library. 
It provides a clean, RAII-compliant interface and CMake integrations to simplify working with syntax trees in C++ projects.
> Note on this Fork: This project is a significant evolution of the original repository by Nick Sumner. It has been rewritten to support older C++ standards (back to C++11), full Query/Wasm support, and advanced tree traversal utilities.

## Key Features
* **Modern RAII API:** Automatic memory management for Parsers, Trees, Nodes, and Cursors using standard smart pointers.
* **Broad Compatibility:** Supports **C++11, C++14, C++17, and C++20**. It automatically leverages modern features like `std::string_view` and `std::concepts` if available, while providing fallbacks for older standards.
* **STL-style Iterators:** Use standard `for` loops to iterate over child nodes.
* **Tree Visitor:** A built-in Depth-First Search (DFS) visitor (`ts::visit`) for easy tree traversal.
* **Wasm Support:** High-level wrappers for loading and managing WebAssembly-based grammars.
* **Query Engine:** Full support for tree-sitter queries (patterns and captures).
* **CMake Integration:** Easy dependency management using [CPM.cmake](https://github.com/cpm-cmake/cpm.cmake).

## Requirements
* **Compiler:** C++11 compatible or newer (C++17/20 recommended).
* **Build System:** [CMake](https://cmake.org/) 3.30 or newer.

## Using in a CMake Project
The easiest way to include `cpp-tree-sitter` is via CPM. Adding this wrapper automatically makes the core `tree-sitter` library available as well.

```cmake
cmake_minimum_required(VERSION 3.30)

project(MyParser)

set(CMAKE_CXX_STANDARD 17) # Works with 11, 14, 17, 20

include(cmake/CPM.cmake)

# Download the wrapper and tree-sitter core
CPMAddPackage(
    NAME cpp-tree-sitter
    GIT_REPOSITORY https://github.com/Muppetsg2/cpp-tree-sitter
    GIT_TAG main
)

# Download a grammar (e.g., JSON) and make it a CMake target
cpp_ts_add_grammar(
    NAME tree-sitter-json
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json.git
    VERSION 0.24.8
)

# or if you have grammar downloaded you can specify location using:
# cpp_ts_add_grammar(
#     NAME tree-sitter-json
#     SOURCE_DIR path/to/dir/tree-sitter-json
# )
#
# or set CPP_TS_GRAMMAR_PATH before downloading cpp-tree-sitter

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE cpp-tree-sitter tree-sitter-json)
```

## Quick Start Example

This example demonstrates parsing a JSON string and using the visitor to inspect nodes.

```cpp
#include <iostream>
#include <cpp-tree-sitter.h>

// Extern declaration for the grammar function
extern "C" TSLanguage* tree_sitter_json();

int main() {
    // Initialize language and parser
    ts::Language language = tree_sitter_json();
    ts::Parser parser{language};

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
    });

    // Or use STL-style iteration
    for (auto child : ts::Children{root}) {
        std::cout << "Child type: " << child.getType() << "\n";
    }

    return 0; // Resources are cleaned up automatically via RAII
}
```

## Major Improvements in this Fork

### Memory & Safety
While the original project provided basic wrappers, this fork implements a full-scale **RAII** architecture. It includes specialized `FreeHelper` functors to ensure that internal C-allocated strings (like S-Expressions) are freed correctly. It also introduces `shared_ptr` for `Language` objects to prevent use-after-free errors when multiple parsers share a grammar.

### Compatibility Layer
The library now features a custom `StringView` for C++11/14 environments and detects standard versions to enable `std::optional` or C++20 `concepts` dynamically.

### Extended API
* **Queries:** Added `Query` and `QueryCursor` classes to perform pattern matching.
* **Wasm:** Added `WasmStore` and `WasmEngine` for WebAssembly environments.
* **Cross-Platform:** Fixed Windows-specific issues regarding file descriptors for Dot graph generation.

### Comparison Table
|      Feature     |   Original Fork  |               This Fork               |
|------------------|------------------|---------------------------------------|
| **C++ Standard** |    C++17 only    |          C++11 through C++20          |
|   **Traversal**  | Manual/Iterators |          Iterators + Visitor          |
|    **Queries**   |        No        |             Yes (Full API)            |
| **Wasm Support** |        No        |                  Yes                  |
|  **RAII Scope**  |      Partial     |  Complete (Tree, Parser, Query, etc.) |
|  **Unit Tests**  |        No        |                  Yes                  |


## License
This project is licensed under the [**MIT License**](LICENSE).\
Copyright (c) 2023 Nick Sumner\
Copyright (c) 2026 Muppetsg2
