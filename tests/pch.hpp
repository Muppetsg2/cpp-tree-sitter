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
#define TEST_CXX_LEVEL _MSVC_LANG
#else
#define TEST_CXX_LEVEL __cplusplus
#endif

#define TEST_HAS_CXX17 (TEST_CXX_LEVEL >= 201703L)
#define TEST_HAS_CXX20 (TEST_CXX_LEVEL >= 202002L)

#if defined(CPP_TS_TEST_FEATURE_WASM)
// wasmtime
#if TEST_HAS_CXX17
#include <wasmtime.hh>
#else
#include <wasm.h>
#endif
#endif
#endif
