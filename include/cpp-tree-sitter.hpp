#pragma once
#ifndef CPP_TREE_SITTER_HPP
#define CPP_TREE_SITTER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// Standard Compatibility Macros
/////////////////////////////////////////////////////////////////////////////

#if defined(_MSVC_LANG)
#define CPP_TS_CXX_LEVEL _MSVC_LANG
#else
#define CPP_TS_CXX_LEVEL __cplusplus
#endif // defined(_MSVC_LANG)

#if defined(CPP_TS_HEADER_ONLY)
#define CPP_TS_API inline
#define CPP_TS_HAS_CXX17 (CPP_TS_CXX_LEVEL >= 201703L)
#define CPP_TS_HAS_CXX20 (CPP_TS_CXX_LEVEL >= 202002L)
#define CPP_TS_HAS_CXX23 (CPP_TS_CXX_LEVEL >= 202302L)
#else
#define CPP_TS_API
#define CPP_TS_HAS_CXX17 (CPP_TS_CXX_LEVEL >= 201703L && CPP_TS_CXX_STANDARD >= 17)
#define CPP_TS_HAS_CXX20 (CPP_TS_CXX_LEVEL >= 202002L && CPP_TS_CXX_STANDARD >= 20)
#define CPP_TS_HAS_CXX23 (CPP_TS_CXX_LEVEL >= 202302L && CPP_TS_CXX_STANDARD >= 23)
#endif // defined(CPP_TS_HEADER_ONLY)

#if CPP_TS_HAS_CXX17
#include <filesystem>
#include <optional>
#include <string_view>
#endif // CPP_TS_HAS_CXX17

#if CPP_TS_HAS_CXX20
#include <concepts>
#include <ranges>
#include <span>
#endif // CPP_TS_HAS_CXX20

#if CPP_TS_HAS_CXX23
#include <expected>
#endif // CPP_TS_HAS_CXX23

#if !CPP_TS_HAS_CXX17
#include "string-view.hpp"
#endif // !CPP_TS_HAS_CXX17

#if !CPP_TS_HAS_CXX20
#include "byte-view.hpp"
#endif // !CPP_TS_HAS_CXX20

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <io.h>
// On Windows we get descriptor using _fileno
#define CPP_TS_FILENO _fileno
// On Windows we create duplicate of descriptor using _dup
#define CPP_TS_DUP _dup
#else
#include <cstdio>
// On POSIX systems we use fileno from <cstdio>
#define CPP_TS_FILENO fileno
#include <unistd.h>
// On POSIX systems we use dup from <unistd.h>
#define CPP_TS_DUP dup
#endif // defined(_WIN32) && !defined(__CYGWIN__)

#include <tree_sitter/api.h>

namespace ts
{
    /////////////////////////////////////////////////////////////////////////////
    // Aliases.
    // Create slightly stricter aliases for some of the core tree-sitter types.
    /////////////////////////////////////////////////////////////////////////////

    // Direct alias of { major_version: uint8_t; minor_version: uint8_t; patch_version: uint8_t }
    using LanguageMetadata = TSLanguageMetadata;

    using StateID = std::uint16_t;
    using Symbol  = std::uint16_t;
    using FieldID = std::uint16_t;
    using Version = std::uint32_t;
    using NodeID  = std::uintptr_t;

#if defined(CPP_TS_FEATURE_WASM)
    using WasmEngine = wasm_engine_t;
#endif // defined(CPP_TS_FEATURE_WASM)

#if CPP_TS_HAS_CXX17
    using StringView = std::string_view;
#else
    using StringView = details::StringView;
#endif // CPP_TS_HAS_CXX17

#if CPP_TS_HAS_CXX20
    using ByteView = std::span<const std::uint8_t>;
#else
    using ByteView = details::ByteView;
#endif // CPP_TS_HAS_CXX20

    using DecodeFunction = std::function<std::uint32_t(ByteView, std::int32_t *)>;

    /////////////////////////////////////////////////////////////////////////////
    // Helper Classes & Structs
    // Internal utilities for memory management and range representation.
    /////////////////////////////////////////////////////////////////////////////

    // An inclusive range representation
    template <typename T>
    struct Extent
    {
        T start;
        T end;

        constexpr Extent() noexcept(std::is_nothrow_default_constructible<T>::value) : start(), end()
        {}

        constexpr Extent(T start_value, T end_value) noexcept(std::is_nothrow_move_constructible<T>::value)
            : start(std::move(start_value)), end(std::move(end_value))
        {}
    };

    namespace details
    {
        using StringViewParameter = const StringView;
        using StringViewReturn    = StringView;

#if CPP_TS_HAS_CXX17
        template <typename T>
        using OptionalParam = std::optional<std::reference_wrapper<T>>;
#else
        template <typename T>
        using OptionalParam = T *;
#endif // CPP_TS_HAS_CXX17

        // NOTE: relies on T having a conversion operator to Raw* (e.g. Tree::operator TSTree*()).
        // We cast the *referenced object*, not its address - static_cast can invoke a
        // user-defined conversion operator on a reference, but NOT between two unrelated
        // pointer types.
        template <typename T, typename Raw>
        [[nodiscard]] static Raw *get_raw(OptionalParam<T> opt)
        {
            if (!opt)
            {
                return nullptr;
            }

#if CPP_TS_HAS_CXX17
            return static_cast<Raw *>(opt->get());
#else
            return static_cast<Raw *>(*opt);
#endif // CPP_TS_HAS_CXX17
        }

        struct FreeHelper
        {
            template <typename T>
            inline void operator()(T *raw_pointer) const
            {
                if (!raw_pointer)
                {
                    return;
                }

                std::free(raw_pointer);
            }
        };

        template <>
        inline void FreeHelper::operator()<const TSLanguage>(const TSLanguage *raw_pointer) const
        {
            if (!raw_pointer)
            {
                return;
            }

            ts_language_delete(raw_pointer);
        }

        template <>
        inline void FreeHelper::operator()<TSQueryCursor>(TSQueryCursor *raw_pointer) const
        {
            if (!raw_pointer)
            {
                return;
            }

            ts_query_cursor_delete(raw_pointer);
        }

        template <>
        inline void FreeHelper::operator()<TSLookaheadIterator>(TSLookaheadIterator *raw_pointer) const
        {
            if (!raw_pointer)
            {
                return;
            }

            ts_lookahead_iterator_delete(raw_pointer);
        }

        struct QueryErrorHelper
        {
            [[nodiscard]] static std::string format_error(std::uint32_t error_offset, const TSQueryError &error);
            static void                      validate(std::uint32_t error_offset, TSQueryError &error);
        };

#if defined(CPP_TS_FEATURE_WASM)
        struct WasmErrorHelper
        {
            [[nodiscard]] static std::string format_error(const TSWasmError &error);
            static void                      validate(TSWasmError &error);
        };

        [[nodiscard]] static std::string get_grammar_name_from_file(StringViewParameter path);
#endif // defined(CPP_TS_FEATURE_WASM)

        [[nodiscard]] StringViewReturn make_view(const char *str) noexcept;

        [[nodiscard]] StringViewReturn make_view(const char *str, std::size_t length) noexcept;

        template <typename T_TSOptions, typename T_TSState, typename T_CPPState>
        class OptionsBase
        {
        public:
            using ProgressCallbackFunction = std::function<bool(T_CPPState *)>;

            OptionsBase() : progress_callback_(nullptr), impl_{ std::make_unique<T_TSOptions>() }
            {
                impl_->payload = this;
            }

            OptionsBase(ProgressCallbackFunction callback)
                : progress_callback_(callback ? std::move(callback) : nullptr), impl_{ std::make_unique<T_TSOptions>() }
            {
                impl_->payload = this;
            }

            OptionsBase(OptionsBase &&) = delete;

            OptionsBase &operator=(OptionsBase &&) = delete;

            OptionsBase(const OptionsBase &) = delete;

            OptionsBase &operator=(const OptionsBase &) = delete;

            [[nodiscard]] const ProgressCallbackFunction &getProgressCallback() const noexcept
            {
                return progress_callback_;
            }

            void setProgressCallback(ProgressCallbackFunction callback)
            {
                if (!callback)
                {
                    progress_callback_ = nullptr;
                    return;
                }
                progress_callback_ = std::move(callback);
            }

