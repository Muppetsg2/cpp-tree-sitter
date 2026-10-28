// Tree: validity, copy/move, root with offset, edits, incremental parsing, ranges and DOT output.

#include "common.hpp"

// STL
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("Invalid tree returns neutral values", "[tree][null]")
{
    ts::Tree tree(nullptr);

    CHECK_FALSE(tree.isValid());
    CHECK_FALSE(tree.hasError());
    CHECK(tree.getRootNode().isNull());
    CHECK(tree.getRootNodeWithOffset(5, ts::Point(1, 0)).isNull());
    CHECK_FALSE(tree.getLanguage().isValid());
    CHECK(tree.getIncludedRanges().empty());
    CHECK(static_cast<TSTree *>(tree) == nullptr);
    CHECK_FALSE(tree.copy().isValid());
    CHECK_THROWS_AS(tree.edit(ts::InputEdit()), std::logic_error);

    // If either tree is invalid there are no changed ranges.
    ts_test::JsonFixture json("{}");
    CHECK(ts::Tree::getChangedRanges(tree, tree).empty());
    CHECK(ts::Tree::getChangedRanges(tree, json.tree).empty());
    CHECK(ts::Tree::getChangedRanges(json.tree, tree).empty());
}

TEST_CASE("Tree attributes", "[tree]")
{
    ts_test::JsonFixture json("[1, 2]");

    CHECK(json.tree.isValid());
    CHECK_FALSE(json.tree.hasError());
    CHECK(json.tree.getRootNode().getType() == "document");
    CHECK(static_cast<const TSLanguage *>(json.tree.getLanguage()) == static_cast<const TSLanguage *>(json.language));
    CHECK(ts_tree_language(json.tree) == static_cast<const TSLanguage *>(json.language));

    ts_test::JsonFixture broken("[1, ]");
    CHECK(broken.tree.hasError());
}

TEST_CASE("Tree copy and move", "[tree][lifecycle]")
{
    ts_test::JsonFixture json("[1, 2]");

    // Copies are independent trees (ts_tree_copy) with the same content.
    ts::Tree copy(json.tree);
    CHECK(copy.isValid());
    CHECK(static_cast<TSTree *>(copy) != static_cast<TSTree *>(json.tree));
    CHECK(copy.getRootNode().getDescendantsCount() == json.root().getDescendantsCount());

    ts::Tree assigned(nullptr);
    assigned = json.tree;
    CHECK(assigned.isValid());
    CHECK(assigned.getRootNode().getSourceText(json.source) == "[1, 2]");

    ts::Tree explicit_copy = json.tree.copy();
    CHECK(explicit_copy.isValid());

    // Self-assignment keeps the tree.
    ts::Tree *self = &assigned;
    assigned       = *self;
    CHECK(assigned.isValid());
    CHECK(assigned.getRootNode().getType() == "document");

    // Moves transfer the tree.
    TSTree  *raw = copy;
    ts::Tree moved(std::move(copy));
    CHECK(static_cast<TSTree *>(moved) == raw);
    CHECK_FALSE(copy.isValid());

    ts::Tree move_assigned(nullptr);
    move_assigned = std::move(moved);
    CHECK(static_cast<TSTree *>(move_assigned) == raw);
    CHECK_FALSE(moved.isValid());
}

TEST_CASE("Tree root node with offset", "[tree]")
{
    SECTION("Shifted root")
    {
        ts_test::JsonFixture json("[1]");
        const ts::Node       shifted = json.tree.getRootNodeWithOffset(10, ts::Point(2, 0));

        CHECK(shifted.getByteRange() == ts::Extent<std::uint32_t>(10, 13));
        CHECK(shifted.getPointRange().start == ts::Point(2, 0));
        CHECK(shifted.getNamedChild(0).getType() == "array");
    }

    SECTION("Leading whitespace")
    {
        // The root starts at the first token.
        ts_test::JsonFixture json("  [1]");
        CHECK(json.root().getByteRange().start == 2u);
        CHECK(json.root().getType() == "document");
    }
}

