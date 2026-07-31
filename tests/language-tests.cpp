#include "pch.hpp"

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// Catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// STL
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#if CPP_TS_TEST_HAS_CXX17
#include <string_view>
#endif // CPP_TS_TEST_HAS_CXX17

extern "C" const TSLanguage *tree_sitter_json();

// LanguageView is meant to be a cheap, non-owning handle (equivalent to a raw
// `const TSLanguage*`). If either of these ever fails, something in the
// class layout silently started doing more than "hold a pointer".
static_assert(std::is_trivially_copyable<ts::LanguageView>::value,
              "LanguageView should stay trivially copyable (non-owning pointer wrapper)");
static_assert(sizeof(ts::LanguageView) == sizeof(const TSLanguage *),
              "LanguageView should not carry more state than a raw pointer");

TEST_CASE("Language Metadata and Versioning", "[language][languageview]")
{
    ts::Language     lang(tree_sitter_json());
    ts::LanguageView view = lang;

    SECTION("Basic Properties")
    {
        CHECK(view.getSymbolsCount() > 0);
        CHECK(view.getVersion() > 0);
        // CHECK(view.getName().compare("json") == 0); This grammar don't have name
        CHECK(view.getName().compare("") == 0);
    }

    SECTION("Symbol Resolution")
    {
        ts::Symbol sym = view.getSymbolForName("object", true);
        CHECK(sym != 0);
        CHECK(view.getSymbolName(sym).compare("object") == 0);
        CHECK(view.getSymbolType(sym) == ts::SymbolType::TypeRegular);
    }
}

TEST_CASE("Memory Management - Copying", "[lifecycle][language]")
{
    ts::Language     lang(tree_sitter_json());
    ts::LanguageView view = lang;
    {
        ts::Language lang_copy = lang; // Copy constructor test (ts_language_copy)
        CHECK(lang_copy.isValid());
        CHECK(lang_copy.getView().getSymbolsCount() == view.getSymbolsCount());

        ts::Language lang_moved = std::move(lang_copy); // Move test
        CHECK(lang_moved.isValid());
        CHECK(lang_moved.getView().getSymbolsCount() > 0);
    }
    CHECK(lang.isValid());
    CHECK(view.getSymbolsCount() > 0);
}

TEST_CASE("Language Deep Dive", "[language][languageview]")
{
    ts::Language     lang(tree_sitter_json());
    ts::LanguageView view = lang;

    SECTION("Count Accessors")
    {
        CHECK(view.getSymbolsCount() > 0);
        CHECK(view.getStatesCount() > 0);
        CHECK(view.getFieldsCount() > 0);
    }

    SECTION("Field Resolution")
    {
        ts::FieldID key_id = view.getFieldIDForName("key");
        CHECK(key_id > 0);

        ts::StringView field_name = view.getFieldNameForID(key_id);
        CHECK(field_name.compare("key") == 0);

        CHECK(view.getFieldIDForName("non_existent_field") == 0);
        CHECK(view.getFieldNameForID(0).compare("") == 0);

        std::vector<ts::StringView> fields = view.getAllFieldsNames();
        for (const ts::StringView &field : fields)
        {
            CHECK(field.compare("") != 0);
        }
    }

    SECTION("Type Hierarchy")
    {
        std::vector<ts::Symbol> super_types = view.getAllSuperTypes();

        if (!super_types.empty())
        {
            ts::Symbol              first_super = super_types[0];
            std::vector<ts::Symbol> sub_types   = view.getAllSubTypesForSuperType(first_super);
            CHECK_FALSE(sub_types.empty());
        }
    }

    SECTION("State Navigation and Lookahead")
    {
        ts::StateID initial_state = 0;
        ts::Symbol  document_sym  = view.getSymbolForName("document", true);

#if CPP_TS_TEST_HAS_CXX17
        [[maybe_unused]]
#endif // CPP_TS_TEST_HAS_CXX17
        ts::StateID next_state = view.getNextState(initial_state, document_sym);

#if !CPP_TS_TEST_HAS_CXX17
        static_cast<void>(next_state);
#endif // !CPP_TS_TEST_HAS_CXX17

        ts::LookaheadIterator li = view.getLookaheadIterator(initial_state);
        CHECK(li.getLanguage().operator const TSLanguage *() == view.operator const TSLanguage *());
    }

    SECTION("Metadata")
    {
#if CPP_TS_TEST_HAS_CXX17
        std::optional<ts::LanguageMetadata> meta = view.getMetadata();
        if (meta)
        {
            CHECK(meta->major_version >= 0);
        }
#else
        ts::LanguageMetadata meta = view.getMetadata();
        CHECK(meta.major_version >= 0);
#endif // CPP_TS_TEST_HAS_CXX17

#if defined(CPP_TS_TEST_FEATURE_WASM)
        CHECK_FALSE(view.isWasm());
#endif // defined(CPP_TS_TEST_FEATURE_WASM)
    }
}