            [[nodiscard]] operator T_TSOptions() const
            {
                T_TSOptions options{};
                options.payload           = const_cast<OptionsBase *>(this);
                options.progress_callback = progress_callback_ ? &progressCallbackProxy : nullptr;
                return options;
            }

            [[nodiscard]] operator T_TSOptions *() const noexcept
            {
                impl_->progress_callback = progress_callback_ ? &progressCallbackProxy : nullptr;
                return impl_.get();
            }

        private:
            [[nodiscard]] static bool progressCallbackProxy(T_TSState *state)
            {
                if (state && state->payload)
                {
                    auto *self = static_cast<OptionsBase *>(state->payload);
                    if (self->progress_callback_)
                    {
                        T_CPPState cpp_state(state);
                        return self->progress_callback_(&cpp_state);
                    }
                }
                return false;
            }

        private:
            ProgressCallbackFunction     progress_callback_ = nullptr;
            std::unique_ptr<T_TSOptions> impl_{};
        };
    } // namespace details

    /////////////////////////////////////////////////////////////////////////////
    // Enums.
    // Scoped enum wrappers for Tree-sitter constants to ensure type safety.
    /////////////////////////////////////////////////////////////////////////////

    enum class InputEncoding : std::uint8_t
    {
        UTF8    = TSInputEncodingUTF8,
        UTF16LE = TSInputEncodingUTF16LE,
        UTF16BE = TSInputEncodingUTF16BE,
        Custom  = TSInputEncodingCustom
    };

    enum class SymbolType : std::uint8_t
    {
        TypeRegular   = TSSymbolTypeRegular,
        TypeAnonymous = TSSymbolTypeAnonymous,
        TypeSupertype = TSSymbolTypeSupertype,
        TypeAuxiliary = TSSymbolTypeAuxiliary,
    };

    enum class LogType : std::uint8_t
    {
        Parse = TSLogTypeParse,
        Lex   = TSLogTypeLex
    };

    enum class Quantifier : std::uint8_t
    {
        Zero       = TSQuantifierZero,
        ZeroOrOne  = TSQuantifierZeroOrOne,
        ZeroOrMore = TSQuantifierZeroOrMore,
        One        = TSQuantifierOne,
        OneOrMore  = TSQuantifierOneOrMore,
    };

    enum class QueryPredicateStepType : std::uint8_t
    {
        Done    = TSQueryPredicateStepTypeDone,
        Capture = TSQueryPredicateStepTypeCapture,
        String  = TSQueryPredicateStepTypeString
    };

    /////////////////////////////////////////////////////////////////////////////
    // Point
    // A location in source code in terms of rows and columns.
    /////////////////////////////////////////////////////////////////////////////

    struct InputEdit;

    struct Point
    {
        std::uint32_t row;
        std::uint32_t column;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        constexpr Point() noexcept : row(0), column(0)
        {}

        constexpr Point(std::uint32_t row_value, std::uint32_t column_value) noexcept
            : row(row_value), column(column_value)
        {}

        constexpr Point(const TSPoint &other) noexcept : row(other.row), column(other.column)
        {}

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        // Definition deferred until after the definition of InputEdit.
        void edit(const InputEdit &edit, std::uint32_t &byte_offset);

        ////////////////////////////////////////////////////////////////
        // Comparision
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr bool operator==(const Point &other) const noexcept
        {
            return row == other.row && column == other.column;
        }

#if !CPP_TS_HAS_CXX20
        [[nodiscard]] constexpr bool operator!=(const Point &other) const noexcept
        {
            return !(*this == other);
        }
#endif // !CPP_TS_HAS_CXX20

        [[nodiscard]] constexpr bool operator>(const Point &other) const noexcept
        {
            return (row > other.row) || (row == other.row && column > other.column);
        }

        [[nodiscard]] constexpr bool operator<=(const Point &other) const noexcept
        {
            return !(*this > other);
        }

        [[nodiscard]] constexpr bool operator<(const Point &other) const noexcept
        {
            return (row < other.row) || (row == other.row && column < other.column);
        }

        [[nodiscard]] constexpr bool operator>=(const Point &other) const noexcept
        {
            return !(*this < other);
        }

