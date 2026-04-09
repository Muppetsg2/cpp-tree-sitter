#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("Null Node Handling", "[node]")
{
    ts::Node null_node = ts::Node::null();

    SECTION("Basic null checks")
    {
        CHECK(null_node.isNull());
        CHECK(null_node.getType().compare("") == 0);
        CHECK(null_node.getSymbol() == 0);
        CHECK(null_node.getGrammarType().compare("") == 0);
        CHECK(null_node.getGrammarSymbol() == 0);
        CHECK(null_node.getChildCount() == 0);
    }

    SECTION("Identification")
    {
        CHECK(null_node.getID() == 0);
        CHECK(null_node.getLanguage().operator const TSLanguage *() == nullptr);
    }

    SECTION("Navigation from null node")
    {
        CHECK(null_node.getParent().isNull());
        CHECK(null_node.getNextSibling().isNull());
    }

    SECTION("Advanced Navigation")
    {
        CHECK(null_node.getParent().getParent().isNull());
        CHECK(null_node.getNextNamedSibling().isNamed() == false);
        CHECK(null_node.getChildCount() == 0);
        CHECK_THROWS_AS(null_node.getChild(0), std::out_of_range);
    }

    SECTION("Source text on null")
    {
        CHECK(null_node.getSourceRange("any source").empty());
        CHECK(null_node.getSourceText("any source").empty());
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

TEST_CASE("Node identification and language", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code     = "42";
    ts::Tree     tree     = parser.parseString(code);
    ts::Node     root     = tree.getRootNode();
    ts::Node     num_node = root.getNamedChild(0);

    SECTION("ID and Language")
    {
        CHECK(num_node.getID() != 0);
        CHECK(num_node.getLanguage().operator const TSLanguage *() == lang.operator const TSLanguage *());
    }

    SECTION("Grammar types")
    {
        // getType() zwraca alias/nazwê, getGrammarType() zwraca nazwê z gramatyki
        CHECK(num_node.getType().compare("number") == 0);
        CHECK(num_node.getGrammarType().compare("number") == 0);
        CHECK(num_node.getGrammarSymbol() == num_node.getSymbol());
    }
}

TEST_CASE("Node flags and states", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    SECTION("Normal node flags")
    {
        std::string code  = "[1, null]";
        ts::Tree    tree  = parser.parseString(code);
        ts::Node    root  = tree.getRootNode();
        ts::Node    array = root.getNamedChild(0);

        CHECK(array.isNamed());
        CHECK_FALSE(array.isMissing());
        CHECK_FALSE(array.isExtra());
        CHECK_FALSE(array.isError());
        CHECK_FALSE(array.hasError());
        CHECK_FALSE(array.hasChanges());
    }

    SECTION("Missing and Error nodes")
    {
        // Niepe³ny JSON: [1,
        std::string code = "[1, ";
        ts::Tree    tree = parser.parseString(code);
        ts::Node    root = tree.getRootNode();

        CHECK(root.hasError());

        // Szukamy brakuj¹cego wêz³a (missing) lub b³êdu
        ts::visit(root,
                  [](ts::Node n)
                  {
                      if (n.isMissing())
                      {
                          SUCCEED("Found missing node");
                      }
                  });
    }
}

TEST_CASE("Node ranges and source", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code     = "  \"hello\"";
    ts::Tree     tree     = parser.parseString(code);
    ts::Node     root     = tree.getRootNode();
    ts::Node     str_node = root.getNamedChild(0);

    SECTION("Byte and Point ranges")
    {
        auto bytes  = str_node.getByteRange();
        auto points = str_node.getPointRange();

        CHECK(bytes.start == 2);
        CHECK(bytes.end == 9);
        CHECK(points.start.column == 2);
        CHECK(points.end.column == 9);
    }

    SECTION("Source Range Validation")
    {
        // Testowanie zabezpieczenia przed wyjœciem poza zakres
        std::string short_source = "  ";
        CHECK(str_node.getSourceRange(short_source).empty());
    }
}

TEST_CASE("Node navigation (extended)", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code  = "[1, 2, 3]";
    ts::Tree     tree  = parser.parseString(code);
    ts::Node     array = tree.getRootNode().getNamedChild(0);

    SECTION("Siblings and children")
    {
        ts::Node first  = array.getChild(1); // To jest '1' (0 to '[')
        ts::Node second = array.getChild(3); // To jest '2' (2 to ',')

        CHECK(first.getNextSibling() == array.getChild(2)); // przecinek
        CHECK(second.getPreviousSibling() == array.getChild(2));

        CHECK(array.getFirstChild().getType().compare("[") == 0);
        CHECK(array.getLastChild().getType().compare("]") == 0);
    }

    SECTION("Named children navigation")
    {
        ts::Node first_named  = array.getNamedChild(0); // 1
        ts::Node second_named = array.getNamedChild(1); // 2

        CHECK(first_named.getNextNamedSibling() == second_named);
        CHECK(second_named.getPreviousNamedSibling() == first_named);

        CHECK(array.getFirstNamedChild().getSourceText(code) == "1");
        CHECK(array.getLastNamedChild().getSourceText(code) == "3");
    }

    SECTION("Child with descendant")
    {
        ts::Node root   = tree.getRootNode();
        ts::Node target = array.getNamedChild(2); // '3'

        ts::Node found = root.getChildWithDescendant(target);
        CHECK(found == array);
    }
}

TEST_CASE("Node Search and Fields (extended)", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = "{\"item\": 42}";
    ts::Tree     tree = parser.parseString(code);
    ts::Node     pair = tree.getRootNode().getNamedChild(0).getNamedChild(0);

    SECTION("Search for children")
    {
        // Szukamy wêz³a na konkretnym bajcie
        ts::Node found = pair.getFirstChildForByte(9);
        CHECK(found.getType().compare("number") == 0);

        ts::Node found_named = pair.getFirstNamedChildForByte(2);
        CHECK(found_named.getType().compare("string") == 0);
    }

    SECTION("Field access")
    {
        CHECK(pair.getFieldNameForChild(0).compare("key") == 0);
        // "value" jest drugim nazwanym dzieckiem pary (po "key")
        CHECK(pair.getFieldNameForNamedChild(1).compare("value") == 0);

        // Pobieranie po ID (bardziej wydajne ni¿ string)
        uint16_t field_id = lang.getFieldIDForName("value");
        CHECK_FALSE(pair.getChildByFieldID(field_id).isNull());
    }
}

TEST_CASE("Node Parsing State", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    ts::Tree     tree = parser.parseString("{\"a\": 1}");
    ts::Node     root = tree.getRootNode();

    ts::Node obj  = root.getNamedChild(0);
    ts::Node pair = obj.getNamedChild(0);

    SECTION("State access for internal nodes")
    {
        CHECK(obj.getParseState() != 0);

        ts::Node key = pair.getNamedChild(0);
        CHECK(key.getNextParseState() != 0);
    }
}

TEST_CASE("Node Mutation", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = "[1]";
    ts::Tree     tree = parser.parseString(code);
    ts::Node     node = tree.getRootNode();

    SECTION("Edit node")
    {
        // Przesuwamy wêze³ w prawo o 1 bajt
        ts::InputEdit edit{ 0, 0, 1, { 0, 0 }, { 0, 0 }, { 0, 1 } };

        uint32_t old_start = node.getByteRange().start;
        node.edit(edit);
        CHECK(node.getByteRange().start == old_start + 1);
    }
}

TEST_CASE("Node Comparison", "[node]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    ts::Tree     tree  = parser.parseString("[1, 1]");
    ts::Node     root  = tree.getRootNode();
    ts::Node     array = root.getNamedChild(0);

    SECTION("Equality")
    {
        ts::Node first  = array.getNamedChild(0);
        ts::Node second = array.getNamedChild(1);

        CHECK(first == first);
        CHECK(first != second);
        CHECK_FALSE(first == second);
    }
}

TEST_CASE("Node S-Expression", "[node]")
{
    ts::Language lang  = tree_sitter_json();
    ts::Tree     tree  = ts::Parser(lang).parseString("[1]");
    auto         sexpr = tree.getRootNode().getSExpr();

    REQUIRE(sexpr != nullptr);
    CHECK(std::string(sexpr.get()).find("array") != std::string::npos);
}
