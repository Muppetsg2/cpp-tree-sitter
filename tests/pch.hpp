#pragma once
#ifndef CPP_TREE_SITTER_TESTS_PCH_HPP
#define CPP_TREE_SITTER_TESTS_PCH_HPP

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#if defined(_MSVC_LANG)
#define TEST_CXX_LEVEL _MSVC_LANG
#else
#define TEST_CXX_LEVEL __cplusplus
#endif

#define TEST_HAS_CXX17 (TEST_CXX_LEVEL >= 201703L)
#define TEST_HAS_CXX20 (TEST_CXX_LEVEL >= 202002L)
#endif