        [[nodiscard]] static constexpr bool isValidRange(const Point &start, const Point &end) noexcept
        {
            return start <= end;
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr operator TSPoint() const noexcept
        {
            return { row, column };
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // InputEdit
    // Data describing an edit to a source code string.
    /////////////////////////////////////////////////////////////////////////////

    struct InputEdit
    {
        std::uint32_t start_byte;
        std::uint32_t old_end_byte;
        std::uint32_t new_end_byte;
        Point         start_point;
        Point         old_end_point;
        Point         new_end_point;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        constexpr InputEdit() noexcept
            : start_byte(0), old_end_byte(0), new_end_byte(0), start_point{}, old_end_point{}, new_end_point{}
        {}

        constexpr InputEdit(std::uint32_t start_byte_value,
                            std::uint32_t old_end_byte_value,
                            std::uint32_t new_end_byte_value,
                            Point         start_point_value,
                            Point         old_end_point_value,
                            Point         new_end_point_value) noexcept
            : start_byte(start_byte_value)
            , old_end_byte(old_end_byte_value)
            , new_end_byte(new_end_byte_value)
            , start_point{ start_point_value }
            , old_end_point{ old_end_point_value }
            , new_end_point{ new_end_point_value }
        {}

        constexpr explicit InputEdit(const TSInputEdit &edit) noexcept
            : start_byte(edit.start_byte)
            , old_end_byte(edit.old_end_byte)
            , new_end_byte(edit.new_end_byte)
            , start_point{ edit.start_point }
            , old_end_point{ edit.old_end_point }
            , new_end_point{ edit.new_end_point }
        {}

        ////////////////////////////////////////////////////////////////
        // Validation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return start_byte <= old_end_byte && Point::isValidRange(start_point, old_end_point);
        }

        constexpr void validate() const
        {
            if (!isValid())
            {
                throw std::invalid_argument("Tree-sitter: Invalid edit ranges (start > old_end)");
            }
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr operator TSInputEdit() const noexcept
        {
            return { start_byte, old_end_byte, new_end_byte, start_point, old_end_point, new_end_point };
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // Range
    // Representation of a range within the source code using points and bytes.
    /////////////////////////////////////////////////////////////////////////////

    struct Range
    {
        Extent<Point>         point;
        Extent<std::uint32_t> byte;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        constexpr explicit Range(Extent<Point> point_value, Extent<std::uint32_t> byte_value) noexcept
            : point(point_value), byte(byte_value)
        {}

        constexpr explicit Range(const TSRange &range) noexcept
            : point(range.start_point, range.end_point), byte(range.start_byte, range.end_byte)
        {}

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        void edit(const InputEdit &edit);

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr operator TSRange() const noexcept
        {
            return { point.start, point.end, byte.start, byte.end };
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // Input
    // A structure that defines how to read and decode source code incrementally.
    /////////////////////////////////////////////////////////////////////////////

    class Input
    {
    public:
        using ReadFunction = std::function<details::StringViewReturn(std::uint32_t, Point, std::uint32_t *)>;

        Input();

        Input(ReadFunction read, InputEncoding encoding);

        Input(ReadFunction read, ts::DecodeFunction decode);

        Input(const Input &) = delete;

        Input(Input &&other) noexcept;

        ////////////////////////////////////////////////////////////////
        // Accessors
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] const ReadFunction &getRead() const noexcept
        {
            return read_;
        }

        void setRead(ReadFunction read) noexcept
        {
            if (!read)
            {
                return;
            }
            read_ = std::move(read);
        }

        [[nodiscard]] InputEncoding getEncoding() const noexcept
        {
            return encoding_;
        }

        // Switching away from Custom silently drops any decode function, since it would
        // otherwise be ignored by the C API without any indication of why.
        void setEncoding(InputEncoding encoding) noexcept
        {
            encoding_ = encoding;
            if (encoding_ != InputEncoding::Custom)
            {
                decode_ = nullptr;
            }
        }

        [[nodiscard]] const ts::DecodeFunction &getDecode() const noexcept
        {
            return decode_;
        }

        void setDecode(ts::DecodeFunction decode)
        {
            if (!decode)
            {
                return;
            }
            decode_   = std::move(decode);
            encoding_ = InputEncoding::Custom;
        }

        ////////////////////////////////////////////////////////////////
        // Static Functions
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
        static thread_local const Input *current_input_ptr;
#else
        static const Input *&current_ptr() noexcept
        {
            thread_local const Input *ptr = nullptr;
            return ptr;
        }
#endif // CPP_TS_HAS_CXX17

        [[nodiscard]] static const char *read_proxy(void          *payload,
                                                    std::uint32_t  byte_index,
                                                    TSPoint        position,
                                                    std::uint32_t *bytes_read);

        [[nodiscard]] static std::uint32_t decode_proxy(const std::uint8_t *string,
                                                        std::uint32_t       length,
                                                        std::int32_t       *code_point) noexcept;

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        Input &operator=(const Input &) = delete;

        Input &operator=(Input &&other) noexcept;

        [[nodiscard]] operator TSInput() const;

        [[nodiscard]] operator TSInput *() const noexcept;

    private:
        ReadFunction             read_     = nullptr;
        ts::DecodeFunction       decode_   = nullptr;
        InputEncoding            encoding_ = InputEncoding::UTF8;
        std::unique_ptr<TSInput> impl_{};
    };

    /////////////////////////////////////////////////////////////////////////////
    // ParseState
    // A structure representing the state of the parser at a specific position.
    /////////////////////////////////////////////////////////////////////////////

    struct ParseState
    {
        std::uint32_t current_byte_offset;
        bool          has_error;

        constexpr explicit ParseState(const TSParseState *state) noexcept
            : current_byte_offset(state->current_byte_offset), has_error(state->has_error)
        {}

        constexpr explicit ParseState(const TSParseState &state) noexcept
            : current_byte_offset(state.current_byte_offset), has_error(state.has_error)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // ParseOptions
    // Options that control the behavior of the parsing process.
    /////////////////////////////////////////////////////////////////////////////

    using ParseOptions = details::OptionsBase<TSParseOptions, TSParseState, ParseState>;

    /////////////////////////////////////////////////////////////////////////////
    // LanguageView
    // Non-owning reference to a Tree-sitter grammar. Does NOT free memory.
    // Equivalent to passing `const TSLanguage*` in C.
    /////////////////////////////////////////////////////////////////////////////

    class LookaheadIterator;

    class LanguageView
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        constexpr LanguageView() noexcept : impl_{ nullptr }
        {}

        // NOTE: Allowing implicit conversions from TSLanguage to Language
        // improves ergonomics for clients using external grammar providers.

        /* implicit */ constexpr LanguageView(const TSLanguage *language) noexcept : impl_{ language }
        {}

        ////////////////////////////////////////////////////////////////
        // Count Accessors
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::uint32_t getSymbolsCount() const noexcept
        {
            return isValid() ? ts_language_symbol_count(impl_) : 0;
        }

        [[nodiscard]] std::uint32_t getStatesCount() const noexcept
        {
            return isValid() ? ts_language_state_count(impl_) : 0;
        }

        [[nodiscard]] std::uint32_t getFieldsCount() const noexcept
        {
            return isValid() ? ts_language_field_count(impl_) : 0;
        }

        ////////////////////////////////////////////////////////////////
        // Symbol Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getSymbolName(Symbol symbol) const;

        [[nodiscard]] SymbolType getSymbolType(Symbol symbol) const;

        // Returns 0 if field was not found
        [[nodiscard]] Symbol getSymbolForName(details::StringViewParameter name, bool isNamed) const;

        ////////////////////////////////////////////////////////////////
        // Field Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getFieldNameForID(FieldID id) const;

        // Returns 0 if field was not found
        [[nodiscard]] FieldID getFieldIDForName(details::StringViewParameter name) const;

        [[nodiscard]] std::vector<details::StringViewReturn> getAllFieldsNames() const;

        ////////////////////////////////////////////////////////////////
        // Type Hierarchy
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<Symbol> getAllSuperTypes() const noexcept;

        [[nodiscard]] std::vector<Symbol> getAllSubTypesForSuperType(Symbol supertype) const noexcept;

        ////////////////////////////////////////////////////////////////
        // State Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] StateID getNextState(StateID state, Symbol symbol) const noexcept
        {
            return isValid() ? ts_language_next_state(impl_, state, symbol) : 0;
        }

        [[nodiscard]] LookaheadIterator getLookaheadIterator(StateID state) const;

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getName() const;

        [[nodiscard]] Version getVersion() const noexcept
        {
            return isValid() ? ts_language_abi_version(impl_) : 0;
        }

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return impl_ != nullptr;
        }

#if CPP_TS_HAS_CXX17
        [[nodiscard]] std::optional<LanguageMetadata> getMetadata() const noexcept;
#else
        [[nodiscard]] LanguageMetadata getMetadata() const noexcept;
#endif // CPP_TS_HAS_CXX17

#if defined(CPP_TS_FEATURE_WASM)
        [[nodiscard]] bool isWasm() const noexcept
        {
            return isValid() && ts_language_is_wasm(impl_);
        }
#endif // defined(CPP_TS_FEATURE_WASM)

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr operator const TSLanguage *() const noexcept
        {
            return impl_;
        }

    private:
        const TSLanguage *impl_;
    };

    /////////////////////////////////////////////////////////////////////////////
    // Language
    // Owning container for a Tree-sitter grammar. Automatically frees memory.
    /////////////////////////////////////////////////////////////////////////////

    class Language
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        Language() noexcept : impl_{ nullptr }
        {}

        // NOTE: Protects against the accidental taking over of a pointer that should not be destroyed.
        explicit Language(const TSLanguage *language) noexcept : impl_{ language }
        {}

        Language(const Language &other) noexcept
            : impl_{ other.impl_.get() != nullptr ? ts_language_copy(other.impl_.get()) : nullptr }
        {}

        Language(Language &&other) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr LanguageView getView() const noexcept
        {
            return LanguageView(impl_.get());
        }

        [[nodiscard]] bool isValid() const noexcept
        {
            return impl_.get() != nullptr;
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        Language &operator=(const Language &other) noexcept;

        Language &operator=(Language &&other) noexcept = default;

        [[nodiscard]] operator LanguageView() const noexcept
        {
            return LanguageView(impl_.get());
        }

        [[nodiscard]] operator const TSLanguage *() const noexcept
        {
            return impl_.get();
        }

    private:
        std::unique_ptr<const TSLanguage, details::FreeHelper> impl_{ nullptr };
    };

    /////////////////////////////////////////////////////////////////////////////
    // Node
    // A single node in a parse tree, providing navigation and attributes.
    /////////////////////////////////////////////////////////////////////////////

    class TreeCursor;

    class Node
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        constexpr Node() noexcept : impl_{}
        {}

        constexpr explicit Node(TSNode node) noexcept : impl_{ node }
        {}

        [[nodiscard]] static constexpr Node null() noexcept
        {
            return Node();
        }

        ////////////////////////////////////////////////////////////////
        // Identification & Type
        ////////////////////////////////////////////////////////////////

        // Returns a unique identifier for a node in a parse tree.
        [[nodiscard]] constexpr NodeID getID() const noexcept
        {
            return isNull() ? 0 : reinterpret_cast<NodeID>(impl_.id);
        }

        [[nodiscard]] details::StringViewReturn getType() const
        {
            return isNull() ? details::StringViewReturn("") : details::make_view(ts_node_type(impl_));
        }

        [[nodiscard]] Symbol getSymbol() const noexcept
        {
            return isNull() ? 0 : ts_node_symbol(impl_);
        }

        [[nodiscard]] details::StringViewReturn getGrammarType() const
        {
            return isNull() ? details::StringViewReturn("") : details::make_view(ts_node_grammar_type(impl_));
        }

        [[nodiscard]] Symbol getGrammarSymbol() const noexcept
        {
            return isNull() ? 0 : ts_node_grammar_symbol(impl_);
        }

        [[nodiscard]] LanguageView getLanguage() const noexcept
        {
            return isNull() ? LanguageView(nullptr) : LanguageView(ts_node_language(impl_));
        }

        ////////////////////////////////////////////////////////////////
        // Flag Checks
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool isNull() const noexcept
        {
            return ts_node_is_null(impl_);
        }

        [[nodiscard]] bool isNamed() const noexcept
        {
            return !isNull() && ts_node_is_named(impl_);
        }

        [[nodiscard]] bool isMissing() const noexcept
        {
            return !isNull() && ts_node_is_missing(impl_);
        }

        [[nodiscard]] bool isExtra() const noexcept
        {
            return !isNull() && ts_node_is_extra(impl_);
        }

        [[nodiscard]] bool isError() const noexcept
        {
            return !isNull() && ts_node_is_error(impl_);
        }

        [[nodiscard]] bool hasError() const noexcept
        {
            return !isNull() && ts_node_has_error(impl_);
        }

        [[nodiscard]] bool hasChanges() const noexcept
        {
            return !isNull() && ts_node_has_changes(impl_);
        }

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Extent<std::uint32_t> getByteRange() const noexcept
        {
            return isNull() ? Extent<std::uint32_t>()
                            : Extent<std::uint32_t>({ ts_node_start_byte(impl_), ts_node_end_byte(impl_) });
        }

        [[nodiscard]] Extent<Point> getPointRange() const noexcept
        {
            return isNull() ? Extent<Point>() : Extent<Point>({ ts_node_start_point(impl_), ts_node_end_point(impl_) });
        }

        ////////////////////////////////////////////////////////////////
        // Source Text Access
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getSourceRange(details::StringViewParameter source) const;

        [[nodiscard]] std::string getSourceText(details::StringViewParameter source) const;

        // Returns an S-Expression representation of the subtree rooted at this node.
        [[nodiscard]] std::unique_ptr<char, details::FreeHelper> getSExpr() const
        {
            return isNull() ? nullptr : std::unique_ptr<char, details::FreeHelper>{ ts_node_string(impl_) };
        }

        ////////////////////////////////////////////////////////////////
        // Navigation (General)
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getParent() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_parent(impl_) };
        }

        [[nodiscard]] Node getNextSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_next_sibling(impl_) };
        }

