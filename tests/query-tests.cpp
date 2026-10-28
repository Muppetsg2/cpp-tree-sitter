// Query (compilation, metadata, predicates, capture lookup, copy) and QueryCursor (execution, result ranges, limits,
// ranges, progress callbacks).

#include "common.hpp"

// STL
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if CPP_TS_HAS_RANGES
#include <ranges>
#endif // CPP_TS_HAS_RANGES

namespace
{
    std::uint32_t countMatches(ts::QueryCursor &cursor)
    {
        std::uint32_t  count = 0;
        ts::QueryMatch match;
        while (cursor.nextMatch(match))
        {
            ++count;
        }
        return count;
    }
} // namespace

TEST_CASE("Query compilation errors", "[query][exceptions]")
{
    const ts::Language language(tree_sitter_json());

    CHECK_NOTHROW(ts::Query(language, "(number) @n"));
    CHECK_THROWS_AS(ts::Query(language, "(number"), std::runtime_error);          // syntax
    CHECK_THROWS_AS(ts::Query(language, "((invalid_node)"), std::runtime_error);  // syntax
    CHECK_THROWS_AS(ts::Query(language, "(no_such_node)"), std::runtime_error);   // node type
    CHECK_THROWS_AS(ts::Query(language, "(pair nope: (_))"), std::runtime_error); // field
    CHECK_THROWS_AS(ts::Query(ts::LanguageView(), "(number)"), std::runtime_error);

    // The message names the kind of the error.
    CHECK(ts_test::contains(ts_test::thrownMessage([&language] { ts::Query query(language, "(no_such_node)"); }),
                            "Node Type"));
}

#if SIZE_MAX > UINT32_MAX
TEST_CASE("Query rejects sources and names longer than 4GB", "[query][exceptions]")
{
    const ts::Language language(tree_sitter_json());

    // A view with an artificial size, nothing of that size is allocated.
    const char           *fake_str      = "fake";
    constexpr std::size_t overflow_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1;
    const ts::StringView  huge_view(fake_str, overflow_size);

    CHECK_THROWS_AS(ts::Query(language, huge_view), std::length_error);

    ts::Query query(language, "(number) @n");
    CHECK_THROWS_AS(query.disableCapture(huge_view), std::length_error);
}
#endif // SIZE_MAX > UINT32_MAX

TEST_CASE("Query metadata", "[query]")
{
    const ts::Language language(tree_sitter_json());
    const ts::Query    query(language, "(number) @n\n(array (string)+ @s)");

    CHECK(query.isValid());
    CHECK(query.getPatternCount() == 2u);
    CHECK(query.getCaptureCount() == 2u);
    CHECK(query.getCaptureNameForID(0) == "n");
    CHECK(query.getCaptureNameForID(1) == "s");
    CHECK(query.getCaptureQuantifierForID(0, 0) == ts::Quantifier::One);
    CHECK(query.getCaptureQuantifierForID(1, 1) == ts::Quantifier::OneOrMore);
    CHECK(query.getCaptureQuantifierForID(1, 0) == ts::Quantifier::Zero);
    CHECK(query.isPatternRooted(0));
    CHECK_FALSE(query.isPatternNonLocal(0));
    static_cast<void>(query.isPatternGuaranteedAtStep(0));

    // The end of a pattern includes the whitespace following it, up to the next pattern.
    CHECK(query.getByteRangeForPattern(0) == ts::Extent<std::uint32_t>(0, 12));
    CHECK(query.getByteRangeForPattern(1).start == 12u);
}

TEST_CASE("Query capture lookup by name", "[query]")
{
    ts_test::JsonFixture json("{\"a\": 1}");
    const ts::Query      query(json.language, "(pair key: (string) @key value: (_) @value)");

    const std::uint32_t key   = query.getCaptureIndexForName("key");
    const std::uint32_t value = query.getCaptureIndexForName("value");
    CHECK(key == 0u);
    CHECK(value == 1u);
    CHECK(query.getCaptureIndexForName("missing") == std::numeric_limits<std::uint32_t>::max());

    ts::QueryCursor cursor;
    cursor.exec(query, json.root());
    ts::QueryMatch match;
    REQUIRE(cursor.nextMatch(match));
    CHECK(json.text(match.getNodeFor(key)) == "\"a\"");
    CHECK(json.text(match.getNodeFor(value)) == "1");
    CHECK(match.getNodeFor(42).isNull());
}

