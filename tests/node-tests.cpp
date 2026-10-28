// Node: attributes, flags, ranges, navigation, search, fields, edits and comparison.

#include "common.hpp"

// STL
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

TEST_CASE("Null node returns neutral values", "[node][null]")
{
    const ts::Node node = ts::Node::null();

    SECTION("Attributes and flags")
    {
        CHECK(node.isNull());
        CHECK(node == ts::Node());
        CHECK(node.getID() == 0u);
        CHECK(node.getType().empty());
        CHECK(node.getGrammarType().empty());
        CHECK(node.getSymbol() == 0);
        CHECK(node.getGrammarSymbol() == 0);
        CHECK_FALSE(node.getLanguage().isValid());
        CHECK_FALSE(node.isNamed());
        CHECK_FALSE(node.isMissing());
        CHECK_FALSE(node.isExtra());
        CHECK_FALSE(node.isError());
        CHECK_FALSE(node.hasError());
        CHECK_FALSE(node.hasChanges());
        CHECK(node.getParseState() == 0);
        CHECK(node.getNextParseState() == 0);
        CHECK(node.getSExpr() == nullptr);
        CHECK_FALSE(node.getCursor().isValid());
    }

    SECTION("Ranges and source text")
    {
        CHECK(node.getByteRange() == ts::Extent<std::uint32_t>());
        CHECK(node.getPointRange() == ts::Extent<ts::Point>());
        CHECK(node.getSourceRange("any source").empty());
        CHECK(node.getSourceText("any source").empty());
    }

    SECTION("Navigation")
    {
        CHECK(node.getParent().isNull());
        CHECK(node.getParent().getParent().isNull());
        CHECK(node.getNextSibling().isNull());
        CHECK(node.getPreviousSibling().isNull());
        CHECK(node.getNextNamedSibling().isNull());
        CHECK(node.getChild(0).isNull());
        CHECK(node.getNamedChild(0).isNull());
        CHECK(node.getChildCount() == 0u);
        CHECK(node.getNamedChildCount() == 0u);
    }

    SECTION("Search, fields and lookups")
    {
        CHECK(node.getDescendantsCount() == 0u);
        CHECK(node.getFirstChildForByte(0).isNull());
        CHECK(node.getFirstNamedChildForByte(0).isNull());
        CHECK(node.getDescendantForByteRange(ts::Extent<std::uint32_t>(0, 1)).isNull());
        CHECK(node.getNamedDescendantForByteRange(ts::Extent<std::uint32_t>(0, 1)).isNull());
        CHECK(node.getDescendantForPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(0, 1))).isNull());
        CHECK(node.getNamedDescendantForPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(0, 1))).isNull());
        CHECK(node.getFieldNameForChild(0).empty());
        CHECK(node.getFieldNameForNamedChild(0).empty());
        CHECK(node.getChildByFieldName("key").isNull());
        CHECK(node.getChildByFieldID(1).isNull());
        CHECK(node.getChildByType("array").isNull());
        CHECK(node.getNamedChildByType("pair").isNull());
        CHECK(node.getChildBySymbol(1).isNull());
        CHECK(node.getNamedChildBySymbol(1).isNull());
    }

    SECTION("Edit")
    {
        ts::Node editable = node;
        CHECK_THROWS_AS(editable.edit(ts::InputEdit(0, 0, 1, ts::Point(0, 0), ts::Point(0, 0), ts::Point(0, 1))),
                        std::logic_error);
    }
}

TEST_CASE("Node type, symbol and language", "[node]")
{
    ts_test::JsonFixture json("[1, null]");
    const ts::Node       array = json.value();

    CHECK(json.root().getType() == "document");
    CHECK(array.getType() == "array");
    CHECK(array.getGrammarType() == "array");
    CHECK(array.getSymbol() == json.language.getView().getSymbolForName("array", true));
    CHECK(array.getGrammarSymbol() == array.getSymbol());
    CHECK(static_cast<const TSLanguage *>(array.getLanguage()) == static_cast<const TSLanguage *>(json.language));
    CHECK(array.getID() != 0u);
    CHECK(array.getID() == json.value().getID());
    CHECK(array.getID() != json.root().getID());
}

