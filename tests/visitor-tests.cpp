#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("Visitor Utility", "[visitor]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = "[1, 2]";
    ts::Tree     tree = parser.parseString(code);

    SECTION("Visit all nodes")
    {
        int total_nodes = 0;
        ts::visit(tree.getRootNode(), [&](ts::Node n) { ++total_nodes; });

        // document, array, [, 1, ,, 2, ] = 7 nodes
        CHECK(total_nodes == 7);
    }
}
