#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

#if defined(_MSVC_LANG)
#define TEST_CXX_LEVEL _MSVC_LANG
#else
#define TEST_CXX_LEVEL __cplusplus
#endif

#define TEST_HAS_CXX17 (TEST_CXX_LEVEL >= 201703L)
#define TEST_HAS_CXX20 (TEST_CXX_LEVEL >= 202002L)

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

TEST_CASE("Parser and Tree lifecycle", "[parser]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    std::string code = R"({"key": [1, 2, null]})";

    SECTION("Parsing valid code")
    {
        ts::Tree tree = parser.parseString(code);
        ts::Node root = tree.getRootNode();

        CHECK_FALSE(root.isNull());
        CHECK(root.getType().compare("document") == 0);
        CHECK_FALSE(tree.hasError());
    }

    SECTION("Handling syntax errors")
    {
        ts::Tree tree = parser.parseString("{ \"key\": }"); // Error: No value
        CHECK(tree.hasError());
    }
}

TEST_CASE("Node navigation and attributes", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = R"({"id": 123})";
    ts::Tree     tree = parser.parseString(code);
    ts::Node     root = tree.getRootNode();

    // Search for object in document
    ts::Node obj = root.getNamedChild(0);

    SECTION("Source text access")
    {
        CHECK(obj.getType().compare("object") == 0);
        CHECK(obj.getSourceText(code) == code);
    }

    SECTION("Children iteration")
    {
        ts::Node pair = obj.getNamedChild(0);
        CHECK(pair.getType().compare("pair") == 0);
        CHECK(pair.getNamedChildCount() == 2); // "id" and 123
    }

    SECTION("Field names")
    {
        ts::Node pair     = obj.getNamedChild(0);
        ts::Node key_node = pair.getChildByFieldName("key");
        CHECK_FALSE(key_node.isNull());
        CHECK(key_node.getSourceText(code) == "\"id\"");
    }
}

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

TEST_CASE("STL-like Iterators", "[iterators]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code       = "[10, 20, 30]";
    ts::Tree     tree       = parser.parseString(code);
    ts::Node     array_node = tree.getRootNode().getNamedChild(0);

    SECTION("Iterating over named children")
    {
        std::vector<std::string> values;
        ts::Children             children{ array_node };

        for (ts::Node child : children)
        {
            if (child.isNamed())
            {
                values.push_back(child.getSourceText(code));
            }
        }

        REQUIRE(values.size() == 3);
        CHECK(values[0] == "10");
        CHECK(values[2] == "30");
    }
}

TEST_CASE("Query API", "[query]")
{
    ts::Language lang = tree_sitter_json();
    // Search for all numbers in file
    std::string query_source = "((number) @num)";

    CHECK_NOTHROW(ts::Query(lang, query_source));

    SECTION("Invalid query throws")
    {
        CHECK_THROWS_AS(ts::Query(lang, "((invalid_node)"), std::runtime_error);
    }
}

TEST_CASE("Tree Incremental Parsing (Editing)", "[parser][tree]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    std::string code = "[1, 2, 3]";
    ts::Tree    tree = parser.parseString(code);
    ts::Node    root = tree.getRootNode();

    SECTION("Edit and Re-parse")
    {
        std::string new_code = "[1, 20, 3]";

        ts::InputEdit edit{
            4,        // start_byte
            5,        // old_end_byte
            6,        // new_end_byte
            { 0, 4 }, // start_point
            { 0, 5 }, // old_end_point
            { 0, 6 }  // new_end_point
        };

        tree.edit(edit);
#if TEST_HAS_CXX17
        ts::Tree new_tree = parser.parseString(new_code, tree);
#else
        ts::Tree new_tree = parser.parseString(new_code, &tree);
#endif

        CHECK_FALSE(new_tree.hasError());
        ts::Node new_root   = new_tree.getRootNode();
        ts::Node array_node = new_root.getNamedChild(0);
        ts::Node second_num = array_node.getNamedChild(1);

        CHECK(second_num.getType().compare("number") == 0);
        CHECK(second_num.getSourceText(new_code) == "20");
    }
}

TEST_CASE("Node Search by Position", "[node][search]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = R"({
  "key": "value"
})";
    ts::Tree     tree = parser.parseString(code);
    ts::Node     root = tree.getRootNode();

    SECTION("Search by Byte Offset")
    {
        ts::Node value_node = root.getNamedDescendantForByteRange({ 11, 16 });
        CHECK(value_node.getType().compare("string") == 0);
        CHECK(value_node.getSourceText(code) == "\"value\"");
    }

    SECTION("Search by Point")
    {
        ts::Node value_node = root.getNamedDescendantForPointRange({ { 1, 9 }, { 1, 16 } });
        CHECK(value_node.getSourceText(code) == "\"value\"");
    }
}

TEST_CASE("Detailed Query Execution", "[query][cursor]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = R"({"name": "Gemini", "type": "AI"})";
    ts::Tree     tree = parser.parseString(code);

    std::string     query_source = "(pair key: (string) @key_name)";
    ts::Query       query(lang, query_source);
    ts::QueryCursor cursor;
    cursor.exec(query, tree.getRootNode());

    SECTION("Iterate Matches and Captures")
    {
        ts::QueryMatch           match;
        std::vector<std::string> keys;

        while (cursor.nextMatch(match))
        {
            for (const auto &capture : match.captures)
            {
                keys.push_back(capture.node.getSourceText(code));
            }
        }

        REQUIRE(keys.size() == 2);
        CHECK(keys[0] == "\"name\"");
        CHECK(keys[1] == "\"type\"");
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

TEST_CASE("Language Metadata and Versioning", "[language]")
{
    ts::Language lang = tree_sitter_json();

    SECTION("Basic Properties")
    {
        CHECK(lang.getSymbolsCount() > 0);
        CHECK(lang.getVersion() > 0);
        // CHECK(lang.getName().compare("json") == 0); This grammar don't have name
        CHECK(lang.getName().compare("") == 0);
    }

    SECTION("Symbol Resolution")
    {
        ts::Symbol sym = lang.getSymbolForName("object", true);
        CHECK(sym != 0);
        CHECK(lang.getSymbolName(sym).compare("object") == 0);
        CHECK(lang.getSymbolType(sym) == ts::SymbolType::TypeRegular);
    }
}