TEST_CASE("Node flags", "[node]")
{
    SECTION("Named, anonymous and extra nodes")
    {
        ts_test::JsonFixture json("[1, null] // comment");
        CHECK(json.value().isNamed());
        CHECK_FALSE(json.value().getChild(0).isNamed()); // "["
        CHECK_FALSE(json.value().isMissing());
        CHECK_FALSE(json.value().isError());
        CHECK_FALSE(json.value().hasChanges());
        CHECK_FALSE(json.root().hasError());

        const ts::Node comment = json.root().getNamedChild(1);
        CHECK(comment.getType() == "comment");
        CHECK(comment.isExtra());
    }

    SECTION("Missing nodes")
    {
        // A missing closing bracket is inserted as a MISSING node.
        ts_test::JsonFixture missing("[1, 2");
        CHECK(missing.root().hasError());
        const ts::Node last = missing.value().getLastChild();
        CHECK(last.isMissing());
        CHECK(last.getType() == "]");
    }

    SECTION("Error nodes")
    {
        // An unexpected token produces an ERROR node.
        ts_test::JsonFixture error("[1, @]");
        CHECK(error.root().hasError());
        bool found_error = false;
        ts::visit(error.root(), [&](ts::Node node) { found_error = found_error || node.isError(); });
        CHECK(found_error);
    }
}

TEST_CASE("Node ranges and source text", "[node]")
{
    ts_test::JsonFixture json("[\n  1,\n  \"text\"\n]");
    const ts::Node       string = json.value().getNamedChild(1);

    CHECK(string.getByteRange() == ts::Extent<std::uint32_t>(9, 15));
    CHECK(string.getPointRange() == ts::Extent<ts::Point>(ts::Point(2, 2), ts::Point(2, 8)));
    CHECK(string.getSourceRange(json.source) == "\"text\"");
    CHECK(json.text(string) == "\"text\"");
    CHECK(json.text(json.value()) == json.source);

    // A source shorter than the node yields an empty view instead of reading out of bounds.
    CHECK(string.getSourceRange("[1]").empty());
    CHECK(string.getSourceText("[1]").empty());
}

TEST_CASE("Node S-expression", "[node]")
{
    ts_test::JsonFixture           json("[1]");
    const ts::details::Owned<char> sexpr = json.root().getSExpr();
    REQUIRE(sexpr != nullptr);
    CHECK(std::string(sexpr.get()) == "(document (array (number)))");
}

TEST_CASE("Node navigation: children, parent and siblings", "[node][navigation]")
{
    ts_test::JsonFixture json("[1, true, null]");
    const ts::Node       array = json.value();

    SECTION("Children")
    {
        CHECK(array.getChildCount() == 7u); // [ 1 , true , null ]
        CHECK(array.getNamedChildCount() == 3u);
        CHECK(array.getFirstChild().getType() == "[");
        CHECK(array.getLastChild().getType() == "]");
        CHECK(array.getChild(7).isNull()); // out of bounds
        CHECK(array.getFirstNamedChild().getType() == "number");
        CHECK(array.getLastNamedChild().getType() == "null");
        CHECK(array.getNamedChild(3).isNull()); // out of bounds
    }

    SECTION("Parent and siblings")
    {
        const ts::Node number = array.getNamedChild(0);
        const ts::Node truth  = array.getNamedChild(1);
        CHECK(number.getParent() == array);
        CHECK(json.root().getParent().isNull());

        CHECK(number.getNextSibling() == array.getChild(2)); // ","
        CHECK(number.getNextSibling().getType() == ",");
        CHECK(number.getPreviousSibling().getType() == "[");
        CHECK(truth.getPreviousSibling() == array.getChild(2));

        CHECK(number.getNextNamedSibling() == truth);
        CHECK(truth.getPreviousNamedSibling() == number);
        CHECK(number.getPreviousNamedSibling().isNull());
    }

    SECTION("Descendants")
    {
        const ts::Node number = array.getNamedChild(0);
        CHECK(json.root().getChildWithDescendant(number) == array);
        CHECK(json.root().getDescendantsCount() == 9u); // document, array and its 7 children
    }
}