        [[nodiscard]] Node getPreviousSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_prev_sibling(impl_) };
        }

        [[nodiscard]] std::uint32_t getChildCount() const noexcept
        {
            return isNull() ? 0 : ts_node_child_count(impl_);
        }

        [[nodiscard]] Node getChild(std::uint32_t child_index) const noexcept
        {
            return (child_index >= getChildCount()) ? null() : Node{ ts_node_child(impl_, child_index) };
        }

        [[nodiscard]] Node getFirstChild() const noexcept
        {
            return getChild(0);
        }

        [[nodiscard]] Node getLastChild() const noexcept
        {
            std::uint32_t count = getChildCount();
            return getChild(count > 0 ? count - 1 : count);
        }

        [[nodiscard]] Node getChildWithDescendant(Node descendant) const noexcept
        {
            return (isNull() || descendant.isNull()) ? null()
                                                     : Node{ ts_node_child_with_descendant(impl_, descendant.impl_) };
        }

        [[nodiscard]] Node getChildByType(details::StringViewParameter name) const;

        [[nodiscard]] Node getChildBySymbol(Symbol sym) const noexcept;

        ////////////////////////////////////////////////////////////////
        // Navigation (Named Children)
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getNextNamedSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_next_named_sibling(impl_) };
        }

        [[nodiscard]] Node getPreviousNamedSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_prev_named_sibling(impl_) };
        }

        [[nodiscard]] std::uint32_t getNamedChildCount() const noexcept
        {
            return isNull() ? 0 : ts_node_named_child_count(impl_);
        }

        [[nodiscard]] Node getNamedChild(std::uint32_t child_index) const noexcept
        {
            return (child_index >= getNamedChildCount()) ? null() : Node{ ts_node_named_child(impl_, child_index) };
        }

        [[nodiscard]] Node getFirstNamedChild() const noexcept
        {
            return getNamedChild(0);
        }

        [[nodiscard]] Node getLastNamedChild() const noexcept
        {
            std::uint32_t count = getNamedChildCount();
            return getNamedChild(count > 0 ? count - 1 : count);
        }

        [[nodiscard]] Node getNamedChildByType(details::StringViewParameter name) const;

        [[nodiscard]] Node getNamedChildBySymbol(Symbol sym) const noexcept;

        ////////////////////////////////////////////////////////////////
        // Search & Descendants
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::uint32_t getDescendantsCount() const noexcept
        {
            return isNull() ? 0 : ts_node_descendant_count(impl_);
        }

        [[nodiscard]] Node getFirstChildForByte(std::uint32_t byte) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_first_child_for_byte(impl_, byte) };
        }

        [[nodiscard]] Node getFirstNamedChildForByte(std::uint32_t byte) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_first_named_child_for_byte(impl_, byte) };
        }

        [[nodiscard]] Node getDescendantForByteRange(Extent<std::uint32_t> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_descendant_for_byte_range(impl_, range.start, range.end) };
        }

        [[nodiscard]] Node getNamedDescendantForByteRange(Extent<std::uint32_t> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_named_descendant_for_byte_range(impl_, range.start, range.end) };
        }

        [[nodiscard]] Node getDescendantForPointRange(Extent<Point> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_descendant_for_point_range(impl_, range.start, range.end) };
        }

        [[nodiscard]] Node getNamedDescendantForPointRange(Extent<Point> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_named_descendant_for_point_range(impl_, range.start, range.end) };
        }

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getFieldNameForChild(std::uint32_t child_index) const
        {
            return isNull() ? details::StringViewReturn("")
                            : details::make_view(ts_node_field_name_for_child(impl_, child_index));
        }

        [[nodiscard]] details::StringViewReturn getFieldNameForNamedChild(std::uint32_t named_child_index) const
        {
            return isNull() ? details::StringViewReturn("")
                            : details::make_view(ts_node_field_name_for_named_child(impl_, named_child_index));
        }

        [[nodiscard]] Node getChildByFieldName(details::StringViewParameter name) const;

        [[nodiscard]] Node getChildByFieldID(FieldID field_id) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_child_by_field_id(impl_, field_id) };
        }

        ////////////////////////////////////////////////////////////////
        // Parsing State
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] StateID getParseState() const noexcept
        {
            return isNull() ? 0 : ts_node_parse_state(impl_);
        }

        [[nodiscard]] StateID getNextParseState() const noexcept
        {
            return isNull() ? 0 : ts_node_next_parse_state(impl_);
        }

        ////////////////////////////////////////////////////////////////
        // Mutations
        ////////////////////////////////////////////////////////////////

        void edit(const InputEdit &edit);

        ////////////////////////////////////////////////////////////////
        // Cursor
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] TreeCursor getCursor() const noexcept;

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool operator==(const Node &other) const noexcept
        {
            return ts_node_eq(impl_, other.impl_);
        }

#if !CPP_TS_HAS_CXX20
        [[nodiscard]] bool operator!=(const Node &other) const noexcept
        {
            return !(*this == other);
        }
