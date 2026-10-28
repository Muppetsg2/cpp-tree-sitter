#pragma once
#ifndef CPP_TREE_SITTER_BENCHMARK_PCH_HPP
#define CPP_TREE_SITTER_BENCHMARK_PCH_HPP

// tree-sitter
#include <tree_sitter/api.h>

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// STL
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#define CPP_TS_BENCHMARK_HAS_CXX17 CPP_TS_HAS_CXX17
#define CPP_TS_BENCHMARK_HAS_CXX20 CPP_TS_HAS_RANGES   // std::ranges algorithms
#define CPP_TS_BENCHMARK_HAS_CXX23 CPP_TS_HAS_EXPECTED // std::expected based API

#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
#define CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED \
    CPP_TS_BENCHMARK_HAS_CXX17 && (CPP_TS_BENCHMARK_WASMTIME_VERSION >= 390000)
// wasmtime
#if CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
#include <wasmtime.hh>
#else
#include <wasm.h>
#endif // CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
#include <fstream>
#include <ios>
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

#if defined(_WIN32)
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif // defined(_WIN32)

#endif // CPP_TREE_SITTER_BENCHMARK_PCH_HPP
