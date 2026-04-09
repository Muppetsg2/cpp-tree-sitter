#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("TreeCursor navigation", "[cursor]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = "[1, 2]";
    ts::Tree     tree = parser.parseString(code);

    ts::TreeCursor cursor = tree.getRootNode().getCursor();

    SECTION("Walking the tree")
    {
        CHECK(cursor.getCurrentNode().getType().compare("document") == 0);

        REQUIRE(cursor.gotoFirstChild());
        CHECK(cursor.getCurrentNode().getType().compare("array") == 0);

        REQUIRE(cursor.gotoFirstChild()); // '['
        CHECK(cursor.getCurrentNode().getType().compare("[") == 0);

        REQUIRE(cursor.gotoNextSibling()); // '1'
        CHECK(cursor.getCurrentNode().getType().compare("number") == 0);

        REQUIRE(cursor.gotoParent());
        CHECK(cursor.getCurrentNode().getType().compare("array") == 0);
    }
}

TEST_CASE("TreeCursor Advanced Movements", "[cursor]")
{
    ts::Language   lang = tree_sitter_json();
    ts::Parser     parser(lang);
    std::string    code   = "[1, [2, 3], 4]";
    ts::Tree       tree   = parser.parseString(code);
    ts::TreeCursor cursor = tree.getRootNode().getCursor();

    SECTION("Deep Navigation")
    {
        REQUIRE(cursor.gotoFirstChild());  // array
        REQUIRE(cursor.gotoFirstChild());  // [
        REQUIRE(cursor.gotoNextSibling()); // 1
        REQUIRE(cursor.gotoNextSibling()); // ,
        REQUIRE(cursor.gotoNextSibling()); // inner array

        CHECK(cursor.getCurrentNode().getType().compare("array") == 0);

        REQUIRE(cursor.gotoFirstChild());  // inner [
        REQUIRE(cursor.gotoNextSibling()); // 2

        CHECK(cursor.getCurrentNode().getSourceText(code) == "2");
        CHECK(cursor.getDepthFromOrigin() == 3); // root -> doc -> array -> array -> 2
    }
}
