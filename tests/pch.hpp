#pragma once
#ifndef CPP_TREE_SITTER_TESTS_PCH_HPP
#define CPP_TREE_SITTER_TESTS_PCH_HPP

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// Catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// STL
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(_MSVC_LANG)
#define CPP_TS_TEST_CXX_LEVEL _MSVC_LANG
#else
#define CPP_TS_TEST_CXX_LEVEL __cplusplus
#endif // defined(_MSVC_LANG)

#define CPP_TS_TEST_HAS_CXX17 (CPP_TS_TEST_CXX_LEVEL >= 201703L)
#define CPP_TS_TEST_HAS_CXX20 (CPP_TS_TEST_CXX_LEVEL >= 202002L)
#define CPP_TS_TEST_HAS_CXX23 (CPP_TS_TEST_CXX_LEVEL >= 202302L)

#if CPP_TS_TEST_HAS_CXX17
#include <string_view>
#endif // CPP_TS_TEST_HAS_CXX17

#if CPP_TS_TEST_HAS_CXX20
#include <span>
#endif // CPP_TS_TEST_HAS_CXX20

#if CPP_TS_TEST_HAS_CXX23
#include <expected>
#endif // CPP_TS_TEST_HAS_CXX23

#if defined(CPP_TS_TEST_FEATURE_WASM)
#define CPP_TS_TEST_WASMTIME_CXX_ENABLED CPP_TS_TEST_HAS_CXX17 && (CPP_TS_TEST_WASMTIME_VERSION >= 390000)
// wasmtime
#if CPP_TS_TEST_WASMTIME_CXX_ENABLED
#include <wasmtime.hh>
#else
#include <wasm.h>
#endif // CPP_TS_TEST_WASMTIME_CXX_ENABLED
#endif // defined(CPP_TS_TEST_FEATURE_WASM)

#endif // CPP_TREE_SITTER_TESTS_PCH_HPP
