// TreeCursor: navigation, fields, positions, reset, copy and move.

#include "common.hpp"

// STL
#include <cstdint>
#include <utility>

TEST_CASE("Invalid tree cursor", "[cursor][null]")
{
    ts::TreeCursor cursor;

    SECTION("Navigation fails")
    {
        CHECK_FALSE(cursor.isValid());
        CHECK_FALSE(cursor.gotoFirstChild());
        CHECK_FALSE(cursor.gotoLastChild());
        CHECK_FALSE(cursor.gotoNextSibling());
        CHECK_FALSE(cursor.gotoPreviousSibling());
        CHECK_FALSE(cursor.gotoParent());
        CHECK(cursor.gotoFirstChildForByte(0) == -1);
        CHECK(cursor.gotoFirstChildForPoint(ts::Point()) == -1);
    }

    SECTION("Attributes return neutral values")
    {
        CHECK(cursor.getCurrentNode().isNull());
        CHECK(cursor.getCurrentFieldName().empty());
        CHECK(cursor.getCurrentFieldID() == 0);
        CHECK(cursor.getCurrentDescendantIndex() == 0u);
        CHECK(cursor.getDepthFromOrigin() == 0u);
        CHECK(cursor == ts::TreeCursor());
    }

    SECTION("Copies and resets stay invalid")
    {
        // Copying an empty TSTreeCursor used to read past its (empty) stack.
        CHECK_FALSE(cursor.copy().isValid());
        CHECK_FALSE(ts::TreeCursor(TSTreeCursor()).isValid());

        ts::TreeCursor from_null = ts::Node::null().getCursor();
        CHECK_FALSE(from_null.isValid());
        CHECK_NOTHROW(cursor.reset(ts::Node::null()));
        CHECK_NOTHROW(cursor.reset(from_null));
        CHECK_FALSE(cursor.isValid());
    }
}

TEST_CASE("Tree cursor navigation", "[cursor][navigation]")
{
    SECTION("Siblings, parent and last child")
    {
        ts_test::JsonFixture json("[1, true]");
        ts::TreeCursor       cursor = json.value().getCursor();
        REQUIRE(cursor.isValid());
        CHECK(cursor.getCurrentNode() == json.value());

        REQUIRE(cursor.gotoFirstChild());
        CHECK(cursor.getCurrentNode().getType() == "[");
        CHECK(cursor.getDepthFromOrigin() == 1u);

        REQUIRE(cursor.gotoNextSibling());
        CHECK(cursor.getCurrentNode().getType() == "number");
        REQUIRE(cursor.gotoPreviousSibling());
        CHECK(cursor.getCurrentNode().getType() == "[");

        REQUIRE(cursor.gotoParent());
        CHECK(cursor.getCurrentNode() == json.value());
        // The cursor never leaves the node it was created for.
        CHECK_FALSE(cursor.gotoParent());

        REQUIRE(cursor.gotoLastChild());
        CHECK(cursor.getCurrentNode().getType() == "]");
        CHECK_FALSE(cursor.gotoNextSibling());
    }

    SECTION("Deep navigation from the root")
    {
        ts_test::JsonFixture json("[1, [2, 3], 4]");
        ts::TreeCursor       cursor = json.root().getCursor();
        CHECK(cursor.getCurrentNode().getType() == "document");

        REQUIRE(cursor.gotoFirstChild());  // array
        REQUIRE(cursor.gotoFirstChild());  // [
        REQUIRE(cursor.gotoNextSibling()); // 1
        REQUIRE(cursor.gotoNextSibling()); // ,
        REQUIRE(cursor.gotoNextSibling()); // inner array
        CHECK(cursor.getCurrentNode().getType() == "array");

        REQUIRE(cursor.gotoFirstChild());  // inner [
        REQUIRE(cursor.gotoNextSibling()); // 2
        CHECK(json.text(cursor.getCurrentNode()) == "2");
        CHECK(cursor.getDepthFromOrigin() == 3u); // document -> array -> array -> 2
    }
}