#endif // !CPP_TS_HAS_CXX20

        [[nodiscard]] constexpr operator TSNode() const noexcept
        {
            return impl_;
        }

    private:
        TSNode impl_{};
    };

    /////////////////////////////////////////////////////////////////////////////
    // Tree
    // Manages the lifetime of a complete parse tree.
    /////////////////////////////////////////////////////////////////////////////

    class Tree
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        explicit Tree(TSTree *tree) noexcept : impl_{ tree, ts_tree_delete }, is_valid_(tree != nullptr)
        {}

        Tree(const Tree &other) noexcept;

        Tree(Tree &&other) noexcept;

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool hasError() const noexcept
        {
            return isValid() && getRootNode().hasError();
        }

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return is_valid_;
        }

        ////////////////////////////////////////////////////////////////
        // Root
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getRootNode() const noexcept
        {
            return isValid() ? Node{ ts_tree_root_node(impl_.get()) } : Node::null();
        }

        [[nodiscard]] Node getRootNodeWithOffset(std::uint32_t offset_bytes, Point offset_extent) const noexcept
        {
            return isValid() ? Node{ ts_tree_root_node_with_offset(impl_.get(), offset_bytes, offset_extent) }
                             : Node::null();
        }

        void edit(const InputEdit &edit);

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<Range> getIncludedRanges() const;

        [[nodiscard]] static std::vector<Range> getChangedRanges(const Tree &old_tree, const Tree &new_tree);

        ////////////////////////////////////////////////////////////////
        // Language
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] LanguageView getLanguage() const noexcept
        {
            return isValid() ? LanguageView(ts_tree_language(impl_.get())) : LanguageView(nullptr);
        }

        ////////////////////////////////////////////////////////////////
        // Copy
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Tree copy() const noexcept
        {
            return Tree(*this);
        }

        ////////////////////////////////////////////////////////////////
        // Debugging
        ////////////////////////////////////////////////////////////////

        void printDotGraph(int file_descriptor) noexcept
        {
            if (!isValid())
            {
                return;
            }
            ts_tree_print_dot_graph(impl_.get(), file_descriptor);
        }

        void printDotGraph(FILE *file)
        {
            if (!isValid() || file == nullptr)
            {
                return;
            }

            printDotGraph(CPP_TS_FILENO(file));
        }

        void printDotGraph(details::StringViewParameter path);

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        Tree &operator=(const Tree &other);

        Tree &operator=(Tree &&other) noexcept;

        [[nodiscard]] operator TSTree *() const noexcept
        {
            return impl_.get();
        }

    private:
        std::unique_ptr<TSTree, decltype(&ts_tree_delete)> impl_{ nullptr, ts_tree_delete };
        bool                                               is_valid_ = false;
    };

    /////////////////////////////////////////////////////////////////////////////
    // Parser
    // State machine used to produce a syntax tree from source code.
    /////////////////////////////////////////////////////////////////////////////

#if defined(CPP_TS_FEATURE_WASM)
    class WasmStore;
#endif // defined(CPP_TS_FEATURE_WASM)

    class Parser
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Aliases
        ////////////////////////////////////////////////////////////////

        using LoggerFunction = std::function<void(LogType, const char *)>;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        Parser() noexcept : impl_{ ts_parser_new(), ts_parser_delete }
        {}

        explicit Parser(LanguageView language);

#if defined(CPP_TS_FEATURE_WASM)
        Parser(LanguageView language, WasmStore &store);
#endif // defined(CPP_TS_FEATURE_WASM)

        Parser(const Parser &) = delete;

        Parser(Parser &&other) noexcept;

#if CPP_TS_HAS_CXX23
        [[nodiscard]] static std::expected<Parser, std::string> create(LanguageView language) noexcept;

#if defined(CPP_TS_FEATURE_WASM)
        [[nodiscard]] static std::expected<Parser, std::string> create(LanguageView language,
                                                                       WasmStore   &store) noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_CXX23

        ////////////////////////////////////////////////////////////////
        // Configuration
        ////////////////////////////////////////////////////////////////

        [[nodiscard]]
#if CPP_TS_HAS_CXX23
        std::expected<bool, std::string> setLanguage(LanguageView language);
#else
        bool setLanguage(LanguageView language)
#if defined(CPP_TS_FEATURE_WASM)
                ;
#else
                noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_CXX23

        [[nodiscard]]
#if CPP_TS_HAS_CXX20
        bool setIncludedRanges(std::span<const Range> ranges);
#else
        bool setIncludedRanges(const std::vector<Range> &ranges);
#endif // CPP_TS_HAS_CXX20

#if defined(CPP_TS_FEATURE_WASM)
        // Passing a WasmStore invalidates it. The parser takes control of the WasmStore. You can retrieve it using the
        // takeWasmStore() function. If you set a new WasmStore without retrieving the previous one, the previous
        // instance will be discarded.
        void setWasmStore(WasmStore &store) noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)

        [[nodiscard]] LanguageView getCurrentLanguage() const noexcept
        {
            return hasLanguage() ? LanguageView(ts_parser_language(impl_.get())) : LanguageView(nullptr);
        }

        [[nodiscard]] std::vector<Range> getIncludedRanges() const noexcept;

#if defined(CPP_TS_FEATURE_WASM)
        // If the set language is WebAssembly, it will be removed from the parser.
        [[nodiscard]] WasmStore takeWasmStore() noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr bool hasLanguage() const noexcept
        {
            return language_set_;
        }

#if defined(CPP_TS_FEATURE_WASM)
        [[nodiscard]] constexpr bool hasWasmStore() const noexcept
        {
            return wasm_store_set_;
        }
#endif // defined(CPP_TS_FEATURE_WASM)

        ////////////////////////////////////////////////////////////////
        // Parsing
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Tree parse(Input                                     &input,
                                 details::OptionalParam<Tree>               old_tree = {},
                                 details::OptionalParam<const ParseOptions> options  = {});

        [[nodiscard]] Tree parseString(details::StringViewParameter buffer, details::OptionalParam<Tree> old_tree = {});

        [[nodiscard]] Tree parseStringEncoded(details::StringViewParameter buffer,
                                              InputEncoding                encoding,
                                              details::OptionalParam<Tree> old_tree = {});

        ////////////////////////////////////////////////////////////////
        // Debugging
        ////////////////////////////////////////////////////////////////

        void enableDotGraphs(int file_descriptor);

        void enableDotGraphs(FILE *file)
        {
            if (!file)
            {
                return;
            }

            enableDotGraphs(CPP_TS_FILENO(file));
        }

        void enableDotGraphs(details::StringViewParameter path);

        void disableDotGraphs()
        {
            enableDotGraphs(-1);
        }

        ////////////////////////////////////////////////////////////////
        // Logging
        ////////////////////////////////////////////////////////////////

        void setLogger(LoggerFunction logger_func);

        void removeLogger() noexcept
        {
            current_logger_ = nullptr;
            ts_parser_set_logger(impl_.get(), { nullptr, nullptr });
        }

        ////////////////////////////////////////////////////////////////
        // Reset
        ////////////////////////////////////////////////////////////////

        // Does not remove logger
        void reset() noexcept
        {
            ts_parser_reset(impl_.get());
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        Parser &operator=(const Parser &) = delete;

        Parser &operator=(Parser &&other) noexcept;

    private:
        std::unique_ptr<TSParser, decltype(&ts_parser_delete)> impl_{ nullptr, ts_parser_delete };
        std::function<void(LogType, const char *)>             current_logger_ = nullptr;
        bool                                                   language_set_   = false;
#if defined(CPP_TS_FEATURE_WASM)
        bool wasm_store_set_ = false;
#endif // defined(CPP_TS_FEATURE_WASM)
    };

    /////////////////////////////////////////////////////////////////////////////
    // TreeCursor
    // A stateful pointer for walking a syntax tree efficiently.
    /////////////////////////////////////////////////////////////////////////////

    class TreeCursor
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        TreeCursor() noexcept : impl_{}, is_valid_(false)
        {}

        explicit TreeCursor(TSNode node) noexcept;

        TreeCursor(const TSTreeCursor &cursor) noexcept;

        // By default avoid copies until the ergonomics are clearer.
        TreeCursor(const TreeCursor &other) = delete;

        TreeCursor(TreeCursor &&other) noexcept : TreeCursor()
        {
            std::swap(impl_, other.impl_);
            std::swap(is_valid_, other.is_valid_);
        }

        ~TreeCursor() noexcept
        {
            if (!isValid())
            {
                return;
            }
            ts_tree_cursor_delete(&impl_);
        }

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return is_valid_;
        }

        ////////////////////////////////////////////////////////////////
        // State Control
        ////////////////////////////////////////////////////////////////

        void reset(Node node) noexcept
        {
            if (!isValid() || node.isNull())
            {
                return;
            }
            ts_tree_cursor_reset(&impl_, node);
        }

        void reset(TreeCursor &cursor) noexcept
        {
            if (!isValid() || !cursor.isValid())
            {
                return;
            }
            ts_tree_cursor_reset_to(&impl_, &cursor.impl_);
        }

        [[nodiscard]] TreeCursor copy() const noexcept
        {
            return TreeCursor(impl_);
        }

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool gotoParent() noexcept
        {
            return isValid() && ts_tree_cursor_goto_parent(&impl_);
        }

        [[nodiscard]] bool gotoFirstChild() noexcept
        {
            return isValid() && ts_tree_cursor_goto_first_child(&impl_);
        }

        [[nodiscard]] bool gotoLastChild() noexcept
        {
            return isValid() && ts_tree_cursor_goto_last_child(&impl_);
        }

        [[nodiscard]] bool gotoNextSibling() noexcept
        {
            return isValid() && ts_tree_cursor_goto_next_sibling(&impl_);
        }

        [[nodiscard]] bool gotoPreviousSibling() noexcept
        {
            return isValid() && ts_tree_cursor_goto_previous_sibling(&impl_);
        }

        [[nodiscard]] std::int64_t gotoFirstChildForByte(std::uint32_t byte) noexcept
        {
            return isValid() ? ts_tree_cursor_goto_first_child_for_byte(&impl_, byte) : -1;
        }

        [[nodiscard]] std::int64_t gotoFirstChildForPoint(Point point) noexcept
        {
            return isValid() ? ts_tree_cursor_goto_first_child_for_point(&impl_, point) : -1;
        }

        void gotoDescendant(std::uint32_t descendant_index) noexcept
        {
            if (!isValid())
            {
                return;
            }
            ts_tree_cursor_goto_descendant(&impl_, descendant_index);
        }

        ////////////////////////////////////////////////////////////////
        // Current Node Attributes
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getCurrentNode() const noexcept
        {
            return isValid() ? Node{ ts_tree_cursor_current_node(&impl_) } : Node::null();
        }

        [[nodiscard]] details::StringViewReturn getCurrentFieldName() const
        {
            return isValid() ? details::make_view(ts_tree_cursor_current_field_name(&impl_))
                             : details::StringViewReturn("");
        }

        [[nodiscard]] FieldID getCurrentFieldID() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_field_id(&impl_) : 0;
        }

        [[nodiscard]] std::uint32_t getCurrentDescendantIndex() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_descendant_index(&impl_) : 0;
        }

        [[nodiscard]] std::uint32_t getDepthFromOrigin() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_depth(&impl_) : 0;
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool operator==(const TreeCursor &other) const noexcept;