TEST_CASE("Node search by position", "[node][search]")
{
    SECTION("Single line")
    {
        ts_test::JsonFixture json("[10, 20]");
        const ts::Node       array = json.value();

        CHECK(array.getFirstChildForByte(2).getType() == "number");
        CHECK(array.getFirstNamedChildForByte(4).getSourceText(json.source) == "20");

        CHECK(json.root().getDescendantForByteRange(ts::Extent<std::uint32_t>(1, 2)).getType() == "number");
        CHECK(json.root().getNamedDescendantForByteRange(ts::Extent<std::uint32_t>(0, 1)).getType() == "array");
        CHECK(json.root()
                      .getDescendantForPointRange(ts::Extent<ts::Point>(ts::Point(0, 5), ts::Point(0, 6)))
                      .getSourceText(json.source)
              == "20");
        CHECK(json.root()
                      .getNamedDescendantForPointRange(ts::Extent<ts::Point>(ts::Point(0, 3), ts::Point(0, 4)))
                      .getType()
              == "array");
    }

    SECTION("Unnamed descendants")
    {
        // Byte 0 is '{', an anonymous punctuation node.
        ts_test::JsonFixture json("{\"key\": \"value\"}");
        const ts::Node       brace = json.root().getDescendantForByteRange(ts::Extent<std::uint32_t>(0, 1));
        CHECK_FALSE(brace.isNamed());
        CHECK(brace.getType() == "{");
        CHECK(json.root().getDescendantForPointRange(ts::Extent<ts::Point>(ts::Point(0, 0), ts::Point(0, 1))) == brace);
    }

    SECTION("Several lines")
    {
        ts_test::JsonFixture json("{\n  \"key\": \"value\"\n}");
        const ts::Node       value = json.root().getNamedDescendantForByteRange(ts::Extent<std::uint32_t>(11, 16));
        CHECK(value.getType() == "string");
        CHECK(json.text(value) == "\"value\"");
        CHECK(json.text(json.root().getNamedDescendantForPointRange(
                      ts::Extent<ts::Point>(ts::Point(1, 9), ts::Point(1, 16))))
              == "\"value\"");
    }
}

TEST_CASE("Node fields", "[node][fields]")
{
    ts_test::JsonFixture json("{\"a\": 1}");
    const ts::Node       pair = json.value().getNamedChild(0);
    REQUIRE(pair.getType() == "pair");

    const ts::Node key = pair.getChildByFieldName("key");
    CHECK(json.text(key) == "\"a\"");
    CHECK(json.text(pair.getChildByFieldID(json.language.getView().getFieldIDForName("value"))) == "1");
    CHECK(pair.getChildByFieldName("missing").isNull());
    // A default-constructed view has a null data() pointer, which must not reach the C API.
    CHECK(pair.getChildByFieldName(ts::StringView()).isNull());

    CHECK(pair.getFieldNameForChild(0) == "key");
    CHECK(pair.getFieldNameForChild(1).empty()); // ":"
    CHECK(pair.getFieldNameForNamedChild(1) == "value");
}

TEST_CASE("Node lookup by type and symbol", "[node][search]")
{
    ts_test::JsonFixture   json("{\"key\": [\"x\", 1, 2]}");
    const ts::LanguageView view   = json.language.getView();
    const ts::Node         object = json.value();
    const ts::Node         array  = object.getNamedChild(0).getChildByFieldName("value");
    REQUIRE(array.getType() == "array");

    SECTION("By type")
    {
        // Named (AST) and anonymous (CST) children.
        CHECK(object.getChildByType("pair").isNamed());
        CHECK(object.getChildByType("{").getType() == "{");
        CHECK_FALSE(object.getChildByType("{").isNamed());
        CHECK(json.text(array.getChildByType("number")) == "1");

        // A valid type that is not a child, and an unknown type.
        CHECK(object.getChildByType("array").isNull());
        CHECK(object.getChildByType("no_such_type").isNull());

        // Named lookups skip anonymous nodes.
        CHECK(json.text(array.getNamedChildByType("number")) == "1");
        CHECK(object.getNamedChildByType("pair").getType() == "pair");
        CHECK(object.getNamedChildByType("{").isNull());
    }

    SECTION("By symbol")
    {
        const ts::Symbol pair  = view.getSymbolForName("pair", true);
        const ts::Symbol brace = view.getSymbolForName("{", false);
        const ts::Symbol str   = view.getSymbolForName("string", true);
        REQUIRE(pair != 0);
        REQUIRE(brace != 0);

        CHECK(object.getChildBySymbol(pair).getType() == "pair");
        CHECK(object.getChildBySymbol(brace).getType() == "{");
        CHECK(object.getChildBySymbol(view.getSymbolForName("array", true)).isNull());
        CHECK(object.getChildBySymbol(0).isNull());
        CHECK(json.text(array.getChildBySymbol(str)) == "\"x\"");

        CHECK(object.getNamedChildBySymbol(pair).getType() == "pair");
        CHECK(object.getNamedChildBySymbol(brace).isNull());
        CHECK(json.text(array.getNamedChildBySymbol(str)) == "\"x\"");
        CHECK(array.getNamedChildBySymbol(0).isNull());
    }
}