TEST_CASE("Tree cursor positions and descendants", "[cursor][navigation]")
{
    ts_test::JsonFixture json("[10, 20]");
    ts::TreeCursor       cursor = json.value().getCursor();

    CHECK(cursor.gotoFirstChildForByte(5) == 3); // [ 10 , 20 ]
    CHECK(json.text(cursor.getCurrentNode()) == "20");

    cursor.reset(json.value());
    CHECK(cursor.gotoFirstChildForPoint(ts::Point(0, 1)) == 1);
    CHECK(json.text(cursor.getCurrentNode()) == "10");

    cursor.reset(json.value());
    CHECK(cursor.getCurrentDescendantIndex() == 0u);
    cursor.gotoDescendant(2); // array, [, 10
    CHECK(json.text(cursor.getCurrentNode()) == "10");
    CHECK(cursor.getCurrentDescendantIndex() == 2u);
}

TEST_CASE("Tree cursor fields", "[cursor][fields]")
{
    ts_test::JsonFixture json("{\"a\": 1}");
    ts::TreeCursor       cursor = json.value().getNamedChild(0).getCursor(); // pair

    REQUIRE(cursor.gotoFirstChild());
    CHECK(cursor.getCurrentFieldName() == "key");
    CHECK(cursor.getCurrentFieldID() == json.language.getView().getFieldIDForName("key"));

    // Going back to a descendant by index restores the field.
    const std::uint32_t index = cursor.getCurrentDescendantIndex();
    REQUIRE(cursor.gotoParent());
    cursor.gotoDescendant(index);
    CHECK(cursor.getCurrentFieldName() == "key");

    REQUIRE(cursor.gotoNextSibling()); // ":"
    CHECK(cursor.getCurrentFieldName().empty());
    CHECK(cursor.getCurrentFieldID() == 0);

    REQUIRE(cursor.gotoNextSibling());
    CHECK(cursor.getCurrentFieldName() == "value");
}

TEST_CASE("Tree cursor reset, copy and comparison", "[cursor][lifecycle]")
{
    ts_test::JsonFixture json("[1, 2]");
    ts::TreeCursor       cursor = json.value().getCursor();
    REQUIRE(cursor.gotoFirstChild());

    // A copy is independent of the original.
    ts::TreeCursor copy = cursor.copy();
    CHECK(copy.isValid());
    CHECK(copy == cursor);
    CHECK(copy.getCurrentNode() == cursor.getCurrentNode());
    REQUIRE(copy.gotoNextSibling());
    CHECK(copy != cursor);
    CHECK(cursor.getCurrentNode().getType() == "[");

    // reset(cursor) copies the state of another cursor.
    cursor.reset(copy);
    CHECK(cursor == copy);

    // reset(node) starts again from a node; a null node is ignored.
    cursor.reset(json.root());
    CHECK(cursor.getCurrentNode() == json.root());
    cursor.reset(ts::Node::null());
    CHECK(cursor.getCurrentNode() == json.root());
}

TEST_CASE("Tree cursor move", "[cursor][lifecycle]")
{
    ts_test::JsonFixture json("[1]");
    ts::TreeCursor       cursor = json.value().getCursor();

    ts::TreeCursor moved(std::move(cursor));
    CHECK(moved.isValid());
    CHECK(moved.getCurrentNode() == json.value());
    CHECK_FALSE(cursor.isValid());

    ts::TreeCursor assigned;
    assigned = std::move(moved);
    CHECK(assigned.isValid());
    CHECK_FALSE(moved.isValid());

    // Move-assigning over a valid cursor releases its old state.
    ts::TreeCursor other = json.root().getCursor();
    other                = std::move(assigned);
    CHECK(other.getCurrentNode() == json.value());

    // Self move-assignment keeps the cursor.
    ts::TreeCursor *self = &other;
    other                = std::move(*self);
    CHECK(other.isValid());
    CHECK(other.getCurrentNode() == json.value());
}