#if !CPP_TS_HAS_CXX20
        [[nodiscard]] bool operator!=(const TreeCursor &other) const noexcept
        {
            return !(*this == other);
        }
#endif // !CPP_TS_HAS_CXX20

        TreeCursor &operator=(const TreeCursor &other) = delete;

        TreeCursor &operator=(TreeCursor &&other) noexcept
        {
            if (this != &other)
            {
                if (isValid())
                {
                    ts_tree_cursor_delete(&impl_);
                }

                impl_     = other.impl_;
                is_valid_ = other.is_valid_;

                other.is_valid_ = false;
            }

            return *this;
        }

    private:
        TSTreeCursor impl_{};
        bool         is_valid_ = false;
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCapture
    // A capture of a specific node by a specific name within a query pattern.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryCapture
    {
        Node          node;
        std::uint32_t index;

        constexpr QueryCapture() noexcept : node(Node::null()), index(0)
        {}

        // For easy conversion from C API
        constexpr QueryCapture(const TSQueryCapture &capture) noexcept : node(capture.node), index(capture.index)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryMatch
    // A single match found by a query, containing one or more captures.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryMatch
    {
        std::uint32_t id;
        std::uint16_t pattern_index;

        // A vector is used for easy access; however, note that data copying occurs only when the QueryMatch object is
        // instantiated.
        std::vector<QueryCapture> captures;

        QueryMatch() noexcept : id(0), pattern_index(0), captures()
        {}

        explicit QueryMatch(const TSQueryMatch &match) : id(match.id), pattern_index(match.pattern_index)
        {
            if (match.capture_count > 0 && match.captures != nullptr)
            {
                captures.assign(match.captures, match.captures + match.capture_count);
            }
            else
            {
                captures = {};
            }
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursorState
    // The current state of an active query cursor.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryCursorState
    {
        std::uint32_t current_byte_offset;

        constexpr explicit QueryCursorState(const TSQueryCursorState *state) noexcept
            : current_byte_offset(state->current_byte_offset)
        {}

        constexpr explicit QueryCursorState(const TSQueryCursorState &state) noexcept
            : current_byte_offset(state.current_byte_offset)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursorOptions
    // Configuration options for a query cursor, such as progress callbacks.
    /////////////////////////////////////////////////////////////////////////////

    using QueryCursorOptions = details::OptionsBase<TSQueryCursorOptions, TSQueryCursorState, QueryCursorState>;

    /////////////////////////////////////////////////////////////////////////////
    // QueryPredicateStep
    // A single step in a predicate defined within a query pattern.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryPredicateStep
    {
        std::uint32_t          value_id;
        QueryPredicateStepType type;

        constexpr QueryPredicateStep(const TSQueryPredicateStep *predicate_step) noexcept
            : value_id(predicate_step->value_id), type(static_cast<QueryPredicateStepType>(predicate_step->type))
        {}

        constexpr QueryPredicateStep(const TSQueryPredicateStep &predicate_step) noexcept
            : value_id(predicate_step.value_id), type(static_cast<QueryPredicateStepType>(predicate_step.type))
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // Query
    // Compiled set of patterns used to search for structures in a syntax tree.
    /////////////////////////////////////////////////////////////////////////////

    class Query
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        Query(LanguageView language, details::StringViewParameter source);

#if CPP_TS_HAS_CXX23
        [[nodiscard]] static std::expected<Query, std::string> create(LanguageView                 language,
                                                                      details::StringViewParameter source) noexcept;
#endif // CPP_TS_HAS_CXX23

        ////////////////////////////////////////////////////////////////
        // Counts
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::uint32_t getPatternCount() const noexcept
        {
            return ts_query_pattern_count(impl_.get());
        }

        [[nodiscard]] std::uint32_t getCaptureCount() const noexcept
        {
            return ts_query_capture_count(impl_.get());
        }

        [[nodiscard]] std::uint32_t getStringCount() const noexcept
        {
            return ts_query_string_count(impl_.get());
        }

        ////////////////////////////////////////////////////////////////
        // Pattern Information
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool isPatternRooted(std::uint32_t pattern_index) const noexcept
        {
            return ts_query_is_pattern_rooted(impl_.get(), pattern_index);
        }

        [[nodiscard]] bool isPatternNonLocal(std::uint32_t pattern_index) const noexcept
        {
            return ts_query_is_pattern_non_local(impl_.get(), pattern_index);
        }

        [[nodiscard]] bool isPatternGuaranteedAtStep(std::uint32_t byte_offset) const noexcept
        {
            return ts_query_is_pattern_guaranteed_at_step(impl_.get(), byte_offset);
        }

        [[nodiscard]] Extent<std::uint32_t> getByteRangeForPattern(std::uint32_t pattern_index) const noexcept
        {
            return { ts_query_start_byte_for_pattern(impl_.get(), pattern_index),
                     ts_query_end_byte_for_pattern(impl_.get(), pattern_index) };
        }

        ////////////////////////////////////////////////////////////////
        // Predicates
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<QueryPredicateStep> getAllPredicatesForPattern(std::uint32_t pattern_index) const;

        ////////////////////////////////////////////////////////////////
        // Capture & String Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getCaptureNameForID(std::uint32_t id) const noexcept
        {
            std::uint32_t length;
            const char   *name = ts_query_capture_name_for_id(impl_.get(), id, &length);
            return details::make_view(name, static_cast<std::size_t>(length));
        }

        [[nodiscard]] Quantifier getCaptureQuantifierForID(std::uint32_t pattern_index,
                                                           std::uint32_t capture_index) const
        {
            return static_cast<Quantifier>(
                    ts_query_capture_quantifier_for_id(impl_.get(), pattern_index, capture_index));
        }

        [[nodiscard]] details::StringViewReturn getStringValueForID(std::uint32_t id) const noexcept
        {
            std::uint32_t length;
            const char   *name = ts_query_string_value_for_id(impl_.get(), id, &length);
            return details::make_view(name, static_cast<std::size_t>(length));
        }

        ////////////////////////////////////////////////////////////////
        // Modification
        ////////////////////////////////////////////////////////////////

        void disableCapture(details::StringViewParameter name);

        void disablePattern(std::uint32_t pattern_index) noexcept
        {
            ts_query_disable_pattern(impl_.get(), pattern_index);
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] operator const TSQuery *() const noexcept
        {
            return impl_.get();
        }

    private:
#if CPP_TS_HAS_CXX23
        explicit Query(TSQuery *raw_query) : impl_(raw_query, ts_query_delete)
        {}
#endif // CPP_TS_HAS_CXX23

    private:
        std::unique_ptr<TSQuery, decltype(&ts_query_delete)> impl_{ nullptr, ts_query_delete };
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursor
    // Executes queries and iterates over matches.
    /////////////////////////////////////////////////////////////////////////////

    class QueryCursor
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        QueryCursor() noexcept : impl_{ ts_query_cursor_new() }
        {}

        // Copying is not supported by the underlying C API.
        QueryCursor(const QueryCursor &) = delete;

        QueryCursor(QueryCursor &&) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // Limits
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::uint32_t getMatchLimit() noexcept
        {
            return ts_query_cursor_match_limit(impl_.get());
        }

        void setMatchLimit(std::uint32_t limit) noexcept
        {
            ts_query_cursor_set_match_limit(impl_.get(), limit);
        }

        [[nodiscard]] bool didExceedMatchLimit() noexcept
        {
            return ts_query_cursor_did_exceed_match_limit(impl_.get());
        }

        void setMaxStartDepth(std::uint32_t max_start_depth) noexcept
        {
            ts_query_cursor_set_max_start_depth(impl_.get(), max_start_depth);
        }

        void resetMaxStartDepth() noexcept
        {
            ts_query_cursor_set_max_start_depth(impl_.get(), std::numeric_limits<std::uint32_t>::max());
        }

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool setByteRange(Extent<std::uint32_t> range) noexcept
        {
            return ts_query_cursor_set_byte_range(impl_.get(), range.start, range.end);
        }

        [[nodiscard]] bool setPointRange(Extent<Point> range) noexcept
        {
            return ts_query_cursor_set_point_range(impl_.get(), range.start, range.end);
        }

        [[nodiscard]] bool setContainingByteRange(Extent<std::uint32_t> range) noexcept
        {
            return ts_query_cursor_set_containing_byte_range(impl_.get(), range.start, range.end);
        }

        [[nodiscard]] bool setContainingPointRange(Extent<Point> range) noexcept
        {
            return ts_query_cursor_set_containing_point_range(impl_.get(), range.start, range.end);
        }

        ////////////////////////////////////////////////////////////////
        // Execution
        ////////////////////////////////////////////////////////////////

        void exec(const Query &query, Node node, details::OptionalParam<const QueryCursorOptions> query_options = {});

        ////////////////////////////////////////////////////////////////
        // Iteration
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool nextMatch(QueryMatch &match) noexcept
        {
            TSQueryMatch c_match{};
            if (ts_query_cursor_next_match(impl_.get(), &c_match))
            {
                match = QueryMatch(c_match);
                return true;
            }
            return false;
        }

        [[nodiscard]] bool nextCapture(QueryMatch &match, std::uint32_t &capture_index) noexcept
        {
            TSQueryMatch  c_match{};
            std::uint32_t c_capture_index = 0;

            if (ts_query_cursor_next_capture(impl_.get(), &c_match, &c_capture_index))
            {
                match         = QueryMatch(c_match);
                capture_index = c_capture_index;
                return true;
            }
            return false;
        }

        void removeMatch(std::uint32_t match_id) noexcept
        {
            return ts_query_cursor_remove_match(impl_.get(), match_id);
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        // Copying is not supported by the underlying C API.
        QueryCursor &operator=(const QueryCursor &) = delete;

        QueryCursor &operator=(QueryCursor &&) noexcept = default;

    private:
        std::unique_ptr<TSQueryCursor, details::FreeHelper> impl_{ nullptr };
    };

    /////////////////////////////////////////////////////////////////////////////
    // LookaheadIterator
    // Iterates over possible symbols in a given parse state.
    /////////////////////////////////////////////////////////////////////////////

    class LookaheadIterator
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        LookaheadIterator(LanguageView language, StateID state);

        // Copying is not supported by the underlying C API.
        LookaheadIterator(const LookaheadIterator &) = delete;

        LookaheadIterator(LookaheadIterator &&other) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // State Control
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool resetState(StateID state) noexcept
        {
            return ts_lookahead_iterator_reset_state(impl_.get(), state);
        }

        [[nodiscard]] bool reset(LanguageView language, StateID state) noexcept
        {
            return ts_lookahead_iterator_reset(impl_.get(), language, state);
        }

        ////////////////////////////////////////////////////////////////
        // Property Accessors
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] LanguageView getLanguage() const noexcept
        {
            return LanguageView(ts_lookahead_iterator_language(impl_.get()));
        }

        [[nodiscard]] Symbol getCurrentSymbol() const noexcept
        {
            return ts_lookahead_iterator_current_symbol(impl_.get());
        }

        [[nodiscard]] details::StringViewReturn getCurrentSymbolName() const
        {
            return details::make_view(ts_lookahead_iterator_current_symbol_name(impl_.get()));
        }

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool next() noexcept
        {
            return ts_lookahead_iterator_next(impl_.get());
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        // Copying is not supported by the underlying C API.
        LookaheadIterator &operator=(const LookaheadIterator &) = delete;

        LookaheadIterator &operator=(LookaheadIterator &&other) noexcept = default;

        [[nodiscard]] operator const TSLookaheadIterator *() const noexcept
        {
            return impl_.get();
        }

    private:
        std::unique_ptr<TSLookaheadIterator, details::FreeHelper> impl_{ nullptr };
    };

#if defined(CPP_TS_FEATURE_WASM)
    /////////////////////////////////////////////////////////////////////////////
    // WasmStore
    // Manages WebAssembly environments and loading Wasm-based languages.
    /////////////////////////////////////////////////////////////////////////////

    class WasmStore
    {
    public:
        //////////////////////////////////////////////////////////////
        // Lifecycle
        //////////////////////////////////////////////////////////////

        explicit WasmStore(WasmEngine *engine);

        explicit WasmStore(TSWasmStore *store) noexcept
            : impl_{ store, ts_wasm_store_delete }, is_valid_(store != nullptr)
        {}

        // Copying is not supported by the underlying C API.
        WasmStore(const WasmStore &) = delete;

        WasmStore(WasmStore &&other) noexcept;

#if CPP_TS_HAS_CXX23
        [[nodiscard]] static std::expected<WasmStore, std::string> create(WasmEngine *engine) noexcept;
#endif // CPP_TS_HAS_CXX23

        //////////////////////////////////////////////////////////////
        // Methods
        //////////////////////////////////////////////////////////////

        [[nodiscard]] Language loadLanguage(details::StringViewParameter name,
                                            details::StringViewParameter wasm_buffer);

        [[nodiscard]] Language loadLanguage(details::StringViewParameter file_path);

#if CPP_TS_HAS_CXX23
        [[nodiscard]] std::expected<Language, std::string> loadLanguageExpected(
                details::StringViewParameter name,
                details::StringViewParameter wasm_buffer) noexcept;

        [[nodiscard]] std::expected<Language, std::string> loadLanguageExpected(details::StringViewParameter file_path);
#endif // CPP_TS_HAS_CXX23

        [[nodiscard]] std::size_t getLanguageCount() const;

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return is_valid_;
        }

        //////////////////////////////////////////////////////////////
        // Operators
        //////////////////////////////////////////////////////////////

        // Copying is not supported by the underlying C API.
        WasmStore &operator=(const WasmStore &) = delete;

        WasmStore &operator=(WasmStore &&other) noexcept;

        [[nodiscard]] operator TSWasmStore *() const noexcept
        {
            return impl_.get();
        }

    private:
        void setInvalid() noexcept
        {
            // We need to set impl as nullptr but don't call deleter on stored pointer
            impl_.release();
            is_valid_ = false;
        }

    private:
        std::unique_ptr<TSWasmStore, decltype(&ts_wasm_store_delete)> impl_{ nullptr, ts_wasm_store_delete };
        bool                                                          is_valid_ = false;

        friend class Parser;
    };
#endif // defined(CPP_TS_FEATURE_WASM)

    /////////////////////////////////////////////////////////////////////////////
    // Child Node Iterators
    // STL-compatible iterators for processing Node children.
    /////////////////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
    class ChildIteratorSentinel
    {};
#endif // CPP_TS_HAS_CXX17

    class ChildIterator
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Aliases
        ////////////////////////////////////////////////////////////////

        using value_type        = Node;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        class Child
        {
        public:
            explicit Child(value_type v) noexcept : value_(v)
            {}

#if CPP_TS_HAS_CXX23
            template <typename Self>
            [[nodiscard]] auto *operator->(this Self &&self) noexcept
            {
                return &self.value_;
            }
#else
            [[nodiscard]] const value_type *operator->() const noexcept
            {
                return &value_;
            }

            [[nodiscard]] value_type *operator->() noexcept
            {
                return &value_;
            }
#endif // CPP_TS_HAS_CXX23

        private:
            value_type value_;
        };

        class PostfixProxy
        {
        public:
            explicit PostfixProxy(value_type v) noexcept : value_(v)
            {}

            [[nodiscard]] value_type operator*() const noexcept
            {
                return value_;
            }

            Child operator->() const noexcept
            {
                return Child(value_);
            }

        private:
            value_type value_;
        };

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        ChildIterator() : atEnd_(true)
        {}

        explicit ChildIterator(const Node &node) noexcept
            : cursor_(node.getCursor()), atEnd_(node.isNull() || !cursor_.gotoFirstChild())
        {}

        // Copying is not supported by the underlying C API.
        ChildIterator(const ChildIterator &) = delete;

        ChildIterator(ChildIterator &&) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // Get
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] value_type get() const noexcept
        {
            return cursor_.getCurrentNode();
        }

        [[nodiscard]] value_type operator*() const noexcept
        {
            return get();
        }

        Child operator->() const noexcept
        {
            // Dereference this iterator to get the value,
            // then wrap it in the safe proxy.
            return Child(get());
        }

        ////////////////////////////////////////////////////////////////
        // Advance
        ////////////////////////////////////////////////////////////////

        ChildIterator &operator++() noexcept
        {
            if (!atEnd_)
            {
                atEnd_ = !cursor_.gotoNextSibling();
            }
            return *this;
        }

        PostfixProxy operator++(int) noexcept
        {
            PostfixProxy proxy(get());
            ++(*this);
            return proxy;
        }

        ////////////////////////////////////////////////////////////////
        // Retreat
        ////////////////////////////////////////////////////////////////

        ChildIterator &operator--() noexcept
        {
            if (atEnd_)
            {
                atEnd_ = false;
            }
            else
            {
#if CPP_TS_HAS_CXX17
                [[maybe_unused]]
#endif // CPP_TS_HAS_CXX17
                bool prev = cursor_.gotoPreviousSibling();

#if !CPP_TS_HAS_CXX17
                static_cast<void>(prev);
#endif // !CPP_TS_HAS_CXX17
            }
            return *this;
        }

        PostfixProxy operator--(int) noexcept
        {
            PostfixProxy proxy(get());
            --(*this);
            return proxy;
        }

        ////////////////////////////////////////////////////////////////
        // Sentinels Comparision
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
        [[nodiscard]] friend bool operator==(const ChildIterator &a, const ChildIteratorSentinel &) noexcept
        {
            return a.atEnd_;
        }

#if !CPP_TS_HAS_CXX20
        [[nodiscard]] friend bool operator!=(const ChildIterator &a, const ChildIteratorSentinel &b) noexcept
        {
            return !(a == b);
        }
#endif // !CPP_TS_HAS_CXX20

        [[nodiscard]] friend bool operator==(const ChildIteratorSentinel &a, const ChildIterator &b) noexcept
        {
            return b == a;
        }

#if !CPP_TS_HAS_CXX20
        [[nodiscard]] friend bool operator!=(const ChildIteratorSentinel &a, const ChildIterator &b) noexcept
        {
            return b != a;
        }
#endif // !CPP_TS_HAS_CXX20
#endif // CPP_TS_HAS_CXX17

        ////////////////////////////////////////////////////////////////
        // Comparision
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] friend bool operator==(const ChildIterator &a, const ChildIterator &b) noexcept
        {
            if (a.atEnd_ && b.atEnd_)
            {
                return true;
            }

            if (a.atEnd_ != b.atEnd_)
            {
                return false;
            }

            return a.cursor_ == b.cursor_;
        }

#if !CPP_TS_HAS_CXX20
        [[nodiscard]] friend bool operator!=(const ChildIterator &a, const ChildIterator &b) noexcept
        {
            return !(a == b);
        }
#endif // !CPP_TS_HAS_CXX20

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        // Copying is not supported by the underlying C API.
        ChildIterator &operator=(const ChildIterator &) = delete;

        ChildIterator &operator=(ChildIterator &&) noexcept = default;

    private:
        TreeCursor cursor_;
        bool       atEnd_ = false;
    };

