// Error reporting: ts::Error / ts::ErrorKind (every standard) and, when std::expected is available (C++23), the
// non-throwing create / try* variants. Every failure is checked against its throwing counterpart: same kind of
// failure, same message. The exceptions of the throwing API are tested in the file of each class.

#include "common.hpp"

// STL
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#if CPP_TS_HAS_EXPECTED
namespace
{
    // Depth of the tree counting only named nodes.
    struct NamedDepth
    {
        int onNode(ts::Node node, ts::Span<int> children) const
        {
            int deepest = 0;
            for (int depth : children)
            {
                deepest = depth > deepest ? depth : deepest;
            }
            return node.isNamed() ? deepest + 1 : deepest;
        }
    };
} // namespace
#endif // CPP_TS_HAS_EXPECTED

TEST_CASE("Error stores the kind, the message and the offset", "[errors]")
{
    const ts::Error error(ts::ErrorKind::QuerySyntax, "Tree-sitter: message", 7);
    CHECK(error.kind == ts::ErrorKind::QuerySyntax);
    CHECK(error.message == "Tree-sitter: message");
    CHECK(error.offset == 7u);
    CHECK(std::string(error.what()) == error.message);

    CHECK(ts::Error(ts::ErrorKind::Unknown, "x").offset == 0u);
}

TEST_CASE("Every ErrorKind has a distinct name", "[errors]")
{
    std::set<std::string> names;
    const int             last = static_cast<int>(ts::ErrorKind::Unknown);
    for (int kind = 0; kind <= last; ++kind)
    {
        const std::string name = ts::getErrorKindName(static_cast<ts::ErrorKind>(kind));
        CHECK_FALSE(name.empty());
        names.insert(name);
    }
    CHECK(names.size() == static_cast<std::size_t>(last + 1));
    CHECK(std::string(ts::getErrorKindName(ts::ErrorKind::QuerySyntax)) == "QuerySyntax");
    CHECK(std::string(ts::getErrorKindName(ts::ErrorKind::WasmStoreRequired)) == "WasmStoreRequired");
    CHECK(std::string(ts::getErrorKindName(ts::ErrorKind::OldTreeLanguage)) == "OldTreeLanguage");
    CHECK(std::string(ts::getErrorKindName(ts::ErrorKind::Unknown)) == "Unknown");
}

#if CPP_TS_HAS_EXPECTED

TEST_CASE("Edits with std::expected", "[errors][expected][edit]")
{
    const ts::InputEdit insertion(2, 2, 7, ts::Point(0, 2), ts::Point(0, 2), ts::Point(0, 7));
    const ts::InputEdit backwards(4, 2, 6, ts::Point(0, 4), ts::Point(0, 2), ts::Point(0, 6));

    SECTION("Point")
    {
        ts::Point     point(0, 10);
        std::uint32_t byte = 10;

        const std::expected<void, ts::Error> rejected = point.tryEdit(backwards, byte);
        REQUIRE_FALSE(rejected.has_value());
        CHECK(rejected.error().kind == ts::ErrorKind::InvalidEdit);
        CHECK(point == ts::Point(0, 10));
        CHECK(rejected.error().message == ts_test::thrownMessage([&] { point.edit(backwards, byte); }));

        REQUIRE(point.tryEdit(insertion, byte).has_value());
        CHECK(point == ts::Point(0, 15));
        CHECK(byte == 15u);
    }

    SECTION("Range")
    {
        ts::Range range(ts::Extent<ts::Point>(ts::Point(0, 10), ts::Point(0, 20)), ts::Extent<std::uint32_t>(10, 20));
        CHECK(range.tryEdit(backwards).error().kind == ts::ErrorKind::InvalidEdit);
        REQUIRE(range.tryEdit(insertion).has_value());
        CHECK(range.byte == ts::Extent<std::uint32_t>(15, 25));
    }

    SECTION("Tree")
    {
        ts_test::JsonFixture json("[1, 2]");
        CHECK(json.tree.tryEdit(backwards).error().kind == ts::ErrorKind::InvalidEdit);
        CHECK(json.tree.tryEdit(insertion).has_value());

        ts::Tree invalid(nullptr);
        CHECK(invalid.tryEdit(insertion).error().kind == ts::ErrorKind::InvalidObject);
        CHECK(invalid.tryEdit(insertion).error().message == ts_test::thrownMessage([&] { invalid.edit(insertion); }));
    }

    SECTION("Node")
    {
        ts::Node null_node = ts::Node::null();
        CHECK(null_node.tryEdit(insertion).error().kind == ts::ErrorKind::InvalidObject);

        ts_test::JsonFixture json("[1, 2]");
        ts::Node             node = json.value();
        CHECK(node.tryEdit(backwards).error().kind == ts::ErrorKind::InvalidEdit);
        CHECK(node.tryEdit(insertion).has_value());
    }
}

