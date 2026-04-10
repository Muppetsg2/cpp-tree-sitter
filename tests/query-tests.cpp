#include "pch.hpp"

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// Catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// STL
#include <cstdint>
#include <string>
#include <vector>

extern "C" const TSLanguage *tree_sitter_json();

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

TEST_CASE("Query Metadata and Information", "[query]")
{
    ts::Language lang         = tree_sitter_json();
    std::string  query_source = "((pair key: (string) @key) (#match? @key \"name\"))\n(number)+ @num";

    ts::Query query(lang, query_source);

    SECTION("Counts")
    {
        CHECK(query.getPatternCount() == 2);
        CHECK(query.getCaptureCount() == 2); // @key, @num
        CHECK(query.getStringCount() >= 1);  // "match?"
    }

    SECTION("Capture and String Resolution")
    {
        CHECK(query.getCaptureNameForID(0).compare("key") == 0);
        CHECK(query.getCaptureNameForID(1).compare("num") == 0);

        bool found_match_pred = false;
        for (uint32_t i = 0; i < query.getStringCount(); ++i)
        {
            if (query.getStringValueForID(i).compare("match?") == 0)
            {
                found_match_pred = true;
            }
        }
        CHECK(found_match_pred);
    }

    SECTION("Pattern Properties")
    {
        CHECK(query.isPatternRooted(0) == true);
        auto range = query.getByteRangeForPattern(0);
        CHECK(range.end > range.start);
    }

    SECTION("Quantifiers")
    {
        // JSON number
        CHECK(query.getCaptureQuantifierForID(1, 1) == ts::Quantifier::OneOrMore);
    }
}

TEST_CASE("Query Predicates", "[query]")
{
    ts::Language lang         = tree_sitter_json();
    std::string  query_source = "((pair key: (string) @k) (#eq? @k \"\\\"target\\\"\"))";
    ts::Query    query(lang, query_source);

    SECTION("Examine Predicate Steps")
    {
        auto steps = query.getAllPredicatesForPattern(0);
        REQUIRE(steps.size() >= 3); // "eq?", "@k", "\"target\"", "Done"

        CHECK(steps[0].type == ts::QueryPredicateStepType::String);

        // string "eq?"
        CHECK(query.getStringValueForID(steps[0].value_id).compare("eq?") == 0);

        // Capture (@k)
        CHECK(steps[1].type == ts::QueryPredicateStepType::Capture);

        // Last is always Done
        CHECK(steps.back().type == ts::QueryPredicateStepType::Done);
    }
}

TEST_CASE("Query Management", "[query]")
{
    ts::Language lang = tree_sitter_json();

    SECTION("Verify disableCapture only removes tags")
    {
        ts::Query query(lang, "((string) @s) ((number) @n)");
        query.disableCapture("n"); // Disable tag @n

        ts::Parser      parser(lang);
        ts::Tree        tree = parser.parseString("123");
        ts::QueryCursor cursor;
        cursor.exec(query, tree.getRootNode());

        ts::QueryMatch match;
        if (cursor.nextMatch(match))
        {
            CHECK(match.captures.empty());
        }
    }

    SECTION("Disable Patterns")
    {
        ts::Query query(lang, "((string) @s) ((number) @n)");
        query.disablePattern(0);
        query.disablePattern(1);

        ts::Parser      parser(lang);
        std::string     code = "{\"a\": 1}";
        ts::Tree        tree = parser.parseString(code);
        ts::QueryCursor cursor;
        cursor.exec(query, tree.getRootNode());

        ts::QueryMatch match;
        CHECK_FALSE(cursor.nextMatch(match));
    }
}

TEST_CASE("QueryCursor Limits and Ranges", "[query][cursor]")
{
    ts::Language    lang = tree_sitter_json();
    ts::Parser      parser(lang);
    std::string     code = "[1, 2, 3, 4, 5]";
    ts::Tree        tree = parser.parseString(code);
    ts::Query       query(lang, "(number) @n");
    ts::QueryCursor cursor;

    SECTION("Match Limit")
    {
        ts::Query query_complex(lang, "(_ (_)) @match");

        cursor.setMatchLimit(1);
        CHECK(cursor.getMatchLimit() == 1);

        std::string complex_code = "[[[[[[[[[[1]]]]]]]]]]";
        ts::Tree    complex_tree = parser.parseString(complex_code);

        cursor.exec(query_complex, complex_tree.getRootNode());

        ts::QueryMatch m;
        int            count = 0;
        while (cursor.nextMatch(m))
        {
            count++;
        }

        CHECK(cursor.didExceedMatchLimit() == true);
    }

    SECTION("Byte Range")
    {
        bool ok = cursor.setByteRange({ 3, 8 });

        REQUIRE(ok == true);

        cursor.exec(query, tree.getRootNode());

        ts::QueryMatch m;
        uint32_t       count = 0;
        while (cursor.nextMatch(m))
        {
            count++;
        }
        CHECK(count == 2);
    }
}

TEST_CASE("QueryCursor NextCapture", "[query][cursor]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  code = "{\"a\": 1}";
    ts::Tree     tree = parser.parseString(code);

    ts::Query       query(lang, "(pair key: (string) @k value: (number) @v)");
    ts::QueryCursor cursor;
    cursor.exec(query, tree.getRootNode());

    SECTION("Iterate via captures")
    {
        ts::QueryMatch match;
        uint32_t       capture_index;
        int            count = 0;

        while (cursor.nextCapture(match, capture_index))
        {
            ++count;
            CHECK(match.captures[capture_index].node.isNull() == false);
        }
        CHECK(count == 2); // One @k and one @v
    }
}

TEST_CASE("QueryCursor Progress Callback", "[query][cursor]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    // Large JSON to make cursor work
    std::string code = "[";
    for (int i = 0; i < 500; ++i)
    {
        code += "1,";
    }
    code += "1]";

    ts::Tree        tree = parser.parseString(code);
    ts::Query       query(lang, "(number) @n");
    ts::QueryCursor cursor;

    int                    progress_calls = 0;
    ts::QueryCursorOptions options;
    options.progress_callback = [&](ts::QueryCursorState *state)
    {
        ++progress_calls;
        CHECK(state->current_byte_offset <= code.size());
        return false; // Continue
    };

    SECTION("Execution with progress")
    {
#if TEST_HAS_CXX17
        cursor.exec(query, tree.getRootNode(), options);
#else
        cursor.exec(query, tree.getRootNode(), &options);
#endif
        ts::QueryMatch m;
        int            matches = 0;
        while (cursor.nextMatch(m))
        {
            ++matches;
        }

        CHECK(progress_calls > 0);
        CHECK(matches != progress_calls);
        CHECK(matches > progress_calls);
    }
}

TEST_CASE("QueryCursor Remove Match", "[query][cursor]")
{
    ts::Language    lang = tree_sitter_json();
    ts::Parser      parser(lang);
    ts::Tree        tree = parser.parseString("[1, 2]");
    ts::Query       query(lang, "(number) @n");
    ts::QueryCursor cursor;
    cursor.exec(query, tree.getRootNode());

    SECTION("Remove active match")
    {
        ts::QueryMatch m;
        REQUIRE(cursor.nextMatch(m));
        uint32_t first_id = m.id;

        cursor.removeMatch(first_id);

        CHECK(cursor.nextMatch(m));
    }
}