#if CPP_TS_HAS_CXX20
    struct Children : public std::ranges::view_interface<Children>
#else
    struct Children
#endif // CPP_TS_HAS_CXX20
    {
        using iterator = ChildIterator;
#if CPP_TS_HAS_CXX17
        using sentinel = ChildIteratorSentinel;
#else
        using sentinel = ChildIterator;
#endif // CPP_TS_HAS_CXX17

        ts::Node node_ref;

        explicit Children(const ts::Node &n) noexcept : node_ref(n)
        {}

        [[nodiscard]] iterator begin() const noexcept
        {
            return iterator(node_ref);
        }

        [[nodiscard]] sentinel end() const noexcept
        {
            return sentinel();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return static_cast<std::size_t>(node_ref.getChildCount());
        }
    };

#if CPP_TS_HAS_CXX20
    static_assert(std::input_iterator<ChildIterator>);
    static_assert(std::sentinel_for<ChildIteratorSentinel, ChildIterator>);
#endif // CPP_TS_HAS_CXX20

    /////////////////////////////////////////////////////////////////////////////
    // Visitor
    // High-level utility for depth-first traversal of a syntax tree.
    /////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Concepts
    ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX20
    template <typename T>
    concept VisitorConcept = requires {
        { std::declval<T>()(std::declval<Node>()) } -> std::convertible_to<bool>;
    };
