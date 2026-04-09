#include "pch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cpp-tree-sitter.hpp>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("Language Metadata and Versioning", "[language]")
{
    ts::Language lang = tree_sitter_json();

    SECTION("Basic Properties")
    {
        CHECK(lang.getSymbolsCount() > 0);
        CHECK(lang.getVersion() > 0);
        // CHECK(lang.getName().compare("json") == 0); This grammar don't have name
        CHECK(lang.getName().compare("") == 0);
    }

    SECTION("Symbol Resolution")
    {
        ts::Symbol sym = lang.getSymbolForName("object", true);
        CHECK(sym != 0);
        CHECK(lang.getSymbolName(sym).compare("object") == 0);
        CHECK(lang.getSymbolType(sym) == ts::SymbolType::TypeRegular);
    }
}

TEST_CASE("Memory Management - Copying", "[lifecycle]")
{
    ts::Language lang = tree_sitter_json();
    {
        ts::Language lang_copy = lang; // Copy constructor test (ts_language_copy)
        CHECK(lang_copy.getSymbolsCount() == lang.getSymbolsCount());

        ts::Language lang_moved = std::move(lang_copy); // Move test
        CHECK(lang_moved.getSymbolsCount() > 0);
    }
    CHECK(lang.getSymbolsCount() > 0);
}

TEST_CASE("Language Deep Dive", "[language]")
{
    ts::Language lang = tree_sitter_json();

    SECTION("Count Accessors")
    {
        CHECK(lang.getSymbolsCount() > 0);
        CHECK(lang.getStatesCount() > 0);
        CHECK(lang.getFieldsCount() > 0);
    }

    SECTION("Field Resolution")
    {
        ts::FieldID key_id = lang.getFieldIDForName("key");
        CHECK(key_id > 0);

        auto field_name = lang.getFieldNameForID(key_id);
        CHECK(field_name.compare("key") == 0);

        CHECK(lang.getFieldIDForName("non_existent_field") == 0);
        CHECK(lang.getFieldNameForID(0).compare("") == 0);
    }

    SECTION("Type Hierarchy")
    {
        auto super_types = lang.getAllSuperTypes();

        if (!super_types.empty())
        {
            ts::Symbol first_super = super_types[0];
            auto       sub_types   = lang.getAllSubTypesForSuperType(first_super);
            CHECK_FALSE(sub_types.empty());
        }
    }

    SECTION("State Navigation and Lookahead")
    {
        ts::StateID initial_state = 0;
        ts::Symbol  document_sym  = lang.getSymbolForName("document", true);

        ts::StateID next_state = lang.getNextState(initial_state, document_sym);

        ts::LookaheadIterator li = lang.getLookaheadIterator(initial_state);
        CHECK(li.getLanguage().operator const TSLanguage *() == lang.operator const TSLanguage *());
    }

    SECTION("Metadata and Wasm")
    {
#if TEST_HAS_CXX17
        auto meta = lang.getMetadata();
        if (meta)
        {
            CHECK((meta->major_version >= 0));
        }
#else
        ts::LanguageMetadata meta = lang.getMetadata();
#endif

        CHECK_FALSE(lang.isWasm());
    }
}

TEST_CASE("Language Operators and Assignments", "[language][lifecycle]")
{
    ts::Language lang1 = tree_sitter_json();
    ts::Language lang2 = tree_sitter_json();

    SECTION("Assignment operators")
    {
        ts::Language lang3 = lang1; // Copy
        CHECK(lang3.getSymbolsCount() == lang1.getSymbolsCount());

        ts::Language lang4(nullptr); // Null Object
        lang4 = lang1;               // Swap
        CHECK(lang4.getSymbolsCount() == lang1.getSymbolsCount());

        ts::Language lang5 = std::move(lang1);
        CHECK(lang5.getSymbolsCount() > 0);
    }

    SECTION("C-API Interop")
    {
        const TSLanguage *raw = lang2;
        CHECK(raw != nullptr);
        CHECK(ts_language_symbol_count(raw) == lang2.getSymbolsCount());
    }
}