TEST_CASE("Parser::create and trySetLanguage", "[errors][expected][parser]")
{
    const ts::Language language(tree_sitter_json());

    SECTION("create()")
    {
        std::expected<ts::Parser, ts::Error> parser = ts::Parser::create(language);
        REQUIRE(parser.has_value());
        CHECK(parser->hasLanguage());
        CHECK_FALSE(parser->parseString("{}").hasError());
    }

    SECTION("create() with an invalid language")
    {
        const std::expected<ts::Parser, ts::Error> invalid = ts::Parser::create(ts::LanguageView());
        REQUIRE_FALSE(invalid.has_value());
        CHECK(invalid.error().kind == ts::ErrorKind::InvalidLanguage);
        CHECK(ts_test::contains(invalid.error().message, "invalid"));
        // The error carries the same message as the exception of the throwing constructor.
        CHECK_THROWS_WITH(ts::Parser(ts::LanguageView()), invalid.error().message);
    }

    SECTION("trySetLanguage")
    {
        ts::Parser parser;
        CHECK(parser.trySetLanguage(language).has_value());
        CHECK(parser.hasLanguage());

        // An invalid view clears the language, which is not an error.
        CHECK(parser.trySetLanguage(ts::LanguageView()).has_value());
        CHECK_FALSE(parser.hasLanguage());
    }
}

TEST_CASE("Parser try* parse methods", "[errors][expected][parser]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser;

    SECTION("Without a language")
    {
        const std::expected<ts::Tree, ts::Error> no_language = parser.tryParseString("[1]");
        REQUIRE_FALSE(no_language.has_value());
        CHECK(no_language.error().kind == ts::ErrorKind::NoLanguage);
        CHECK_THROWS_WITH(parser.parseString("[1]"), no_language.error().message);
        CHECK(parser.tryParseStringEncoded("[1]", ts::InputEncoding::UTF8).error().kind == ts::ErrorKind::NoLanguage);
    }

    SECTION("Strings and Inputs")
    {
        REQUIRE(parser.setLanguage(language));
        std::expected<ts::Tree, ts::Error> tree = parser.tryParseString("[1, 2]");
        REQUIRE(tree.has_value());
        CHECK(tree->getRootNode().getNamedChild(0).getNamedChildCount() == 2u);

        CHECK(parser.tryParseStringEncoded("[1]", ts::InputEncoding::UTF8, *tree).has_value());

        const std::string                  source = "[true]";
        ts::Input                          input(ts_test::ChunkedReader{ &source, 2 }, ts::InputEncoding::UTF8);
        std::expected<ts::Tree, ts::Error> parsed = parser.tryParse(input);
        REQUIRE(parsed.has_value());
        CHECK_FALSE(parsed->hasError());
    }

    SECTION("Cancelled parse")
    {
        // parse() returns an invalid Tree, tryParse() reports ParseFailed.
        REQUIRE(parser.setLanguage(language));
        const std::string      source = ts_test::bigJsonArray(5000);
        ts::Input              input(ts_test::ChunkedReader{ &source, 64 }, ts::InputEncoding::UTF8);
        const ts::ParseOptions cancel([](ts::ParseState *) { return true; });

        CHECK_FALSE(parser.parse(input, nullptr, cancel).isValid());
        const std::expected<ts::Tree, ts::Error> cancelled = parser.tryParse(input, nullptr, cancel);
        REQUIRE_FALSE(cancelled.has_value());
        CHECK(cancelled.error().kind == ts::ErrorKind::ParseFailed);
    }

#if SIZE_MAX > UINT32_MAX
    SECTION("Inputs longer than 4GB")
    {
        REQUIRE(parser.setLanguage(language));
        const char           *fake_str      = "fake";
        constexpr std::size_t overflow_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1;
        const ts::StringView  huge_view(fake_str, overflow_size);

        CHECK(parser.tryParseString(huge_view).error().kind == ts::ErrorKind::InputTooLarge);
        CHECK(parser.tryParseStringEncoded(huge_view, ts::InputEncoding::UTF8).error().kind
              == ts::ErrorKind::InputTooLarge);
    }
#endif // SIZE_MAX > UINT32_MAX

    SECTION("Exceptions of user callbacks are rethrown, not turned into errors")
    {
        REQUIRE(parser.setLanguage(language));
        ts::Input input([](std::uint32_t, ts::Point, std::uint32_t *) -> ts::details::StringViewReturn
                        { throw std::runtime_error("read failed"); },
                        ts::InputEncoding::UTF8);
        CHECK_THROWS_WITH(parser.tryParse(input), "read failed");

        parser.setLogger([](ts::LogType, const char *) { throw std::runtime_error("logger failed"); });
        CHECK_THROWS_WITH(parser.tryParseString("[1]"), "logger failed");
    }

#if !defined(_WIN32)
    // The MSVC runtime treats _dup of a closed descriptor as an invalid parameter (debug assertion), so POSIX only.
    SECTION("DOT graphs to an invalid file descriptor")
    {
        const std::expected<void, ts::Error> result = parser.tryEnableDotGraphs(987654);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().kind == ts::ErrorKind::SystemError);
        CHECK(parser.tryEnableDotGraphs(-1).has_value());
    }