TEST_CASE("Query predicates", "[query][predicates]")
{
    const ts::Language language(tree_sitter_json());

    SECTION("Steps of a predicate")
    {
        const ts::Query query(language, "((string) @s (#eq? @s \"\\\"a\\\"\"))");

        // Predicates are exposed as raw steps: "eq?" @s "\"a\"" <done>.
        const std::vector<ts::QueryPredicateStep> steps = query.getAllPredicatesForPattern(0);
        REQUIRE(steps.size() == 4u);
        CHECK(steps[0].type == ts::QueryPredicateStepType::String);
        CHECK(query.getStringValueForID(steps[0].value_id) == "eq?");
        CHECK(steps[1].type == ts::QueryPredicateStepType::Capture);
        CHECK(query.getCaptureNameForID(steps[1].value_id) == "s");
        CHECK(steps[2].type == ts::QueryPredicateStepType::String);
        CHECK(query.getStringValueForID(steps[2].value_id) == "\"a\"");
        CHECK(steps[3].type == ts::QueryPredicateStepType::Done);
        CHECK(query.getStringCount() == 2u);
    }

    SECTION("String table")
    {
        const ts::Query query(language, "((pair key: (string) @key) (#match? @key \"name\"))\n(number)+ @num");
        CHECK(query.getPatternCount() == 2u);
        CHECK(query.getCaptureCount() == 2u);

        bool found_match = false;
        for (std::uint32_t i = 0; i < query.getStringCount(); ++i)
        {
            found_match = found_match || query.getStringValueForID(i) == "match?";
        }
        CHECK(found_match);
    }

    SECTION("Patterns without predicates")
    {
        const ts::Query plain(language, "(number) @n");
        CHECK(plain.getAllPredicatesForPattern(0).empty());
    }
}

TEST_CASE("Query disabling captures and patterns", "[query]")
{
    ts_test::JsonFixture json("{\"a\": 1}");
    ts::QueryCursor      cursor;

    SECTION("Disabled captures and patterns")
    {
        ts::Query query(json.language, "(pair key: (_) @key value: (_) @value)\n(number) @n");
        query.disableCapture("value");
        query.disablePattern(1);

        cursor.exec(query, json.root());
        ts::QueryMatch match;
        REQUIRE(cursor.nextMatch(match));
        CHECK(match.pattern_index == 0u);
        REQUIRE(match.captures.size() == 1u);
        CHECK(json.text(match.captures[0].node) == "\"a\"");
        CHECK_FALSE(cursor.nextMatch(match));
    }

    SECTION("A disabled capture still matches, without captures")
    {
        ts::Query query(json.language, "(number) @n");
        query.disableCapture("n");

        cursor.exec(query, json.root());
        ts::QueryMatch match;
        REQUIRE(cursor.nextMatch(match));
        CHECK(match.captures.empty());
    }

    SECTION("All patterns disabled")
    {
        ts::Query query(json.language, "((string) @s) ((number) @n)");
        query.disablePattern(0);
        query.disablePattern(1);

        cursor.exec(query, json.root());
        CHECK(countMatches(cursor) == 0u);
    }
}

TEST_CASE("Query copy and move", "[query][lifecycle]")
{
    ts_test::JsonFixture json("{\"a\": 1, \"b\": [true, false]}");
    ts::QueryCursor      cursor;
    const ts::Query      query(json.language, "(pair key: (string) @key value: (_) @value)");

    // The copy (ts_query_copy) is independent: disabling a capture does not affect the original.
    ts::Query copy(query);
    copy.disableCapture("value");

    std::uint32_t original_captures = 0;
    cursor.exec(query, json.root());
    for (const ts::QueryMatch &match : cursor.matches())
    {
        original_captures += static_cast<std::uint32_t>(match.captures.size());
    }

    std::uint32_t copied_captures = 0;
    cursor.exec(copy, json.root());
    for (const ts::QueryMatch &match : cursor.matches())
    {
        copied_captures += static_cast<std::uint32_t>(match.captures.size());
    }

    CHECK(original_captures == 4u);
    CHECK(copied_captures == 2u);

    ts::Query assigned(json.language, "(number) @n");
    assigned = query;
    CHECK(assigned.getCaptureCount() == 2u);

    ts::Query moved(std::move(copy));
    CHECK(moved.isValid());
    CHECK_FALSE(copy.isValid());
    CHECK_THROWS_AS(cursor.exec(copy, json.root()), std::invalid_argument);
}

