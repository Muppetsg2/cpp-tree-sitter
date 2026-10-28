// WasmStore and Wasm languages (built only with CPP_TS_FEATURE_WASM).

#include "common.hpp"

#if defined(CPP_TS_FEATURE_WASM)

// STL
#include <stdexcept>
#include <string>
#include <utility>

TEST_CASE("WasmStore loads a language from a file", "[wasm]")
{
    ts_test::Engine engine;
    ts::WasmStore   store(engine.get());
    REQUIRE(store.isValid());
    CHECK(store.getLanguageCount() == 0u);

    ts::Language language;
    CHECK_FALSE(language.isValid());
    REQUIRE_NOTHROW(language = store.loadLanguage(ts_test::jsonWasmPath()));
    REQUIRE(language.isValid());
    CHECK(language.getView().isWasm());
    CHECK(store.getLanguageCount() == 1u);

    // The parser takes the store over.
    ts::Parser parser;
    parser.setWasmStore(store);
    CHECK_FALSE(store.isValid());
    CHECK(parser.hasWasmStore());
    CHECK(parser.setLanguage(language));

    const ts::Tree tree = parser.parseString("[1, {\"a\": null}]");
    CHECK_FALSE(tree.hasError());
    CHECK(tree.getRootNode().getType() == "document");
    CHECK(tree.getRootNode().getNamedChild(0).getNamedChildCount() == 2u);

    // Taking the store back removes the Wasm language from the parser.
    store = parser.takeWasmStore();
    CHECK(store.isValid());
    CHECK_FALSE(parser.hasWasmStore());
    CHECK_FALSE(parser.hasLanguage());
}

TEST_CASE("WasmStore loads a language from memory", "[wasm][regression]")
{
    ts_test::Engine   engine;
    ts::WasmStore     store(engine.get());
    const std::string buffer = ts_test::readFile(ts_test::jsonWasmPath());
    REQUIRE_FALSE(buffer.empty());

    // The view covers only "json" (tree-sitter looks up the `tree_sitter_json` export): the wrapper must pass a
    // null-terminated copy to the C API, otherwise "jsonXXXX" would be used and loading would fail.
    const std::string  names    = "jsonXXXX";
    const ts::Language language = store.loadLanguage(ts::StringView(names.data(), 4), buffer);
    REQUIRE(language.isValid());

    ts::Parser parser(language, store);
    CHECK_FALSE(parser.parseString("[1, 2]").hasError());
}

TEST_CASE("WasmStore errors", "[wasm][exceptions]")
{
    ts_test::Engine engine;
    ts::WasmStore   store(engine.get());

    SECTION("Invalid modules and files")
    {
        CHECK_THROWS_AS(store.loadLanguage("json", "not a wasm module"), std::runtime_error);
        CHECK_THROWS_AS(store.loadLanguage("does-not-exist.wasm"), std::runtime_error);
    }

    SECTION("A store taken over by a parser can no longer be used")
    {
        const ts::Language language = store.loadLanguage(ts_test::jsonWasmPath());
        ts::Parser         parser;
        parser.setWasmStore(store);
        CHECK_THROWS_AS(store.loadLanguage(ts_test::jsonWasmPath()), std::logic_error);
        CHECK_THROWS_AS(store.getLanguageCount(), std::logic_error);
    }

    SECTION("Null store")
    {
        const ts::WasmStore empty(static_cast<TSWasmStore *>(nullptr));
        CHECK_FALSE(empty.isValid());
    }
}

TEST_CASE("Parser with a Wasm language", "[wasm][parser]")
{
    ts_test::Engine    engine;
    ts::WasmStore      store(engine.get());
    const ts::Language language = store.loadLanguage(ts_test::jsonWasmPath());

    SECTION("The constructor takes the store over")
    {
        ts::Parser parser(language, store);
        CHECK(parser.hasLanguage());
        CHECK(parser.hasWasmStore());
        CHECK_FALSE(store.isValid());
        CHECK_FALSE(parser.parseString("[1, 2, 3]").hasError());
    }

    SECTION("A WasmStore is required")
    {
        ts::Parser parser(tree_sitter_json());
        REQUIRE(parser.hasLanguage());
        CHECK_FALSE(parser.hasWasmStore());

        // tree-sitter rejects the language and clears the previous one; hasLanguage() must follow tree-sitter.
        CHECK_THROWS_AS(parser.setLanguage(language), std::logic_error);
        CHECK_FALSE(parser.hasLanguage());
        CHECK_THROWS_AS(parser.parseString("[1]"), std::logic_error);

        // Moving a store keeps it usable.
        ts::WasmStore moved(std::move(store));
        CHECK(moved.isValid());
        CHECK_FALSE(store.isValid());
        parser.setWasmStore(moved);
        CHECK(parser.setLanguage(language));
        CHECK(parser.parseString("[1]").isValid());
    }

    SECTION("An invalid language is rejected before the parser takes the store over")
    {
        CHECK_THROWS_AS(ts::Parser(ts::LanguageView(), store), std::runtime_error);
        CHECK(store.isValid());
    }

    SECTION("Taking the store removes the Wasm language")
    {
        ts::Parser parser(language, store);
        REQUIRE(parser.hasLanguage());

        // tree-sitter clears a Wasm language when its store is taken; the wrapper does not keep a stale flag.
        ts::WasmStore taken = parser.takeWasmStore();
        CHECK_FALSE(parser.hasLanguage());
        CHECK_FALSE(parser.getCurrentLanguage().isValid());
        CHECK(taken.isValid());
    }
}

TEST_CASE("Replacing the WasmStore of a parser", "[wasm][parser]")
{
    ts_test::Engine first_engine;
    ts_test::Engine second_engine;
    ts::WasmStore   first(first_engine.get());
    ts::WasmStore   second(second_engine.get());
    ts::Parser      parser;

    parser.setWasmStore(first);
    CHECK(parser.hasWasmStore());
    CHECK_FALSE(first.isValid());

    // Setting a second store without taking the first one back releases the first one.
    parser.setWasmStore(second);
    CHECK(parser.hasWasmStore());
    CHECK_FALSE(second.isValid());

    ts::WasmStore taken = parser.takeWasmStore();
    CHECK(taken.isValid());
    CHECK_FALSE(parser.hasWasmStore());
}

TEST_CASE("Parser explains an old tree of another language rejected by tree-sitter", "[wasm][parser][edit]")
{
    ts_test::Engine    engine;
    ts::WasmStore      store(engine.get());
    const ts::Language wasm_json = store.loadLanguage(ts_test::jsonWasmPath());

    // The native and the Wasm JSON grammars are different languages.
    ts_test::JsonFixture native("[1]");
    ts::Parser           parser(wasm_json, store);
    REQUIRE(static_cast<const TSLanguage *>(parser.getCurrentLanguage())
            != static_cast<const TSLanguage *>(native.language));

    CHECK_THROWS_AS(parser.parseString("[1, 2]", native.tree), std::invalid_argument);

    // Without an old tree the parse works.
    CHECK(parser.parseString("[1, 2]").isValid());
}

#endif // defined(CPP_TS_FEATURE_WASM)
