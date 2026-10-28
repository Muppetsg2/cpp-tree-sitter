// Basic value types (Point, Extent, InputEdit, Range, StringView, Span/ByteView, std::hash, feature macros) and
// end-to-end usage of the library.

#include "common.hpp"

// STL
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

TEST_CASE("Point construction, conversion and comparison", "[types][point]")
{
    SECTION("Construction and conversion")
    {
        const ts::Point origin;
        CHECK(origin.row == 0u);
        CHECK(origin.column == 0u);

        const ts::Point point(3, 7);
        CHECK(point.row == 3u);
        CHECK(point.column == 7u);

        const TSPoint raw = point;
        CHECK(raw.row == 3u);
        CHECK(raw.column == 7u);
        CHECK(ts::Point(raw) == point);
    }

    SECTION("Comparison")
    {
        CHECK(ts::Point(1, 2) == ts::Point(1, 2));
        CHECK(ts::Point(1, 2) != ts::Point(1, 3));
        CHECK(ts::Point(0, 9) < ts::Point(1, 0));
        CHECK(ts::Point(1, 2) < ts::Point(1, 3));
        CHECK(ts::Point(1, 3) > ts::Point(1, 2));
        CHECK(ts::Point(1, 2) <= ts::Point(1, 2));
        CHECK(ts::Point(2, 0) >= ts::Point(1, 9));
        CHECK(ts::Point::isValidRange(ts::Point(0, 1), ts::Point(0, 1)));
        CHECK_FALSE(ts::Point::isValidRange(ts::Point(1, 0), ts::Point(0, 5)));
    }
}

TEST_CASE("Point and Node hashing", "[types][hash]")
{
    std::unordered_set<ts::Point> points;
    points.insert(ts::Point(0, 1));
    points.insert(ts::Point(0, 1));
    points.insert(ts::Point(1, 0));
    CHECK(points.size() == 2u);

    ts_test::JsonFixture         json("[1, 2]");
    std::unordered_set<ts::Node> nodes;
    for (ts::Node node : ts::preorder(json.root()))
    {
        nodes.insert(node);
    }
    nodes.insert(json.tree.getRootNode());
    CHECK(nodes.size() == json.root().getDescendantsCount());
}

TEST_CASE("Extent", "[types][extent]")
{
    const ts::Extent<std::uint32_t> empty;
    CHECK(empty.start == 0u);
    CHECK(empty.end == 0u);

    const ts::Extent<std::uint32_t> range(2, 5);
    CHECK(range.start == 2u);
    CHECK(range.end == 5u);
    CHECK(range == ts::Extent<std::uint32_t>(2, 5));
    CHECK(range != ts::Extent<std::uint32_t>(2, 6));
    CHECK(range != ts::Extent<std::uint32_t>(1, 5));

    const ts::Extent<ts::Point> points(ts::Point(0, 1), ts::Point(2, 3));
    CHECK(points.start == ts::Point(0, 1));
    CHECK(points.end == ts::Point(2, 3));
}

TEST_CASE("InputEdit validation and conversion", "[types][edit]")
{
    const ts::InputEdit empty;
    CHECK(empty.isValid());

    const ts::InputEdit edit(2, 4, 6, ts::Point(0, 2), ts::Point(0, 4), ts::Point(0, 6));
    CHECK(edit.isValid());
    CHECK_NOTHROW(edit.validate());

    const TSInputEdit raw = edit;
    CHECK(raw.start_byte == 2u);
    CHECK(raw.old_end_byte == 4u);
    CHECK(raw.new_end_byte == 6u);
    CHECK(ts::Point(raw.new_end_point) == ts::Point(0, 6));

    const ts::InputEdit copy(raw);
    CHECK(copy.start_point == edit.start_point);
    CHECK(copy.old_end_point == edit.old_end_point);

    // The old end must not precede the start.
    const ts::InputEdit backwards(4, 2, 6, ts::Point(0, 4), ts::Point(0, 2), ts::Point(0, 6));
    CHECK_FALSE(backwards.isValid());
    CHECK_THROWS_AS(backwards.validate(), std::invalid_argument);
}