TEST_CASE("QueryCursor nextMatch and nextCapture", "[query][cursor]")
{
    ts_test::JsonFixture json("{\"a\": 1, \"b\": 2}");
    const ts::Query      query(json.language, "(pair key: (string) @key value: (_) @value)");
    ts::QueryCursor      cursor;

    SECTION("Matches")
    {
        cursor.exec(query, json.root());
        ts::QueryMatch match;
        REQUIRE(cursor.nextMatch(match));
        CHECK(match.pattern_index == 0u);
        REQUIRE(match.captures.size() == 2u);
        CHECK(json.text(match.getNodeFor(0)) == "\"a\"");
        CHECK(json.text(match.getNodeFor(1)) == "1");
        REQUIRE(cursor.nextMatch(match));
        CHECK(json.text(match.getNodeFor(0)) == "\"b\"");
        CHECK_FALSE(cursor.nextMatch(match));
    }

    SECTION("Captures come in source order")
    {
        cursor.exec(query, json.root());
        ts::QueryMatch           match;
        std::vector<std::string> captured;
        std::uint32_t            index = 0;
        while (cursor.nextCapture(match, index))
        {
            CHECK_FALSE(match.captures[index].node.isNull());
            captured.push_back(json.text(match.captures[index].node));
        }
        REQUIRE(captured.size() == 4u);
        CHECK(captured[0] == "\"a\"");
        CHECK(captured[1] == "1");
        CHECK(captured[3] == "2");
    }
}

TEST_CASE("QueryCursor anchors and multiple patterns", "[query][cursor]")
{
    ts_test::JsonFixture json("[1, 2, 3]");
    ts::QueryCursor      cursor;

    SECTION("Anchored siblings")
    {
        // The comma must immediately follow the number (anchor `.`); the last number has no comma.
        const ts::Query query(json.language, "(array (number) @num . \",\" @comma)");
        cursor.exec(query, json.root());

        ts::QueryMatch match;
        REQUIRE(cursor.nextMatch(match));
        REQUIRE(match.captures.size() == 2u);
        CHECK(json.text(match.captures[0].node) == "1");
        CHECK(match.captures[1].node.getType() == ",");

        REQUIRE(cursor.nextMatch(match));
        CHECK(json.text(match.captures[0].node) == "2");
        CHECK(match.captures[1].node.getType() == ",");

        CHECK_FALSE(cursor.nextMatch(match));
    }

    SECTION("Disjoint patterns")
    {
        // 1 match for the array + 3 for the numbers.
        const ts::Query query(json.language, "(array) @arr \n (number) @num");
        cursor.exec(query, json.root());
        CHECK(countMatches(cursor) == 4u);
    }
}

TEST_CASE("QueryCursor matches() and captures() ranges", "[query][cursor]")
{
    ts_test::JsonFixture json("{\"a\": 1, \"b\": [true, false]}");
    const ts::Query      query(json.language, "(pair key: (string) @key value: (_) @value)");
    ts::QueryCursor      cursor;

    const std::uint32_t key   = query.getCaptureIndexForName("key");
    const std::uint32_t value = query.getCaptureIndexForName("value");

    SECTION("matches()")
    {
        cursor.exec(query, json.root());
        std::vector<std::string> keys;
        for (const ts::QueryMatch &match : cursor.matches())
        {
            keys.push_back(json.text(match.getNodeFor(key)));
            CHECK(match.getNodeFor(value).isNamed());
        }
        REQUIRE(keys.size() == 2u);
        CHECK(keys[0] == "\"a\"");
        CHECK(keys[1] == "\"b\"");

        // A finished execution produces an empty range.
        std::uint32_t remaining = 0;
        for (const ts::QueryMatch &match : cursor.matches())
        {
            static_cast<void>(match);
            ++remaining;
        }
        CHECK(remaining == 0u);
    }

    SECTION("captures()")
    {
        cursor.exec(query, json.root());
        std::vector<std::string> texts;
        for (const ts::QueryCaptureEntry &entry : cursor.captures())
        {
            texts.push_back(json.text(entry.capture().node));
        }
        REQUIRE(texts.size() == 4u);
        CHECK(texts[0] == "\"a\"");
        CHECK(texts[1] == "1");
        CHECK(texts[2] == "\"b\"");
        CHECK(texts[3] == "[true, false]");
    }

#if CPP_TS_HAS_RANGES
    SECTION("matches() composes with std::views")
    {
        cursor.exec(query, json.root());
        std::uint32_t arrays = 0;
        for (const ts::QueryMatch &match :
             cursor.matches()
                     | std::views::filter([&](const ts::QueryMatch &m)
                                          { return m.getNodeFor(value).getType() == "array"; }))
        {
            static_cast<void>(match);
            ++arrays;
        }
        CHECK(arrays == 1u);
    }
#endif // CPP_TS_HAS_RANGES
}

