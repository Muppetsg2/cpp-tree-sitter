// Iterators: children ranges, ChildIterator operators, pre-order walk and lookahead iterator.

#include "common.hpp"

// STL
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if CPP_TS_HAS_RANGES
#include <ranges>
#endif // CPP_TS_HAS_RANGES

namespace
{
    std::string types(const ts::Children &children)
    {
        std::string result;
        for (ts::Node child : children)
        {
            result += std::string(child.getType()) + " ";
        }
        return result;
    }
} // namespace

TEST_CASE("Children range", "[iterators][children]")
{
    ts_test::JsonFixture json("[1, true, null]");

    SECTION("All and named children")
    {
        const ts::Children all(json.value());
        CHECK(types(all) == "[ number , true , null ] ");
        CHECK(all.size() == 7u);
        CHECK(all.size() == json.value().getChildCount());
        CHECK_FALSE(all.empty());

        const ts::Children named(json.value(), ts::ChildScope::NamedOnly);
        CHECK(types(named) == "number true null ");
        CHECK(named.size() == 3u);

        CHECK(types(json.value().getChildren()) == "[ number , true , null ] ");
        CHECK(types(json.value().getNamedChildren()) == "number true null ");

        std::vector<std::string> texts;
        for (ts::Node child : json.value().getNamedChildren())
        {
            texts.push_back(json.text(child));
        }
        REQUIRE(texts.size() == 3u);
        CHECK(texts[0] == "1");
        CHECK(texts[2] == "null");
    }

    SECTION("Nodes without children")
    {
        // Leaves and null nodes have no children: begin() equals end() in both directions.
        const ts::Children leaf(json.value().getNamedChild(0));
        CHECK(leaf.empty());
        CHECK(leaf.begin() == leaf.end());
        CHECK(leaf.end() == leaf.begin());
        CHECK_FALSE(leaf.begin() != leaf.end());

        CHECK(types(ts::Children(ts::Node::null())).empty());
        CHECK(types(ts::Children()).empty());
    }

#if CPP_TS_HAS_RANGES
    SECTION("Children views compose with std::views")
    {
        auto literals = json.value().getChildren() | std::views::filter([](ts::Node n) { return n.isNamed(); })
                      | std::views::transform([](ts::Node n) { return n.getType(); });
        std::vector<std::string_view> found;
        for (std::string_view type : literals)
        {
            found.push_back(type);
        }
        CHECK(found == std::vector<std::string_view>{ "number", "true", "null" });
    }
#endif // CPP_TS_HAS_RANGES
}

TEST_CASE("ChildIterator operators", "[iterators][children]")
{
    ts_test::JsonFixture json("[1, 2]");
    const ts::Children   children(json.value());

    ts::ChildIterator it = children.begin();
    CHECK((*it).getType() == "[");
    CHECK(it->getType() == "[");
    CHECK(it.get() == json.value().getChild(0));

    // Postfix increment and decrement return the previous node.
    CHECK((*it++).getType() == "[");
    CHECK(it->getType() == "number");
    CHECK((*it--).getType() == "number");
    CHECK(it->getType() == "[");

    ++it;
    ++it;
    ++it;
    ++it;
    CHECK(it->getType() == "]");
    ++it;
    CHECK(it == children.end());
    CHECK(children.end() == it);

    // Decrementing the end iterator returns to the last child.
    --it;
    CHECK(it != children.end());
    CHECK(it->getType() == "]");

    // Iterators over the same node compare equal; moving keeps the position.
    ts::ChildIterator first  = children.begin();
    ts::ChildIterator second = children.begin();
    CHECK(first == second);
    ++second;
    CHECK(first != second);
    ts::ChildIterator moved(std::move(second));
    CHECK(moved->getType() == "number");
}

TEST_CASE("Named ChildIterator skips anonymous children in both directions", "[iterators][children]")
{
    ts_test::JsonFixture json("[1, true, null]");

    ts::ChildIterator it(json.value(), ts::ChildScope::NamedOnly);
    CHECK(json.text(*it) == "1");
    ++it;
    CHECK(json.text(*it) == "true");
    --it;
    CHECK(json.text(*it) == "1");
}