#elif CPP_TS_HAS_CXX17
    template <typename T, typename = void>
    struct VisitorConcept : std::false_type
    {};

    template <typename T>
    struct VisitorConcept<T,
                          std::void_t<std::enable_if_t<
                                  std::is_convertible_v<decltype(std::declval<T>()(std::declval<Node>())), bool>>>>
        : std::true_type
    {};
#endif // CPP_TS_HAS_CXX20 / CPP_TS_HAS_CXX17

    ////////////////////////////////////////////////////////////////
    // Function
    ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX20
    template <VisitorConcept F>
#elif CPP_TS_HAS_CXX17
    template <typename F, std::enable_if_t<VisitorConcept<F>::value, bool> = true>
#else
    template <typename F>
#endif // CPP_TS_HAS_CXX20 / CPP_TS_HAS_CXX17
    // Callback should return `true` if wants to break from loop.
    void visit(Node root, F &&callback)
    {
        if (root.isNull())
        {
            return;
        }

        TreeCursor          cursor      = root.getCursor();
        const std::uint32_t start_depth = cursor.getDepthFromOrigin();

        while (true)
        {
            if (callback(cursor.getCurrentNode()))
            {
                break;
            }

            // 1. Go to first child (down)
            if (cursor.gotoFirstChild())
            {
                continue;
            }

            // 2. If there is no child go to neighbour (sideway)
            if (cursor.gotoNextSibling())
            {
                continue;
            }

            // 3. If no child and neighbours go up if you find next neighbours but not before `root`
            bool found_next = false;
            while (cursor.getDepthFromOrigin() > start_depth)
            {
                if (cursor.gotoParent())
                {
                    if (cursor.gotoNextSibling())
                    {
                        found_next = true;
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            if (!found_next)
            {
                break; // Everything was checked
            }
        }
    }

} // namespace ts

#if defined(CPP_TS_HEADER_ONLY)
#include "cpp-tree-sitter.inl"
#endif // defined(CPP_TS_HEADER_ONLY)

#endif // CPP_TREE_SITTER_HPP
