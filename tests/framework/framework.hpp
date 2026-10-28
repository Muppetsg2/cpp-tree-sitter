#pragma once
#ifndef CPP_TREE_SITTER_TESTS_FRAMEWORK_HPP
#define CPP_TREE_SITTER_TESTS_FRAMEWORK_HPP

// Selects the test framework. Every test file is written once against the subset of Catch2 implemented by minitest.hpp:
//
//   TEST_CASE, SECTION, CHECK, CHECK_FALSE, REQUIRE, REQUIRE_FALSE, CHECK_THROWS, REQUIRE_THROWS, CHECK_THROWS_AS,
//   REQUIRE_THROWS_AS, CHECK_THROWS_WITH, REQUIRE_THROWS_WITH, CHECK_NOTHROW, REQUIRE_NOTHROW, FAIL, SUCCEED
//
// CPP_TS_TEST_USE_CATCH2 is defined by CMake when Catch2 is available (C++14 and newer); the C++11 configuration uses
// the built-in framework. Printers of library types are specializations of TS_TEST_STRING_MAKER_NAMESPACE::StringMaker
// (`Catch` or `ts_test`), see common.hpp.

#if defined(CPP_TS_TEST_USE_CATCH2)
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <catch2/matchers/catch_matchers_string.hpp> // CHECK_THROWS_WITH
#define TS_TEST_STRING_MAKER_NAMESPACE Catch
#else
#include "minitest.hpp"
#define TS_TEST_STRING_MAKER_NAMESPACE ts_test
#endif // defined(CPP_TS_TEST_USE_CATCH2)

#endif // CPP_TREE_SITTER_TESTS_FRAMEWORK_HPP