TEST_CASE("Tree edit and incremental parsing", "[tree][edit]")
{
    SECTION("Insertion")
    {
        ts_test::JsonFixture json("[1, 2]");

        // "[1, 2]" -> "[1, 2, 3]"
        const std::string   updated = "[1, 2, 3]";
        const ts::InputEdit edit(5, 5, 8, ts::Point(0, 5), ts::Point(0, 5), ts::Point(0, 8));
        json.tree.edit(edit);
        CHECK(json.tree.getRootNode().hasChanges());

        ts::Tree reparsed = json.parser.parseString(updated, json.tree);
        REQUIRE(reparsed.isValid());
        CHECK(reparsed.getRootNode().getNamedChild(0).getNamedChildCount() == 3u);

        const std::vector<ts::Range> changed = ts::Tree::getChangedRanges(json.tree, reparsed);
        REQUIRE_FALSE(changed.empty());
        CHECK(changed.front().byte.start <= 5u);
        CHECK(changed.back().byte.end >= 8u);
    }

    SECTION("Replacement")
    {
        ts_test::JsonFixture json("[1, 2, 3]");
        const ts::Tree       original = json.tree.copy();

        // "[1, 2, 3]" -> "[1, 20, 3]"
        const std::string updated = "[1, 20, 3]";
        json.tree.edit(ts::InputEdit(4, 5, 6, ts::Point(0, 4), ts::Point(0, 5), ts::Point(0, 6)));
        const ts::Tree reparsed = json.parser.parseString(updated, json.tree);

        CHECK_FALSE(reparsed.hasError());
        const ts::Node second = reparsed.getRootNode().getNamedChild(0).getNamedChild(1);
        CHECK(second.getType() == "number");
        CHECK(second.getSourceText(updated) == "20");

        const std::vector<ts::Range> changed = ts::Tree::getChangedRanges(original, reparsed);
        REQUIRE_FALSE(changed.empty());
        CHECK(changed[0].byte.start <= 5u);
        CHECK(changed[0].byte.end >= 6u);
    }

    SECTION("Invalid edit")
    {
        ts_test::JsonFixture json("[1, 2]");
        const ts::InputEdit  invalid(5, 2, 8, ts::Point(0, 5), ts::Point(0, 2), ts::Point(0, 8));
        CHECK_THROWS_AS(json.tree.edit(invalid), std::invalid_argument);
        CHECK_FALSE(json.tree.getRootNode().hasChanges());
    }

    SECTION("No edits, no changed ranges")
    {
        // Different tree instances without recorded edits.
        ts_test::JsonFixture first("{\"key\": \"value\"}");
        ts_test::JsonFixture second("{\"key\": \"value\"}");
        CHECK(ts::Tree::getChangedRanges(first.tree, second.tree).empty());
    }
}

TEST_CASE("Tree included ranges", "[tree][range]")
{
    ts_test::JsonFixture         json("[1]");
    const std::vector<ts::Range> ranges = json.tree.getIncludedRanges();

    // Without restrictions the tree includes the whole document.
    REQUIRE(ranges.size() == 1u);
    CHECK(ranges[0].byte.start == 0u);
    CHECK(ranges[0].byte.end == 0xFFFFFFFFu);
}

TEST_CASE("Tree DOT graph output", "[tree][debug]")
{
    ts_test::JsonFixture json("{\"test\": 123}");

    SECTION("To a FILE")
    {
        const char *path = "cpp-tree-sitter-tree-file.dot";
#if defined(_WIN32)
        std::FILE *file = nullptr;
        fopen_s(&file, path, "w");
#else
        std::FILE *file = std::fopen(path, "w");
#endif // defined(_WIN32)
        REQUIRE(file != nullptr);
        json.tree.printDotGraph(file);
        std::fclose(file);

        const std::string content = ts_test::readFile(path);
        std::remove(path);
        CHECK(ts_test::contains(content, "digraph"));
    }

    SECTION("To a path")
    {
        const std::string path = "cpp-tree-sitter-tree-path.dot";
        CHECK_NOTHROW(json.tree.printDotGraph(path));

        const std::string content = ts_test::readFile(path.c_str());
        CHECK(std::remove(path.c_str()) == 0);
        CHECK(ts_test::contains(content, "digraph"));
        CHECK(ts_test::contains(content, "object"));
        CHECK(ts_test::contains(content, "pair"));
    }

    SECTION("Null files and invalid trees are ignored")
    {
        CHECK_NOTHROW(json.tree.printDotGraph(static_cast<std::FILE *>(nullptr)));

        ts::Tree invalid(nullptr);
        CHECK_NOTHROW(invalid.printDotGraph(static_cast<std::FILE *>(nullptr)));
        CHECK_NOTHROW(invalid.printDotGraph(-1));
        CHECK_NOTHROW(invalid.printDotGraph("cpp-tree-sitter-not-written.dot"));
        CHECK(ts_test::readFile("cpp-tree-sitter-not-written.dot").empty());
    }
}
