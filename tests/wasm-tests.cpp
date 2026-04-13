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
#include <cstdint>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

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

#endif