TEST_CASE("QueryMatch reuses its storage", "[query][cursor]")
{
    ts::QueryMatch empty;
    CHECK(empty.id == 0u);
    CHECK(empty.captures.empty());

    ts_test::JsonFixture json("[1, 2]");
    const ts::Query      query(json.language, "(number) @n");
    ts::QueryCursor      cursor;
    cursor.exec(query, json.root());

    ts::QueryMatch match;
    REQUIRE(cursor.nextMatch(match));
    const std::size_t capacity = match.captures.capacity();
    REQUIRE(cursor.nextMatch(match));
    CHECK(match.captures.capacity() == capacity);
    CHECK(json.text(match.captures[0].node) == "2");
}

TEST_CASE("QueryCursor match limit and start depth", "[query][cursor][limits]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    ts::QueryCursor    cursor;

    SECTION("Match limit")
    {
        cursor.setMatchLimit(1);
        CHECK(cursor.getMatchLimit() == 1u);
        const ts::Tree  nested = parser.parseString("[[[[[[[[[[1]]]]]]]]]]");
        const ts::Query any(language, "(_ (_)) @match");
        cursor.exec(any, nested.getRootNode());
        CHECK(countMatches(cursor) == 6u);
        CHECK(cursor.didExceedMatchLimit());
    }

    SECTION("Start depth")
    {
        const ts::Tree  flat = parser.parseString("[1, [2]]");
        const ts::Query numbers(language, "(number) @n");

        // Depth 0: only patterns starting at the root node itself.
        cursor.setMaxStartDepth(0);
        cursor.exec(numbers, flat.getRootNode());
        CHECK(countMatches(cursor) == 0u);

        cursor.resetMaxStartDepth();
        cursor.exec(numbers, flat.getRootNode());
        CHECK(countMatches(cursor) == 2u);

        // A deeply nested number is not found below depth 1.
        const ts::Tree deep = parser.parseString("[[[[1]]]]");
        cursor.setMaxStartDepth(1);
        cursor.exec(numbers, deep.getRootNode());
        CHECK(countMatches(cursor) == 0u);
    }
}

TEST_CASE("QueryCursor byte and point ranges", "[query][cursor][range]")
{
    ts::QueryCursor cursor;

    SECTION("Byte, point and containing ranges")
    {
        ts_test::JsonFixture json("[1, 2, 3, 4, 5]");
        const ts::Query      numbers(json.language, "(number) @n");

        // Matches intersecting bytes [3, 8): 2 and 3.
        REQUIRE(cursor.setByteRange(ts::Extent<std::uint32_t>(3, 8)));
        cursor.exec(numbers, json.root());
        CHECK(countMatches(cursor) == 2u);
        CHECK_FALSE(cursor.setByteRange(ts::Extent<std::uint32_t>(8, 3)));

        REQUIRE(cursor.setByteRange(ts::Extent<std::uint32_t>(0, 0xFFFFFFFFu)));
        REQUIRE(cursor.setPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(0, 2))));
        cursor.exec(numbers, json.root());
        CHECK(countMatches(cursor) == 1u);

        // Containing ranges only keep matches fully inside the range.
        REQUIRE(cursor.setPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(0xFFFFFFFFu, 0xFFFFFFFFu))));
        const ts::Query array(json.language, "(array) @a");
        REQUIRE(cursor.setContainingByteRange(ts::Extent<std::uint32_t>(0, 5)));
        cursor.exec(array, json.root());
        CHECK(countMatches(cursor) == 0u);
        REQUIRE(cursor.setContainingByteRange(ts::Extent<std::uint32_t>(0, 0xFFFFFFFFu)));
        REQUIRE(cursor.setContainingPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(1, 0))));
        cursor.exec(array, json.root());
        CHECK(countMatches(cursor) == 1u);
    }

    SECTION("Point range over several lines")
    {
        // Two objects on different lines: only the first number is inside rows [0, 2).
        ts_test::JsonFixture json("{\n\"a\": 1\n}\n{\n\"b\": 2\n}");
        const ts::Query      numbers(json.language, "(number) @n");
        REQUIRE(cursor.setPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(2, 0))));
        cursor.exec(numbers, json.root());
        CHECK(countMatches(cursor) == 1u);
    }
}