TEST_CASE("Language Operators and Assignments", "[language][languageview][lifecycle]")
{
    ts::Language lang1(tree_sitter_json());
    ts::Language lang2(tree_sitter_json());

    SECTION("Assignment operators")
    {
        ts::Language lang3 = lang1; // Copy
        CHECK(lang3.getView().getSymbolsCount() == lang1.getView().getSymbolsCount());

        ts::Language lang4(nullptr); // Null Object
        CHECK_FALSE(lang4.isValid());

        lang4 = lang1; // Copy-assignment (deep copy, not a swap)
        CHECK(lang4.isValid());
        CHECK(lang4.getView().getSymbolsCount() == lang1.getView().getSymbolsCount());

        ts::Language lang5 = std::move(lang1);
        CHECK(lang5.isValid());
        CHECK(lang5.getView().getSymbolsCount() > 0);
    }

    SECTION("C-API Interop")
    {
        const TSLanguage *raw = lang2;
        CHECK(raw != nullptr);
        CHECK(ts_language_symbol_count(raw) == lang2.getView().getSymbolsCount());
    }

    SECTION("Conversion to LanguageView")
    {
        ts::LanguageView view = lang2; // implicit Language -> LanguageView conversion
        CHECK(view.isValid());
        CHECK(view.operator const TSLanguage *() == lang2.operator const TSLanguage *());
    }
}

TEST_CASE("Language Invalid State Handling", "[language][languageview][error_handling]")
{
    // Language constructed with nullptr
    ts::Language     invalid_lang(nullptr);
    ts::LanguageView invalid_view = invalid_lang;

    SECTION("Default construction is also invalid")
    {
        ts::Language     default_lang;
        ts::LanguageView default_view;

        CHECK_FALSE(default_lang.isValid());
        CHECK_FALSE(default_view.isValid());
        CHECK(default_lang.operator const TSLanguage *() == nullptr);
        CHECK(default_view.operator const TSLanguage *() == nullptr);
    }

    SECTION("Validation Flags")
    {
        CHECK_FALSE(invalid_lang.isValid());
        CHECK(invalid_lang.operator const TSLanguage *() == nullptr);
        CHECK_FALSE(invalid_view.isValid());
        CHECK(invalid_view.operator const TSLanguage *() == nullptr);
    }

    SECTION("Count Accessors on Invalid State")
    {
        CHECK(invalid_view.getSymbolsCount() == 0);
        CHECK(invalid_view.getStatesCount() == 0);
        CHECK(invalid_view.getFieldsCount() == 0);
    }

    SECTION("Resolutions on Invalid State")
    {
        // Symbol Resolution
        CHECK(invalid_view.getSymbolName(1).empty());
        CHECK(invalid_view.getSymbolType(1) == ts::SymbolType::TypeRegular);
        CHECK(invalid_view.getSymbolForName("test", true) == 0);

        // Field Resolution
        CHECK(invalid_view.getFieldNameForID(1).empty());
        CHECK(invalid_view.getFieldIDForName("test") == 0);
    }

    SECTION("Type Hierarchy on Invalid State")
    {
        CHECK(invalid_view.getAllSuperTypes().empty());
        CHECK(invalid_view.getAllSubTypesForSuperType(1).empty());
    }

    SECTION("Navigation and Exceptions on Invalid State")
    {
        CHECK(invalid_view.getNextState(0, 0) == 0);
        CHECK_THROWS_AS(invalid_view.getLookaheadIterator(0), std::logic_error);
    }

    SECTION("Metadata on Invalid State")
    {
        CHECK(invalid_view.getName().empty());
        CHECK(invalid_view.getVersion() == 0);

#if CPP_TS_TEST_HAS_CXX17
        CHECK_FALSE(invalid_view.getMetadata().has_value());
#else
        ts::LanguageMetadata meta = invalid_view.getMetadata();
        CHECK(meta.major_version == 0);
        CHECK(meta.minor_version == 0);
        CHECK(meta.patch_version == 0);
#endif // CPP_TS_TEST_HAS_CXX17
    }
}

TEST_CASE("Language Edge Cases", "[language][languageview]")
{
    ts::Language     lang(tree_sitter_json());
    ts::LanguageView view = lang;

    SECTION("Self Assignment")
    {
        ts::Language *ptr_lang = &lang;
        lang                   = *ptr_lang;
        CHECK(lang.isValid());
        CHECK(lang.getView().getSymbolsCount() > 0);
    }

    SECTION("Invalid lookups on valid language")
    {
        // Checking the behavior for a symbol ID that is definitely not a SuperType (e.g., 9999).
        // We assume that the C API will correctly return null/0 and the wrapper will throw an empty vector.
        CHECK(view.getAllSubTypesForSuperType(9999).empty());
    }

#if SIZE_MAX > UINT32_MAX
    SECTION("Throw exception on size overflow")
    {
        // We bypass the real 4GB allocation by creating an artificial view, but with a given gigantic size.
        const char           *fake_str      = "fake";
        constexpr std::size_t overflow_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1;

        const ts::StringView huge_view(fake_str, overflow_size);

        CHECK_THROWS_AS(view.getSymbolForName(huge_view, true), std::length_error);
        CHECK_THROWS_AS(view.getFieldIDForName(huge_view), std::length_error);
    }
#endif // SIZE_MAX > UINT32_MAX
}
