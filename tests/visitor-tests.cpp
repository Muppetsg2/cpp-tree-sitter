// ts::visit (callbacks and visitor objects) and ts::fold.

#include "common.hpp"

// STL
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    // Records enter/leave events and does not descend into objects.
    struct RecordingVisitor
    {
        std::string events;

        ts::VisitAction onEnter(ts::Node node)
        {
            events += "+" + std::string(node.getType()) + " ";
            return node.getType() == "object" ? ts::VisitAction::SkipChildren : ts::VisitAction::Continue;
        }

        void onLeave(ts::Node node)
        {
            events += "-" + std::string(node.getType()) + " ";
        }
    };

    // Visitor object without onLeave.
    struct CountingVisitor
    {
        std::uint32_t count;

        ts::VisitAction onEnter(ts::Node)
        {
            ++count;
            return ts::VisitAction::Continue;
        }
    };

    // Depth of the tree counting only named nodes.
    struct NamedDepth
    {
        int onNode(ts::Node node, ts::Span<int> children)
        {
            int deepest = 0;
            for (int depth : children)
            {
                deepest = depth > deepest ? depth : deepest;
            }
            return node.isNamed() ? deepest + 1 : deepest;
        }
    };

    std::size_t countNodes(ts::Node, ts::Span<std::size_t> children)
    {
        std::size_t total = 1;
        for (std::size_t child : children)
        {
            total += child;
        }
        return total;
    }
} // namespace

TEST_CASE("visit with a boolean callback", "[visitor]")
{
    ts_test::JsonFixture json("[1, 2]");

    SECTION("Returning false visits everything")
    {
        std::uint32_t visited = 0;
        ts::visit(json.root(),
                  [&visited](ts::Node) -> bool
                  {
                      ++visited;
                      return false; // continue
                  });
        CHECK(visited == 7u); // document, array, [, 1, ",", 2, ]
        CHECK(visited == json.root().getDescendantsCount());
    }

    SECTION("Returning true stops the traversal")
    {
        std::uint32_t until_stop = 0;
        ts::visit(json.root(),
                  [&until_stop](ts::Node) -> bool
                  {
                      ++until_stop;
                      return until_stop == 3;
                  });
        CHECK(until_stop == 3u);
    }
}

TEST_CASE("visit with void and VisitAction callbacks", "[visitor]")
{
    ts_test::JsonFixture json("[{\"a\": 1}, 2]");

    SECTION("void visits everything, like preorder()")
    {
        std::vector<ts::Node> visited;
        ts::visit(json.root(), [&visited](ts::Node node) { visited.push_back(node); });

        std::vector<ts::Node> walked;
        for (ts::Node node : ts::preorder(json.root()))
        {
            walked.push_back(node);
        }
        CHECK(visited.size() == json.root().getDescendantsCount());
        CHECK(visited == walked);
    }

    SECTION("SkipChildren")
    {
        std::string order;
        ts::visit(json.root(),
                  [&order](ts::Node node) -> ts::VisitAction
                  {
                      order += std::string(node.getType()) + " ";
                      return node.getType() == "object" ? ts::VisitAction::SkipChildren : ts::VisitAction::Continue;
                  });
        CHECK(order == "document array [ object , number ] ");
    }

    SECTION("Stop")
    {
        std::uint32_t until_stop = 0;
        ts::visit(json.root(),
                  [&until_stop](ts::Node node) -> ts::VisitAction
                  {
                      ++until_stop;
                      return node.getType() == "array" ? ts::VisitAction::Stop : ts::VisitAction::Continue;
                  });
        CHECK(until_stop == 2u);
    }
}

TEST_CASE("visit with visitor objects", "[visitor]")
{
    ts_test::JsonFixture json("[{\"a\": 1}, 2]");

    // onLeave is called after the children, in the order a recursive walk would produce (LIFO).
    RecordingVisitor recording;
    ts::visit(json.root(), recording);
    CHECK(recording.events == "+document +array +[ -[ +object -object +, -, +number -number +] -] -array -document ");

    CountingVisitor counting = { 0 };
    ts::visit(json.root(), counting);
    CHECK(counting.count == json.root().getDescendantsCount());
}

TEST_CASE("visit stays inside the subtree and ignores null roots", "[visitor]")
{
    ts_test::JsonFixture json("[[1, 2], 3]");

    SECTION("Subtree")
    {
        // The inner array itself and [ 1 , 2 ]; nothing outside of it.
        const ts::Node inner_array = json.value().getNamedChild(0);
        std::uint32_t  inner       = 0;
        bool           escaped     = false;
        ts::visit(inner_array,
                  [&](ts::Node node)
                  {
                      ++inner;
                      escaped = escaped || node == json.root() || node == json.value() || json.text(node) == "3";
                  });
        CHECK(inner == 6u);
        CHECK_FALSE(escaped);
    }

    SECTION("Leaf")
    {
        std::uint32_t calls = 0;
        ts::visit(json.value().getNamedChild(1),
                  [&calls](ts::Node node)
                  {
                      ++calls;
                      CHECK(node.getType() == "number");
                  });
        CHECK(calls == 1u);
    }

    SECTION("Null root")
    {
        std::uint32_t calls = 0;
        ts::visit(ts::Node::null(), [&calls](ts::Node) { ++calls; });
        CHECK(calls == 0u);

        RecordingVisitor recording;
        ts::visit(ts::Node::null(), recording);
        CHECK(recording.events.empty());
    }
}

TEST_CASE("fold computes values bottom-up", "[visitor][fold]")
{
    ts_test::JsonFixture json("[[1, [2]], 3]");

    SECTION("Folder object")
    {
        // document > array > array > array > number
        CHECK(ts::fold<int>(json.root(), NamedDepth()) == 5);
    }

    SECTION("Callable folders")
    {
        CHECK(ts::fold<std::size_t>(json.root(), &countNodes) == json.root().getDescendantsCount());

        const std::size_t nodes = ts::fold<std::size_t>(json.root(),
                                                        [](ts::Node, ts::Span<std::size_t> children)
                                                        {
                                                            std::size_t total = 1;
                                                            for (std::size_t child : children)
                                                            {
                                                                total += child;
                                                            }
                                                            return total;
                                                        });
        CHECK(nodes == json.root().getDescendantsCount());
    }

    SECTION("A leaf receives an empty span")
    {
        CHECK(ts::fold<int>(json.value().getNamedChild(1), NamedDepth()) == 1);
    }

    SECTION("Null root")
    {
        CHECK_THROWS_AS(ts::fold<int>(ts::Node::null(), NamedDepth()), std::invalid_argument);
    }
}