TEST_CASE("QueryCursor remove match", "[query][cursor]")
{
    ts_test::JsonFixture json("[1, 2]");
    const ts::Query      query(json.language, "(number) @n");
    ts::QueryCursor      cursor;

    cursor.exec(query, json.root());
    ts::QueryMatch match;
    REQUIRE(cursor.nextMatch(match));
    CHECK_NOTHROW(cursor.removeMatch(match.id));
    CHECK(cursor.nextMatch(match));
    CHECK(cursor.raw() != nullptr);
}

TEST_CASE("QueryCursorOptions progress callback accessors", "[query][cursor][options]")
{
    ts::QueryCursorOptions options;
    CHECK_FALSE(static_cast<bool>(options.getProgressCallback()));

    options.setProgressCallback([](ts::QueryCursorState *) { return false; });
    CHECK(static_cast<bool>(options.getProgressCallback()));

    options.setProgressCallback(ts::QueryCursorOptions::ProgressCallbackFunction());
    CHECK_FALSE(static_cast<bool>(options.getProgressCallback()));

    const ts::QueryCursorOptions constructed([](ts::QueryCursorState *) { return false; });
    CHECK(static_cast<bool>(constructed.getProgressCallback()));
}

TEST_CASE("QueryCursor progress callback and cancellation", "[query][cursor][options]")
{
    ts_test::JsonFixture json(ts_test::bigJsonArray(2000));
    const ts::Query      query(json.language, "(number) @n");
    ts::QueryCursor      cursor;

    SECTION("The cursor owns the callback of the execution")
    {
        std::shared_ptr<std::uint32_t> calls = std::make_shared<std::uint32_t>(0);
        {
            // tree-sitter keeps a pointer to the options for the whole execution. The options object (and the
            // callback stored in it) dies here, before any match is pulled: the cursor must keep its own copy.
            ts::QueryCursorOptions options(
                    [calls, &json](ts::QueryCursorState *state) -> bool
                    {
                        ++*calls;
                        CHECK(state->current_byte_offset <= json.source.size());
                        return false;
                    });
            cursor.exec(query, json.root(), options);
        }
        CHECK(countMatches(cursor) == 2000u);
        CHECK(*calls > 0u);
        CHECK_FALSE(cursor.wasCancelled());
    }

    SECTION("Returning true cancels the execution")
    {
        // The matches produced are a prefix of the full result.
        cursor.exec(query, json.root(), ts::QueryCursorOptions([](ts::QueryCursorState *) { return true; }));
        CHECK(countMatches(cursor) < 2000u);
        CHECK(cursor.wasCancelled());

        // A new execution clears the cancellation.
        cursor.exec(query, json.root());
        CHECK_FALSE(cursor.wasCancelled());
        CHECK(countMatches(cursor) == 2000u);
    }

    SECTION("Exceptions of the callback are rethrown")
    {
        const ts::Query pairs(json.language, "(pair key: (string) @key)");
        cursor.exec(pairs,
                    json.root(),
                    ts::QueryCursorOptions([](ts::QueryCursorState *) -> bool { throw std::runtime_error("query"); }));
        CHECK_THROWS_WITH(countMatches(cursor), "query");
        CHECK(cursor.wasCancelled());

        // A new execution clears the cancellation.
        cursor.exec(pairs, json.root());
        CHECK_FALSE(cursor.wasCancelled());
        CHECK(countMatches(cursor) > 0u);
    }
}

TEST_CASE("QueryCursor move", "[query][cursor][lifecycle]")
{
    ts_test::JsonFixture json("[1, 2]");
    const ts::Query      query(json.language, "(number) @n");

    ts::QueryCursor cursor;
    cursor.exec(query, json.root());
    ts::QueryCursor moved(std::move(cursor));
    CHECK(countMatches(moved) == 2u);
}
