#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("Basic data structures work correctly", "[types]")
{
    SECTION("Point comparisons")
    {
        ts::Point p1({ 1, 5 });
        ts::Point p2({ 1, 10 });
        ts::Point p3({ 2, 0 });

        CHECK(p1 < p2);
        CHECK(p2 < p3);
        CHECK(p1 != p2);
        CHECK(p1 == ts::Point({ 1, 5 }));
    }

    SECTION("Extent (Range) representation")
    {
        ts::Extent<uint32_t> range{ 10, 20 };
        CHECK(range.start == 10);
        CHECK(range.end == 20);
    }
}