TEST_CASE("Range conversion, comparison and edit", "[types][range][edit]")
{
    ts::Range range(ts::Extent<ts::Point>(ts::Point(0, 10), ts::Point(0, 20)), ts::Extent<std::uint32_t>(10, 20));

    const TSRange raw = range;
    CHECK(raw.start_byte == 10u);
    CHECK(raw.end_byte == 20u);
    CHECK(ts::Range(raw) == range);
    CHECK(ts::Range(raw) != ts::Range(ts::Extent<ts::Point>(), ts::Extent<std::uint32_t>(0, 1)));

    const ts::InputEdit insertion(2, 2, 7, ts::Point(0, 2), ts::Point(0, 2), ts::Point(0, 7));
    const ts::InputEdit invalid(4, 2, 6, ts::Point(0, 4), ts::Point(0, 2), ts::Point(0, 6));

    // A rejected edit leaves the range unchanged.
    CHECK_THROWS_AS(range.edit(invalid), std::invalid_argument);
    CHECK(range.byte == ts::Extent<std::uint32_t>(10, 20));

    // Insert 5 bytes at byte 2: the whole range moves by 5.
    range.edit(insertion);
    CHECK(range.byte == ts::Extent<std::uint32_t>(15, 25));
    CHECK(range.point == ts::Extent<ts::Point>(ts::Point(0, 15), ts::Point(0, 25)));
}

TEST_CASE("Point edit", "[types][point][edit]")
{
    ts::Point           point(0, 10);
    std::uint32_t       byte = 10;
    const ts::InputEdit insertion(2, 2, 7, ts::Point(0, 2), ts::Point(0, 2), ts::Point(0, 7));
    const ts::InputEdit invalid(4, 2, 6, ts::Point(0, 4), ts::Point(0, 2), ts::Point(0, 6));

    CHECK_THROWS_AS(point.edit(invalid, byte), std::invalid_argument);
    CHECK(point == ts::Point(0, 10));
    CHECK(byte == 10u);

    point.edit(insertion, byte);
    CHECK(point == ts::Point(0, 15));
    CHECK(byte == 15u);
}

TEST_CASE("StringView", "[types][string_view]")
{
    const std::string    text = "tree-sitter";
    const ts::StringView view(text);

    CHECK(view.size() == text.size());
    CHECK_FALSE(view.empty());
    CHECK(view.data() == text.data());
    CHECK(view.front() == 't');
    CHECK(view.back() == 'r');
    CHECK(view.substr(5) == "sitter");
    CHECK(view.substr(0, 4) == "tree");
    CHECK(view.compare("tree-sitter") == 0);
    CHECK(view.compare("tree") > 0);
    CHECK(view == ts::StringView("tree-sitter"));
    CHECK(view != ts::StringView("tree"));
    CHECK(std::string(view) == text);
    CHECK_THROWS_AS(view.substr(100), std::out_of_range);

    ts::StringView prefix = view;
    prefix.remove_prefix(5);
    CHECK(prefix == "sitter");

    const ts::StringView empty;
    CHECK(empty.empty());
    CHECK(empty.size() == 0u);
}

TEST_CASE("Span and ByteView", "[types][span]")
{
    const std::vector<int>    values = { 1, 2, 3, 4 };
    const ts::Span<const int> span(values);

    CHECK(span.size() == 4u);
    CHECK_FALSE(span.empty());
    CHECK(span.data() == values.data());
    CHECK(span[2] == 3);
    CHECK(span.front() == 1);
    CHECK(span.back() == 4);
    CHECK(span.first(2).size() == 2u);
    CHECK(span.last(1)[0] == 4);
    CHECK(span.subspan(1, 2)[1] == 3);
    CHECK(span.subspan(3).size() == 1u);

    int sum = 0;
    for (int value : span)
    {
        sum += value;
    }
    CHECK(sum == 10);

    const std::uint8_t bytes[] = { 0x7b, 0x7d };
    const ts::ByteView view(bytes, 2);
    CHECK(view.size() == 2u);
    CHECK(view[0] == 0x7b);

    CHECK(ts::Span<const int>().empty());
}