#if SIZE_MAX > UINT32_MAX
TEST_CASE("Node lookups reject names longer than 4GB", "[node][search][exceptions]")
{
    ts_test::JsonFixture json("{\"a\": 1}");
    const ts::Node       pair = json.value().getNamedChild(0);

    // A view with an artificial size, nothing of that size is allocated.
    const char           *fake_str      = "fake";
    constexpr std::size_t overflow_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1;
    const ts::StringView  huge_view(fake_str, overflow_size);

    CHECK_THROWS_AS(pair.getChildByFieldName(huge_view), std::length_error);
    CHECK_THROWS_AS(pair.getChildByType(huge_view), std::length_error);
    CHECK_THROWS_AS(pair.getNamedChildByType(huge_view), std::length_error);
    CHECK_THROWS_AS(ts::Node::null().getChildByType(huge_view), std::length_error);
}
#endif // SIZE_MAX > UINT32_MAX

TEST_CASE("Node parse states", "[node]")
{
    ts_test::JsonFixture json("{\"a\": 1}");
    const ts::Node       object = json.value();
    const ts::Node       key    = object.getNamedChild(0).getNamedChild(0);

    CHECK(object.getParseState() != 0);
    CHECK(key.getNextParseState() != 0);
    CHECK(key.getNextParseState() < json.language.getView().getStatesCount());
}

TEST_CASE("Node edit", "[node][edit]")
{
    ts_test::JsonFixture json("[1, 2]");
    ts::Node             second = json.value().getNamedChild(1);
    REQUIRE(second.getByteRange() == ts::Extent<std::uint32_t>(4, 5));

    // "[1, 2]" -> "[100, 2]": the node fetched before the edit has to be adjusted.
    const ts::InputEdit edit(2, 2, 4, ts::Point(0, 2), ts::Point(0, 2), ts::Point(0, 4));
    json.tree.edit(edit);
    second.edit(edit);
    CHECK(second.getByteRange() == ts::Extent<std::uint32_t>(6, 7));
    CHECK(json.tree.getRootNode().hasChanges());

    // An invalid edit is rejected before anything changes.
    const ts::InputEdit backwards(4, 2, 6, ts::Point(0, 4), ts::Point(0, 2), ts::Point(0, 6));
    CHECK_THROWS_AS(second.edit(backwards), std::invalid_argument);
    CHECK(second.getByteRange() == ts::Extent<std::uint32_t>(6, 7));
}

TEST_CASE("Node comparison", "[node]")
{
    ts_test::JsonFixture json("[1, 1]");
    const ts::Node       first  = json.value().getNamedChild(0);
    const ts::Node       second = json.value().getNamedChild(1);

    CHECK(first == first);
    CHECK(first == json.value().getNamedChild(0));
    CHECK(first != second);
    CHECK_FALSE(first == second);
    CHECK(static_cast<TSNode>(first).id == static_cast<TSNode>(json.value().getNamedChild(0)).id);

    // The same position in another tree is a different node.
    ts_test::JsonFixture other("[1, 1]");
    CHECK(first != other.value().getNamedChild(0));
}

TEST_CASE("Node cursor and children views", "[node][iterators]")
{
    ts_test::JsonFixture json("[1, true]");

    const ts::TreeCursor cursor = json.root().getCursor();
    CHECK(cursor.isValid());
    CHECK(cursor.getCurrentNode() == json.root());

    std::string named;
    for (ts::Node child : json.value().getNamedChildren())
    {
        named += std::string(child.getType()) + ";";
    }
    CHECK(named == "number;true;");
    CHECK(json.value().getChildren().size() == 5u);
}
