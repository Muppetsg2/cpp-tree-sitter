#include "pch.hpp"

#if defined(CPP_TS_TEST_FEATURE_WASM)

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// wasmtime
#if TEST_HAS_CXX17
#include <wasmtime.hh>
#else
#include <wasm.h>
#endif

// Catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// STL
#include <stdexcept>
#include <string>

TEST_CASE("Wasm Loading with Wasmtime C++ API", "[wasm]")
{
#if TEST_HAS_CXX17
    // C++ Api Wasm Engine
    wasmtime::Engine engine;
#else
    // C Api Wasm Engine
    wasm_engine_t *engine = wasm_engine_new();
#endif

    SECTION("Successful language loading and parsing")
    {
        // Engine will be deleted by tree-sitter
#if TEST_HAS_CXX17
        ts::WasmStore store(engine.capi());
#else
        ts::WasmStore store(engine);
#endif

        // Loading compilated tree-spitter-json grammar
        ts::Language json_lang{ nullptr };
        REQUIRE_NOTHROW(json_lang = store.loadLanguage(CPP_TS_TEST_WASM_FILES_DIR "/tree-sitter-json.wasm"));

        // Loaded language tests
        REQUIRE(json_lang.isWasm());
        REQUIRE(store.getLanguageCount() == 1);

        // Parser test using wasm
        ts::Parser parser;

        parser.setWasmStore(store);

        CHECK_FALSE(store.isValid());

        parser.setLanguage(json_lang);

        std::string code = R"({"test": 123})";
        ts::Tree    tree = parser.parseString(code);

        REQUIRE_FALSE(tree.hasError());
        REQUIRE(tree.getRootNode().getType().compare("document") == 0);

        store = parser.takeWasmStore();

        CHECK(store.isValid());
        CHECK_FALSE(parser.hasLanguage());
    }

    SECTION("Error handling for invalid Wasm data")
    {
#if TEST_HAS_CXX17
        ts::WasmStore store(engine.capi());
#else
        ts::WasmStore store(engine);
#endif

        // Plain text. Not a binary wasm
        std::string garbage = "not a wasm file";

        // WasmErrorHelper::validate error
        REQUIRE_THROWS_AS(store.loadLanguage("invalid", garbage), std::runtime_error);
    }
}

TEST_CASE("WasmStore Lifecycle", "[wasm]")
{
#if TEST_HAS_CXX17
    // C++ Api Wasm Engine
    wasmtime::Engine engine;
#else
    // C Api Wasm Engine
    wasm_engine_t *raw_engine = wasm_engine_new();
#endif

    SECTION("RAII Safety")
    {
#if TEST_HAS_CXX17
        wasm_engine_t *raw_engine = engine.capi();
#endif
        {
            ts::WasmStore store(raw_engine);
            // WasmStore Live here
        }
        SUCCEED("WasmStore destroyed without crashing");
    }
}

TEST_CASE("Parser Wasm Exceptions and Guardrails", "[wasm][parser][exceptions]")
{
#if TEST_HAS_CXX17
    wasmtime::Engine engine;
    ts::WasmStore    store(engine.capi());
#else
    wasm_engine_t *engine = wasm_engine_new();
    ts::WasmStore  store(engine);
#endif

    ts::Language json_lang = store.loadLanguage(CPP_TS_TEST_WASM_FILES_DIR "/tree-sitter-json.wasm");

    SECTION("Setting Wasm language without WasmStore throws logic_error")
    {
        ts::Parser parser;

        CHECK_FALSE(parser.hasWasmStore());

        // The parser must reject a Wasm language if no WasmStore is provided
        CHECK_THROWS_AS(parser.setLanguage(json_lang), std::logic_error);
    }

    SECTION("Constructor throws on invalid language with WasmStore")
    {
        ts::Language invalid_lang(nullptr);

        // Constructor should validate the language before transferring the store
        CHECK_THROWS_AS(ts::Parser(invalid_lang, store), std::runtime_error);
    }
}

TEST_CASE("Parser Constructor and WasmStore Transfer", "[wasm][parser]")
{
#if TEST_HAS_CXX17
    wasmtime::Engine engine;
    ts::WasmStore    store(engine.capi());
#else
    wasm_engine_t *engine = wasm_engine_new();
    ts::WasmStore  store(engine);
#endif

    ts::Language json_lang = store.loadLanguage(CPP_TS_TEST_WASM_FILES_DIR "/tree-sitter-json.wasm");

    SECTION("Initialize Parser directly with WasmStore and Language")
    {
        // The constructor should take ownership of the store and set the language safely
        ts::Parser parser(json_lang, store);

        CHECK(parser.hasLanguage());
        CHECK(parser.hasWasmStore());

        // The original store object should be invalidated after the transfer
        CHECK_FALSE(store.isValid());

        // Verify that parsing works correctly using the transferred store
        std::string code = "[1, 2, 3]";
        ts::Tree    tree = parser.parseString(code);
        CHECK_FALSE(tree.hasError());
    }
}

TEST_CASE("Replacing WasmStore in Parser", "[wasm][parser]")
{
#if TEST_HAS_CXX17
    wasmtime::Engine engine1;
    wasmtime::Engine engine2;
    ts::WasmStore    store1(engine1.capi());
    ts::WasmStore    store2(engine2.capi());
#else
    wasm_engine_t *engine1 = wasm_engine_new();
    wasm_engine_t *engine2 = wasm_engine_new();
    ts::WasmStore  store1(engine1);
    ts::WasmStore  store2(engine2);
#endif

    ts::Parser parser;

    SECTION("Setting a new WasmStore safely discards the old one")
    {
        parser.setWasmStore(store1);
        CHECK(parser.hasWasmStore());
        CHECK_FALSE(store1.isValid());

        // Set a second store without taking the first one back.
        // The parser should safely delete the previous store internally.
        parser.setWasmStore(store2);
        CHECK(parser.hasWasmStore());
        CHECK_FALSE(store2.isValid());

        // Taking the store should now return the second one (which is valid)
        ts::WasmStore retrieved_store = parser.takeWasmStore();
        CHECK(retrieved_store.isValid());
        CHECK_FALSE(parser.hasWasmStore());
    }
}
#endif