TEST_CASE("Feature macros match the types used by the API", "[types][features]")
{
#if CPP_TS_HAS_STRING_VIEW
    CHECK(std::is_same<ts::StringView, std::string_view>::value);
#else
    CHECK(std::is_same<ts::StringView, ts::details::StringView>::value);
#endif // CPP_TS_HAS_STRING_VIEW

#if CPP_TS_HAS_SPAN
    CHECK(std::is_same<ts::ByteView, std::span<const std::uint8_t>>::value);
#else
    CHECK(std::is_same<ts::ByteView, ts::details::Span<const std::uint8_t>>::value);
#endif // CPP_TS_HAS_SPAN

    typedef decltype(ts::LanguageView().getMetadata()) Metadata;
#if CPP_TS_HAS_OPTIONAL
    CHECK(std::is_same<Metadata, std::optional<ts::LanguageMetadata>>::value);
#else
    CHECK(std::is_same<Metadata, ts::LanguageMetadata>::value);
#endif // CPP_TS_HAS_OPTIONAL

    // Library features are never enabled above the language level they need.
    CHECK((!CPP_TS_HAS_STRING_VIEW || CPP_TS_HAS_CXX17));
    CHECK((!CPP_TS_HAS_OPTIONAL || CPP_TS_HAS_CXX17));
    CHECK((!CPP_TS_HAS_SPAN || CPP_TS_HAS_CXX20));
    CHECK((!CPP_TS_HAS_RANGES || CPP_TS_HAS_CONCEPTS));
    CHECK((!CPP_TS_HAS_EXPECTED || CPP_TS_HAS_CXX23));
}

TEST_CASE("End-to-end parsing", "[integration][parser][tree][node]")
{
    ts::Language      lang(tree_sitter_json());
    ts::Parser        parser(lang);
    const std::string code = "[1, 2, 3]";
    const ts::Tree    tree = parser.parseString(code);

    SECTION("Tree and root node")
    {
        CHECK_FALSE(tree.hasError());

        const ts::Node root = tree.getRootNode();
        REQUIRE_FALSE(root.isNull());
        CHECK(root.getType() == "document");
    }

    SECTION("Node traversal and text extraction")
    {
        // The first named child of the document is the array, with the numbers 1, 2 and 3.
        const ts::Node array_node = tree.getRootNode().getNamedChild(0);
        REQUIRE(array_node.getType() == "array");
        REQUIRE(array_node.getNamedChildCount() == 3u);

        const ts::Node first_number = array_node.getNamedChild(0);
        CHECK(first_number.getType() == "number");
        CHECK(first_number.getSourceText(code) == "1");
        CHECK(array_node.getLastNamedChild().getSourceText(code) == "3");
    }
}

TEST_CASE("End-to-end field navigation", "[integration][node]")
{
    ts_test::JsonFixture json("{\"name\": \"Mark\"}");

    // document -> object -> pair
    const ts::Node pair_node = json.value().getNamedChild(0);
    REQUIRE(pair_node.getType() == "pair");

    // Children are fetched by the field names defined by the grammar.
    const ts::Node key_node   = pair_node.getChildByFieldName("key");
    const ts::Node value_node = pair_node.getChildByFieldName("value");
    REQUIRE_FALSE(key_node.isNull());
    REQUIRE_FALSE(value_node.isNull());

    CHECK(json.text(key_node) == "\"name\"");
    CHECK(json.text(value_node) == "\"Mark\"");
}

TEST_CASE("End-to-end query execution", "[integration][query]")
{
    ts_test::JsonFixture json("[{\"id\": 1}, {\"id\": 2}, {\"id\": 3}]");
    const ts::Query      query(json.language, "(number) @num");

    ts::QueryCursor cursor;
    cursor.exec(query, json.root());

    ts::QueryMatch           match;
    std::vector<std::string> captured_numbers;
    while (cursor.nextMatch(match))
    {
        for (const ts::QueryCapture &capture : match.captures)
        {
            captured_numbers.push_back(json.text(capture.node));
        }
    }

    REQUIRE(captured_numbers.size() == 3u);
    CHECK(captured_numbers[0] == "1");
    CHECK(captured_numbers[1] == "2");
    CHECK(captured_numbers[2] == "3");
}
