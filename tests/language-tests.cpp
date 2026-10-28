// Language (owning) and LanguageView (non-owning) grammar handles.

#include "common.hpp"

// STL
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// LanguageView is meant to be a cheap, non-owning handle (equivalent to a raw `const TSLanguage*`). If either of these
// ever fails, something in the class layout silently started doing more than "hold a pointer".
static_assert(std::is_trivially_copyable<ts::LanguageView>::value,
              "LanguageView should stay trivially copyable (non-owning pointer wrapper)");
static_assert(sizeof(ts::LanguageView) == sizeof(const TSLanguage *),
              "LanguageView should not carry more state than a raw pointer");

TEST_CASE("Language ownership, copy and move", "[language][lifecycle]")
{
    SECTION("Invalid languages")
    {
        const ts::Language     default_language;
        const ts::Language     null_language(nullptr);
        const ts::LanguageView default_view;

        CHECK_FALSE(default_language.isValid());
        CHECK_FALSE(default_language.getView().isValid());
        CHECK_FALSE(null_language.isValid());
        CHECK_FALSE(default_view.isValid());
        CHECK(static_cast<const TSLanguage *>(default_language) == nullptr);
        CHECK(static_cast<const TSLanguage *>(default_view) == nullptr);
    }

    SECTION("Copy and move")
    {
        ts::Language language(tree_sitter_json());
        REQUIRE(language.isValid());

        // Copies are independent references (ts_language_copy), usable after the original is gone.
        ts::Language copy(language);
        CHECK(copy.isValid());
        CHECK(copy.getView().getSymbolsCount() == language.getView().getSymbolsCount());

        ts::Language assigned;
        assigned = copy;
        CHECK(assigned.isValid());
        CHECK(assigned.getView().getSymbolsCount() == language.getView().getSymbolsCount());

        // Self-assignment keeps the language.
        ts::Language *self = &assigned;
        assigned           = *self;
        CHECK(assigned.isValid());

        // Moves transfer the handle.
        const TSLanguage *raw = language;
        ts::Language      moved(std::move(language));
        CHECK(static_cast<const TSLanguage *>(moved) == raw);
        CHECK_FALSE(language.isValid());

        ts::Language move_assigned;
        move_assigned = std::move(moved);
        CHECK(static_cast<const TSLanguage *>(move_assigned) == raw);
        CHECK_FALSE(moved.isValid());

        // Language converts implicitly to LanguageView and to the C API pointer.
        const ts::LanguageView view = move_assigned;
        CHECK(static_cast<const TSLanguage *>(view) == raw);
        CHECK(ts_language_symbol_count(move_assigned) == view.getSymbolsCount());
    }
}

TEST_CASE("LanguageView counts, version and parseability", "[language][languageview]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    CHECK(view.isValid());
    CHECK(view.getSymbolsCount() > 0u);
    CHECK(view.getStatesCount() > 0u);
    CHECK(view.getFieldsCount() == 2u); // key, value
    CHECK(view.getVersion() == 14u);    // ABI of the pinned tree-sitter-json grammar
    CHECK(view.isParseable());
#if defined(CPP_TS_FEATURE_WASM)
    CHECK_FALSE(view.isWasm());
#endif // defined(CPP_TS_FEATURE_WASM)
}

TEST_CASE("LanguageView symbols", "[language][languageview]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    const ts::Symbol array = view.getSymbolForName("array", true);
    REQUIRE(array != 0);
    CHECK(view.getSymbolName(array) == "array");
    CHECK(view.getSymbolType(array) == ts::SymbolType::TypeRegular);

    const ts::Symbol bracket = view.getSymbolForName("[", false);
    REQUIRE(bracket != 0);
    CHECK(view.getSymbolType(bracket) == ts::SymbolType::TypeAnonymous);

    CHECK(view.getSymbolForName("no_such_symbol", true) == 0);

    // An empty name must not match "ERROR" (tree-sitter compares names with strncmp) and a default-constructed view
    // (null data() pointer) must not reach the C API.
    CHECK(view.getSymbolForName(ts::StringView(), true) == 0);
    CHECK(view.getSymbolForName(ts::StringView(), false) == 0);
}

