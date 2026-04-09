#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("Lookahead Iterator", "[language][lookahead]")
{
    ts::Language lang = tree_sitter_json();
    ts::Tree     tree = ts::Parser(lang).parseString("{\"a\": 1}");
    ts::Node     root = tree.getRootNode();

    ts::StateID state = root.getNamedChild(0).getNamedChild(0).getNextParseState();

    ts::LookaheadIterator it(lang, state);
    bool                  found_value_symbols = false;

    while (it.next())
    {
        if (it.getCurrentSymbolName().size() > 0)
        {
            found_value_symbols = true;
        }
    }
    CHECK(found_value_symbols);
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
