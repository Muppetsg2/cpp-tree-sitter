#include "pch.hpp"

#if defined(CPP_TS_TEST_FEATURE_WASM)

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// wasmtime
#include <wasmtime.hh>

// Catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// STL
#include <cstdint>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

extern "C" const TSLanguage *tree_sitter_json();

using namespace wasmtime;

std::vector<uint8_t> load_wasm_file(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open wasm file: " + path);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char *>(buffer.data()), size);
    return buffer;
}

TEST_CASE("Wasm Loading with Wasmtime C++ API", "[wasm]")
{
    // C++ Api Wasm Engine
    Engine engine;

    SECTION("Successful language loading and parsing")
    {
        ts::WasmStore store(engine.capi());

        // Loading compilated tree-spitter-json grammar
        auto wasm_data = load_wasm_file(CPP_TS_TEST_WASM_FILES_DIR "/tree-sitter-json.wasm");

        ts::Language json_lang{ nullptr };
        REQUIRE_NOTHROW(json_lang = store.loadLanguage(
                                "json",
                                ts::details::StringViewParameter(reinterpret_cast<char *>(wasm_data.data()),
                                                                 wasm_data.size())));

        // Loaded language tests
        REQUIRE(json_lang.isWasm());
        REQUIRE(store.getLanguageCount() == 1);

        // Parser test using wasm
        ts::Parser parser;

        // TODO: repair bug with ts_wasm_store_delete
        // parser.setWasmStore(store);

        /*
        parser.setLanguage(json_lang);

        std::string code = R"({"test": 123})";
        ts::Tree    tree = parser.parseString(code);

        REQUIRE_FALSE(tree.hasError());
        REQUIRE(tree.getRootNode().getType().compare("document") == 0);

        store = parser.takeWasmStore();
        parser.setLanguage({ nullptr });
        */
    }

    SECTION("Error handling for invalid Wasm data")
    {
        ts::WasmStore store(engine.capi());

        // Plain text. Not a binary wasm
        std::string garbage = "not a wasm file";

        // WasmErrorHelper::validate error
        REQUIRE_THROWS_AS(store.loadLanguage("invalid", garbage), std::runtime_error);
    }
}

TEST_CASE("WasmStore Lifecycle", "[wasm]")
{
    Engine engine;

    SECTION("RAII Safety")
    {
        auto *raw_engine = engine.capi();
        {
            ts::WasmStore store(raw_engine);
            // WasmStore Live here
        }
        SUCCEED("WasmStore destroyed without crashing");
    }
}

#endif