#endif // !defined(_WIN32)
}

TEST_CASE("Query::create and QueryCursor::tryExec", "[errors][expected][query]")
{
    const ts::Language language(tree_sitter_json());

    SECTION("create()")
    {
        std::expected<ts::Query, ts::Error> query = ts::Query::create(language, "(number) @n");
        REQUIRE(query.has_value());
        CHECK(query->getCaptureCount() == 1u);
        CHECK(query->getCaptureNameForID(0) == "n");
    }

    SECTION("Compilation errors carry the kind and the offset")
    {
        const std::string                         source    = "(number) (no_such_node)";
        const std::expected<ts::Query, ts::Error> node_type = ts::Query::create(language, source);
        REQUIRE_FALSE(node_type.has_value());
        CHECK(node_type.error().kind == ts::ErrorKind::QueryNodeType);
        CHECK(node_type.error().offset == 10u);
        CHECK(ts_test::contains(node_type.error().message, "Node"));
        CHECK_THROWS_WITH(ts::Query(language, source), node_type.error().message);

        const std::expected<ts::Query, ts::Error> syntax = ts::Query::create(language, "((number) @num");
        REQUIRE_FALSE(syntax.has_value());
        CHECK(syntax.error().kind == ts::ErrorKind::QuerySyntax);
        CHECK(syntax.error().offset <= 14u);
        CHECK(ts_test::contains(syntax.error().message, "Syntax"));

        CHECK(ts::Query::create(language, "(pair nope: (_))").error().kind == ts::ErrorKind::QueryField);
        CHECK(ts::Query::create(ts::LanguageView(), "(number)").error().kind == ts::ErrorKind::InvalidLanguage);
    }

    SECTION("tryExec()")
    {
        ts_test::JsonFixture json("[1, 2]");
        ts::Query            query(language, "(number) @n");
        ts::QueryCursor      cursor;
        CHECK(cursor.tryExec(query, json.root()).has_value());

        const ts::Query moved = std::move(query);
        CHECK(cursor.tryExec(query, json.root()).error().kind == ts::ErrorKind::InvalidArgument);
        CHECK(cursor.tryExec(query, json.root()).error().message
              == ts_test::thrownMessage([&] { cursor.exec(query, json.root()); }));
        CHECK(cursor.tryExec(moved, json.root()).has_value());
    }
}