TEST_CASE("LanguageView fields", "[language][languageview]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    const ts::FieldID key = view.getFieldIDForName("key");
    REQUIRE(key != 0);
    CHECK(view.getFieldNameForID(key) == "key");
    CHECK(view.getFieldNameForID(0).empty());
    CHECK(view.getFieldIDForName("missing") == 0);
    CHECK(view.getFieldIDForName(ts::StringView()) == 0);

    const std::vector<ts::details::StringViewReturn> names = view.getAllFieldsNames();
    REQUIRE(names.size() == 2u);
    CHECK(((names[0] == "key" && names[1] == "value") || (names[0] == "value" && names[1] == "key")));
}

#if SIZE_MAX > UINT32_MAX
TEST_CASE("LanguageView rejects names longer than 4GB", "[language][languageview][exceptions]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    // A view with an artificial size, nothing of that size is allocated.
    const char           *fake_str      = "fake";
    constexpr std::size_t overflow_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1;
    const ts::StringView  huge_view(fake_str, overflow_size);

    CHECK_THROWS_AS(view.getSymbolForName(huge_view, true), std::length_error);
    CHECK_THROWS_AS(view.getFieldIDForName(huge_view), std::length_error);
}
#endif // SIZE_MAX > UINT32_MAX

TEST_CASE("LanguageView metadata and type hierarchy of an ABI 14 grammar", "[language][languageview]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    // Names, metadata and supertypes are stored in the language since ABI 15.
    CHECK(view.getName().empty());
    CHECK(ts_test::metadata(view).major_version == 0);

    CHECK(view.getAllSuperTypes().empty());
    CHECK(view.getSuperTypesView().empty());
    CHECK(view.getSubTypesView(view.getSymbolForName("_value", true)).empty());
    CHECK(view.getAllSubTypesForSuperType(9999).empty());

    // The zero-copy views match the copying accessors.
    const std::vector<ts::Symbol> supertypes = view.getAllSuperTypes();
    const ts::SymbolSpan          span       = view.getSuperTypesView();
    REQUIRE(span.size() == supertypes.size());
    for (std::size_t i = 0; i < span.size(); ++i)
    {
        CHECK(span[i] == supertypes[i]);
        CHECK(view.getSubTypesView(span[i]).size() == view.getAllSubTypesForSuperType(span[i]).size());
    }
}

TEST_CASE("LanguageView state navigation", "[language][languageview]")
{
    const ts::Language     language(tree_sitter_json());
    const ts::LanguageView view = language.getView();

    const ts::Symbol array = view.getSymbolForName("array", true);
    CHECK(view.getNextState(1, array) != 0);

    ts::LookaheadIterator iterator = view.getLookaheadIterator(1);
    CHECK(static_cast<const TSLanguage *>(iterator.getLanguage()) == static_cast<const TSLanguage *>(view));
}

TEST_CASE("Invalid LanguageView returns neutral values", "[language][languageview]")
{
    const ts::LanguageView view;

    CHECK_FALSE(view.isValid());
    CHECK_FALSE(view.isParseable());
    CHECK(view.getSymbolsCount() == 0u);
    CHECK(view.getStatesCount() == 0u);
    CHECK(view.getFieldsCount() == 0u);
    CHECK(view.getVersion() == 0u);
    CHECK(view.getName().empty());
    CHECK(view.getSymbolName(1).empty());
    CHECK(view.getSymbolType(1) == ts::SymbolType::TypeRegular);
    CHECK(view.getSymbolForName("array", true) == 0);
    CHECK(view.getFieldIDForName("key") == 0);
    CHECK(view.getFieldNameForID(1).empty());
    CHECK(view.getAllFieldsNames().empty());
    CHECK(view.getAllSuperTypes().empty());
    CHECK(view.getAllSubTypesForSuperType(1).empty());
    CHECK(view.getSuperTypesView().empty());
    CHECK(view.getNextState(1, 1) == 0);
    CHECK(ts_test::metadata(view).major_version == 0);
#if CPP_TS_HAS_OPTIONAL
    CHECK_FALSE(view.getMetadata().has_value());
#endif // CPP_TS_HAS_OPTIONAL
    CHECK_THROWS_AS(view.getLookaheadIterator(1), std::logic_error);
}