TEST_CASE("Pre-order walk", "[iterators][preorder]")
{
    ts_test::JsonFixture json("[{\"a\": 1}, 2]");

    SECTION("Walks every node in document order")
    {
        std::vector<std::string> walked;
        for (ts::Node node : ts::preorder(json.root()))
        {
            walked.push_back(std::string(node.getType()));
        }
        CHECK(walked.size() == json.root().getDescendantsCount());
        CHECK(walked.front() == "document");
        CHECK(walked[1] == "array");
        CHECK(walked.back() == "]");
    }

    SECTION("Stays inside the subtree of the start node")
    {
        std::uint32_t inside_object = 0;
        for (ts::Node node : ts::preorder(json.value().getNamedChild(0)))
        {
            static_cast<void>(node);
            ++inside_object;
        }
        CHECK(inside_object == json.value().getNamedChild(0).getDescendantsCount());
    }

    SECTION("Null root")
    {
        std::uint32_t null_walk = 0;
        for (ts::Node node : ts::preorder(ts::Node::null()))
        {
            static_cast<void>(node);
            ++null_walk;
        }
        CHECK(null_walk == 0u);
    }

    SECTION("Skips children and reports the depth")
    {
        std::string            walked;
        std::uint32_t          deepest = 0;
        const ts::PreorderView view    = ts::preorder(json.root());
        for (ts::PreorderIterator it = view.begin(); it != view.end(); ++it)
        {
            walked += std::string((*it).getType()) + " ";
            deepest = it.depth() > deepest ? it.depth() : deepest;
            if ((*it).getType() == "object")
            {
                it.skipChildren();
            }
        }
        CHECK(walked == "document array [ object , number ] ");
        CHECK(deepest == 2u);
    }
}

TEST_CASE("Lookahead iterator", "[iterators][lookahead]")
{
    ts_test::JsonFixture json("[1]");
    const ts::StateID    state = json.value().getChild(0).getParseState();

    SECTION("Iteration, exhaustion and reset")
    {
        ts::LookaheadIterator iterator = json.language.getView().getLookaheadIterator(state);
        CHECK(static_cast<const TSLanguage *>(iterator.getLanguage())
              == static_cast<const TSLanguage *>(json.language));
        CHECK(static_cast<const TSLookaheadIterator *>(iterator) != nullptr);

        // A new iterator is not positioned on a symbol until next() is called.
        CHECK(iterator.getCurrentSymbolName().empty());

        std::vector<std::string> symbols;
        while (iterator.next())
        {
            symbols.push_back(std::string(iterator.getCurrentSymbolName()));
            CHECK_FALSE(iterator.getCurrentSymbolName().empty());
            CHECK(json.language.getView().getSymbolName(iterator.getCurrentSymbol())
                  == iterator.getCurrentSymbolName());
        }
        CHECK_FALSE(symbols.empty());
        // Exhaustion is sticky since tree-sitter 0.27.
        CHECK_FALSE(iterator.next());

        // Resetting starts over.
        REQUIRE(iterator.resetState(state));
        std::size_t again = 0;
        while (iterator.next())
        {
            ++again;
        }
        CHECK(again == symbols.size());
        CHECK(iterator.reset(json.language, state));
    }

    SECTION("Constructor and move")
    {
        ts::LookaheadIterator constructed(json.language, state);
        CHECK(constructed.next());

        ts::LookaheadIterator moved(std::move(constructed));
        CHECK(static_cast<const TSLookaheadIterator *>(moved) != nullptr);
        CHECK(static_cast<const TSLookaheadIterator *>(constructed) == nullptr);
    }

    SECTION("Invalid language or state")
    {
        CHECK_THROWS_AS(ts::LookaheadIterator(json.language, 0xFFFF), std::runtime_error);
        CHECK_THROWS_AS(ts::LookaheadIterator(ts::LanguageView(), 1), std::runtime_error);
    }
}