TEST_CASE("LookaheadIterator::create and tryGetLookaheadIterator", "[errors][expected][language]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    CHECK(view.tryGetLookaheadIterator(1).has_value());
    CHECK(ts::LookaheadIterator::create(view, 1).has_value());

    const ts::StateID past_last = static_cast<ts::StateID>(view.getStatesCount());
    CHECK(view.tryGetLookaheadIterator(past_last).error().kind == ts::ErrorKind::InvalidParseState);
    CHECK(ts::LanguageView().tryGetLookaheadIterator(1).error().kind == ts::ErrorKind::InvalidObject);
    CHECK(ts::LookaheadIterator::create(ts::LanguageView(), 1).error().kind == ts::ErrorKind::InvalidLanguage);
}

TEST_CASE("tryFold", "[errors][expected][visitor]")
{
    ts_test::JsonFixture json("[[1, [2]], 3]");

    const std::expected<int, ts::Error> depth = ts::tryFold<int>(json.root(), NamedDepth());
    REQUIRE(depth.has_value());
    CHECK(*depth == 5);

    const std::expected<int, ts::Error> null_root = ts::tryFold<int>(ts::Node::null(), NamedDepth());
    REQUIRE_FALSE(null_root.has_value());
    CHECK(null_root.error().kind == ts::ErrorKind::InvalidArgument);
}

#if defined(CPP_TS_FEATURE_WASM)
TEST_CASE("WasmStore and Wasm parsers with std::expected", "[errors][expected][wasm]")
{
    ts_test::Engine engine;

    std::expected<ts::WasmStore, ts::Error> store = ts::WasmStore::create(engine.get());
    REQUIRE(store.has_value());
    CHECK(store->tryGetLanguageCount().value() == 0u);

    std::expected<ts::Language, ts::Error> language = store->tryLoadLanguage(ts_test::jsonWasmPath());
    REQUIRE(language.has_value());
    CHECK(language->getView().isWasm());
    CHECK(store->tryGetLanguageCount().value() == 1u);

    SECTION("Loading errors")
    {
        const std::expected<ts::Language, ts::Error> missing = store->tryLoadLanguage("does-not-exist.wasm");
        REQUIRE_FALSE(missing.has_value());
        CHECK(missing.error().kind == ts::ErrorKind::FileError);
        CHECK(ts_test::contains(missing.error().message, "Failed to open"));

        const std::expected<ts::Language, ts::Error> garbage = store->tryLoadLanguage("json", "not a wasm module");
        REQUIRE_FALSE(garbage.has_value());
        CHECK(garbage.error().kind != ts::ErrorKind::FileError);
        CHECK_THROWS_WITH(store->loadLanguage("json", "not a wasm module"), garbage.error().message);
    }

    SECTION("Parsers")
    {
        // Without a store tree-sitter rejects the Wasm language.
        CHECK(ts::Parser::create(*language).error().kind == ts::ErrorKind::WasmStoreRequired);
        ts::Parser without_store;
        CHECK(without_store.trySetLanguage(*language).error().kind == ts::ErrorKind::WasmStoreRequired);
        CHECK(ts_test::contains(without_store.trySetLanguage(*language).error().message, "WasmStore"));

        // An invalid language does not take the store.
        CHECK(ts::Parser::create(ts::LanguageView(), *store).error().kind == ts::ErrorKind::InvalidLanguage);
        REQUIRE(store->isValid());

        std::expected<ts::Parser, ts::Error> parser = ts::Parser::create(*language, *store);
        REQUIRE(parser.has_value());
        CHECK(parser->hasWasmStore());
        CHECK(parser->tryParseString("[1]").has_value());

        // An old tree of another language (the native JSON grammar).
        ts_test::JsonFixture native("[1]");
        CHECK(parser->tryParseString("[1, 2]", native.tree).error().kind == ts::ErrorKind::OldTreeLanguage);

        // The store now belongs to the parser.
        CHECK(store->tryLoadLanguage(ts_test::jsonWasmPath()).error().kind == ts::ErrorKind::InvalidObject);
        CHECK(store->tryGetLanguageCount().error().kind == ts::ErrorKind::InvalidObject);
    }
}
#endif // defined(CPP_TS_FEATURE_WASM)

#endif // CPP_TS_HAS_EXPECTED
