#pragma once
#ifndef CPP_TREE_SITTER_HPP
#define CPP_TREE_SITTER_HPP

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// Standard Compatibility Macros
/////////////////////////////////////////////////////////////////////////////

#if defined(_MSVC_LANG)
#define CPP_STANDARD _MSVC_LANG
#else
#define CPP_STANDARD __cplusplus
#endif

#if CPP_STANDARD >= 202'002L
#include <concepts>
#include <utility>
#define TS_CXX_20
#elif CPP_STANDARD >= 201'703L
#include <type_traits>
#include <utility>
#define TS_CXX_17
#endif

#if defined(TS_CXX_17) || defined(TS_CXX_20)
#include <optional>
#include <string_view>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
// On Windows we get descriptor using _fileno
#include <io.h>
#define TS_FILENO _fileno
#else
// On POSIX systems we use fileno z <cstdio>
#include <cstdio>
#define TS_FILENO fileno
#endif

#include <tree_sitter/api.h>

namespace ts
{
    /////////////////////////////////////////////////////////////////////////////
    // Forward Declarations
    /////////////////////////////////////////////////////////////////////////////

    struct Point;
    struct InputEdit;
    struct Node;
    class TreeCursor;
    class LookaheadIterator;

    /////////////////////////////////////////////////////////////////////////////
    // Helper Classes & Structs
    // Internal utilities for memory management and range representation.
    /////////////////////////////////////////////////////////////////////////////

    struct FreeHelper
    {
        template <typename T>
        void operator()(T *raw_pointer) const
        {
            std::free(raw_pointer);
        }
    };

    // An inclusive range representation
    template <typename T>
    struct Extent
    {
        T start;
        T end;
    };

#if !defined(TS_CXX_17) && !defined(TS_CXX_20)
    class StringView
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        constexpr StringView() noexcept : data_(nullptr), size_(0)
        {}

        constexpr StringView(const char *s, size_t count) : data_(s), size_(count)
        {}

        StringView(const char *s) : data_(s), size_(s ? std::strlen(s) : 0)
        {}

        StringView(const std::string &s) noexcept : data_(s.data()), size_(s.size())
        {}

        ////////////////////////////////////////////////////////////////
        // Data
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr const char *data() const noexcept
        {
            return data_;
        }

        [[nodiscard]] constexpr const char *begin() const noexcept
        {
            return data_;
        }

        [[nodiscard]] constexpr const char *end() const noexcept
        {
            return data_ + size_;
        }

        ////////////////////////////////////////////////////////////////
        // Size
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return size_;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return size_ == 0;
        }

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] StringView substr(size_t pos, size_t n) const
        {
            return { data_ + pos, n };
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] constexpr char operator[](size_t pos) const
        {
            return data_[pos];
        }

        explicit operator std::string() const
        {
            return std::string(data_, size_);
        }

    private:
        const char *data_;
        size_t      size_;
    };
#endif

    /////////////////////////////////////////////////////////////////////////////
    // Aliases.
    // Create slightly stricter aliases for some of the core tree-sitter types.
    /////////////////////////////////////////////////////////////////////////////

    // Direct alias of { major_version: uint8_t; minor_version: uint8_t; patch_version: uint8_t }
    using LanguageMetadata = TSLanguageMetadata;

    using WasmEngine = wasm_engine_t;
    using StateID    = uint16_t;
    using Symbol     = uint16_t;
    using FieldID    = uint16_t;
    using Version    = uint32_t;
    using NodeID     = uintptr_t;

    namespace details
    {
#if defined(TS_CXX_17) || defined(TS_CXX_20)
        using StringViewParameter = const std::string_view;
        using StringViewReturn    = std::string_view;
        using ByteViewU8_t        = const std::basic_string_view<uint8_t>;

        template <typename T>
        using OptionalParam = std::optional<std::reference_wrapper<T>>;

        template <typename T, typename Raw>
        [[nodiscard]] static Raw *get_raw(OptionalParam<T> opt)
        {
            return opt.has_value() ? static_cast<Raw *>(static_cast<void *>(&opt->get())) : nullptr;
        }
#else
        using StringViewParameter = const std::string &;
        using StringViewReturn    = StringView;

        template <typename T>
        using OptionalParam = T *;

        template <typename T, typename Raw>
        [[nodiscard]] static Raw *get_raw(OptionalParam<T> opt)
        {
            return opt ? static_cast<Raw *>(static_cast<void *>(opt)) : nullptr;
        }
#endif

        struct QueryErrorHelper
        {
            static void validate(uint32_t error_offset, TSQueryError &error)
            {
                if (error == TSQueryErrorNone)
                {
                    return;
                }

                StringViewReturn type_name;
                switch (error)
                {
                    case TSQueryErrorSyntax:
                        type_name = "Syntax";
                        break;
                    case TSQueryErrorNodeType:
                        type_name = "Node Type";
                        break;
                    case TSQueryErrorField:
                        type_name = "Field";
                        break;
                    case TSQueryErrorCapture:
                        type_name = "Capture";
                        break;
                    case TSQueryErrorStructure:
                        type_name = "Structure";
                        break;
                    case TSQueryErrorLanguage:
                        type_name = "Language";
                        break;
                    default:
                        type_name = "Unknown";
                        break;
                }

                throw std::runtime_error("Tree-sitter Query Error at offset " + std::to_string(error_offset)
                                         + " (Type: " + type_name + ")");
            }
        };

        struct WasmErrorHelper
        {
            static void validate(TSWasmError &error)
            {
                if (error.kind == TSWasmErrorKindNone)
                {
                    return;
                }

                StringViewReturn kind_name;
                switch (error.kind)
                {
                    case TSWasmErrorKindParse:
                        kind_name = "Parse";
                        break;
                    case TSWasmErrorKindCompile:
                        kind_name = "Compile";
                        break;
                    case TSWasmErrorKindInstantiate:
                        kind_name = "Instantiate";
                        break;
                    case TSWasmErrorKindAllocate:
                        kind_name = "Allocate";
                        break;
                    default:
                        kind_name = "Unknown";
                        break;
                }

                std::string message = error.message ? error.message : "No message";

                if (error.message)
                {
                    std::free(error.message);
                }

                throw std::runtime_error("Tree-sitter Wasm Error [" + std::string(kind_name) + "]: " + message);
            }
        };

        [[nodiscard]] inline StringViewReturn make_view(const char *str)
        {
            return str ? StringViewReturn(str) : StringViewReturn();
        }

        [[nodiscard]] inline StringViewReturn make_view(const char *str, uint32_t length)
        {
            return (str && length > 0) ? StringViewReturn(str) : StringViewReturn();
        }
    } // namespace details

#if defined(TS_CXX_17) || defined(TS_CXX_20)
    using DecodeFunction = std::function<uint32_t(details::ByteViewU8_t, int32_t *)>;
#else
    using DecodeFunction = std::function<uint32_t(const uint8_t *, uint32_t, int32_t *)>;
#endif

    /////////////////////////////////////////////////////////////////////////////
    // Enums.
    // Scoped enum wrappers for Tree-sitter constants to ensure type safety.
    /////////////////////////////////////////////////////////////////////////////

    enum class InputEncoding : uint8_t
    {
        UTF8    = TSInputEncodingUTF8,
        UTF16LE = TSInputEncodingUTF16LE,
        UTF16BE = TSInputEncodingUTF16BE,
        Custom  = TSInputEncodingCustom
    };

    enum class SymbolType : uint8_t
    {
        TypeRegular   = TSSymbolTypeRegular,
        TypeAnonymous = TSSymbolTypeAnonymous,
        TypeSupertype = TSSymbolTypeSupertype,
        TypeAuxiliary = TSSymbolTypeAuxiliary,
    };

    enum class LogType : uint8_t
    {
        Parse = TSLogTypeParse,
        Lex   = TSLogTypeLex
    };

    enum class Quantifier : uint8_t
    {
        Zero       = TSQuantifierZero,
        ZeroOrOne  = TSQuantifierZeroOrOne,
        ZeroOrMore = TSQuantifierZeroOrMore,
        One        = TSQuantifierOne,
        OneOrMore  = TSQuantifierOneOrMore,
    };

    enum class QueryPredicateStepType : uint8_t
    {
        Done    = TSQueryPredicateStepTypeDone,
        Capture = TSQueryPredicateStepTypeCapture,
        String  = TSQueryPredicateStepTypeString
    };

    /////////////////////////////////////////////////////////////////////////////
    // Point
    // A location in source code in terms of rows and columns.
    /////////////////////////////////////////////////////////////////////////////

    struct Point
    {
        uint32_t row;
        uint32_t column;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        Point(const TSPoint &other) : row(other.row), column(other.column)
        {}

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        // Definition deferred until after the definition of InputEdit.
        void edit(const InputEdit &edit, uint32_t &byte_offset);

        ////////////////////////////////////////////////////////////////
        // Comparision
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool operator==(const Point &other) const
        {
            return row == other.row && column == other.column;
        }

        [[nodiscard]] bool operator!=(const Point &other) const
        {
            return !(*this == other);
        }

        [[nodiscard]] bool operator>(const Point &other) const
        {
            return (row > other.row) || (row == other.row && column > other.column);
        }

        [[nodiscard]] bool operator<=(const Point &other) const
        {
            return !(*this > other);
        }

        [[nodiscard]] bool operator<(const Point &other) const
        {
            return (row < other.row) || (row == other.row && column < other.column);
        }

        [[nodiscard]] bool operator>=(const Point &other) const
        {
            return !(*this < other);
        }

        [[nodiscard]] static void is_valid_range(const Point &start, const Point &end)
        {
            return start <= end;
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        operator TSPoint() const
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
        uint32_t start_byte;
        uint32_t old_end_byte;
        uint32_t new_end_byte;
        Point    start_point;
        Point    old_end_point;
        Point    new_end_point;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        explicit InputEdit(const TSInputEdit &edit)
            : start_byte(edit.start_byte)
            , old_end_byte(edit.old_end_byte)
            , new_end_byte(edit.new_end_byte)
            , start_point(edit.start_point)
            , old_end_point(edit.old_end_point)
            , new_end_point(edit.new_end_point)
        {}

        ////////////////////////////////////////////////////////////////
        // Validation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] void validate() const
        {
            bool bytes_invalid = start_byte > old_end_byte;

            bool points_invalid = Point::is_valid_range(start_point, old_end_point);

            if (bytes_invalid || points_invalid)
            {
                throw std::invalid_argument("Tree-sitter: Invalid edit ranges (start > old_end)");
            }
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        operator TSInputEdit() const
        {
            return { start_byte, old_end_byte, new_end_byte, start_point, old_end_point, new_end_point };
        }
    };

    inline void Point::edit(const InputEdit &edit, uint32_t &byte_offset)
    {
        edit.validate();

        TSPoint raw_point = static_cast<TSPoint>(*this);
        ts_point_edit(&raw_point, &byte_offset, &edit);

        row    = raw_point.row;
        column = raw_point.column;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Range
    // Representation of a range within the source code using points and bytes.
    /////////////////////////////////////////////////////////////////////////////

    struct Range
    {
        Extent<Point>    point;
        Extent<uint32_t> byte;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        explicit Range(const TSRange &range)
            : point(range.start_point, range.end_point), byte(range.start_byte, range.end_byte)
        {}

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        void edit(const InputEdit &edit)
        {
            edit.validate(edit);

            TSRange raw_range = static_cast<TSRange>(*this);

            ts_range_edit(&raw_range, &edit);

            point.start = raw_range.start_point;
            point.end   = raw_range.end_point;
            byte.start  = raw_range.start_byte;
            byte.end    = raw_range.end_byte;
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        operator TSRange() const
        {
            return { point.start, point.end, byte.start, byte.end };
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // Input
    // A structure that defines how to read and decode source code incrementally.
    /////////////////////////////////////////////////////////////////////////////

    struct Input
    {
        using ReadFunction = std::function<details::StringViewReturn(uint32_t, Point, uint32_t *)>;

        ReadFunction       read;
        InputEncoding      encoding;
        ts::DecodeFunction decode;

        static thread_local const Input *current_input_ptr;

        ////////////////////////////////////////////////////////////////
        // Static Functions
        ////////////////////////////////////////////////////////////////

        static const char *read_proxy(void *payload, uint32_t byte_index, TSPoint position, uint32_t *bytes_read)
        {
            auto *self = static_cast<Input *>(payload);
            if (self && self->read)
            {
                details::StringViewReturn result = self->read(byte_index, position, bytes_read);
                return result.data();
            }
            *bytes_read = 0;
            return nullptr;
        }

        static uint32_t decode_proxy(const uint8_t *string, uint32_t length, int32_t *code_point)
        {
            if (current_input_ptr && current_input_ptr->decode)
            {
#if defined(TS_CXX_17) || defined(TS_CXX_20)
                // basic_string_view<uint8_t>, int32_t *
                return current_input_ptr->decode({ string, length }, code_point);
#else
                // const uint8_t*, uint32_t, int32_t *
                return current_input_ptr->decode(string, length, code_point);
#endif
            }
            return 0;
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        operator TSInput() const
        {
            TSInput c_input{};
            c_input.payload  = const_cast<Input *>(this);
            c_input.read     = read_proxy;
            c_input.encoding = static_cast<TSInputEncoding>(encoding);
            c_input.decode   = (decode) ? decode_proxy : nullptr;
            return c_input;
        }
    };

    inline thread_local const Input *Input::current_input_ptr = nullptr;

    /////////////////////////////////////////////////////////////////////////////
    // ParseState
    // A structure representing the state of the parser at a specific position.
    /////////////////////////////////////////////////////////////////////////////

    struct ParseState
    {
        uint32_t current_byte_offset;
        bool     has_error;

        explicit ParseState(const TSParseState *state)
            : current_byte_offset(state->current_byte_offset), has_error(state->has_error)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // ParseOptions
    // Options that control the behavior of the parsing process.
    /////////////////////////////////////////////////////////////////////////////

    struct ParseOptions
    {
        using ProgressCallbackFunction = std::function<bool(ParseState *)>;

        ProgressCallbackFunction progress_callback;

        static bool progress_callback_proxy(TSParseState *state)
        {
            if (state && state->payload)
            {
                auto *self = static_cast<ParseOptions *>(state->payload);
                if (self->progress_callback)
                {
                    ParseState cpp_state(state);
                    return self->progress_callback(&cpp_state);
                }
            }
            return false;
        }

        operator TSParseOptions() const
        {
            TSParseOptions options{};
            options.payload           = const_cast<ParseOptions *>(this);
            options.progress_callback = (progress_callback) ? progress_callback_proxy : nullptr;
            return options;
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCapture
    // A capture of a specific node by a specific name within a query pattern.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryCapture
    {
        Node     node;
        uint32_t index;

        // For easy conversion from C API
        explicit QueryCapture(const TSQueryCapture &capture) : node(capture.node), index(capture.index)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryMatch
    // A single match found by a query, containing one or more captures.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryMatch
    {
        uint32_t id;
        uint16_t pattern_index;

        // A vector is used for easy access; however, note that data copying occurs only when the QueryMatch object is
        // instantiated.
        std::vector<QueryCapture> captures;

        explicit QueryMatch(const TSQueryMatch &match) : id(match.id), pattern_index(match.pattern_index)
        {
            captures.reserve(match.capture_count);
            for (uint32_t i = 0; i < match.capture_count; ++i)
            {
                captures.emplace_back(match.captures[i]);
            }
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursorState
    // The current state of an active query cursor.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryCursorState
    {
        uint32_t current_byte_offset;

        explicit QueryCursorState(const TSQueryCursorState *state) : current_byte_offset(state->current_byte_offset)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursorOptions
    // Configuration options for a query cursor, such as progress callbacks.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryCursorOptions
    {
        using ProgressCallbackFunction = std::function<bool(QueryCursorState *)>;

        ProgressCallbackFunction progress_callback;

        static bool progress_callback_proxy(TSQueryCursorState *state)
        {
            if (state && state->payload)
            {
                auto *self = static_cast<QueryCursorOptions *>(state->payload);
                if (self->progress_callback)
                {
                    QueryCursorState cpp_state(state);
                    return self->progress_callback(&cpp_state);
                }
            }
            return false;
        }

        operator TSQueryCursorOptions() const
        {
            TSQueryCursorOptions options{};
            options.payload           = const_cast<ParseOptions *>(this);
            options.progress_callback = (progress_callback) ? progress_callback_proxy : nullptr;
            return options;
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // QueryPredicateStep
    // A single step in a predicate defined within a query pattern.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryPredicateStep
    {
        QueryPredicateStepType type;
        uint32_t               value_id;

        explicit QueryPredicateStep(const TSQueryPredicateStep *predicate_step)
            : type(static_cast<QueryPredicateStepType>(predicate_step->type)), value_id(predicate_step->value_id)
        {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // Language
    // Represents a Tree-sitter grammar with metadata and symbol definitions.
    /////////////////////////////////////////////////////////////////////////////

    class Language
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        // NOTE: Allowing implicit conversions from TSLanguage to Language
        // improves ergonomics for clients using external grammar providers.

        /* implicit */ Language(const TSLanguage *language)
            : impl{ language, [](const TSLanguage *l) { ts_language_delete(l); } }
        {}

        Language(const Language &other) : Language(ts_language_copy(other.impl.get()))
        {}

        Language(Language &&other) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // Count Accessors
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] size_t getSymbolsCount() const
        {
            return ts_language_symbol_count(impl.get());
        }

        [[nodiscard]] size_t getStatesCount() const
        {
            return ts_language_state_count(impl.get());
        }

        [[nodiscard]] size_t getFieldsCount() const
        {
            return ts_language_field_count(impl.get());
        }

        ////////////////////////////////////////////////////////////////
        // Symbol Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getSymbolName(Symbol symbol) const
        {
            return details::make_view(ts_language_symbol_name(impl.get(), symbol));
        }

        [[nodiscard]] SymbolType getSymbolType(Symbol symbol) const
        {
            return static_cast<SymbolType>(ts_language_symbol_type(impl.get(), symbol));
        }

        [[nodiscard]] Symbol getSymbolForName(details::StringViewParameter name, bool isNamed) const
        {
            return ts_language_symbol_for_name(impl.get(), name.data(), static_cast<uint32_t>(name.size()), isNamed);
        }

        ////////////////////////////////////////////////////////////////
        // Field Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getFieldNameForID(FieldID id) const
        {
            return details::make_view(ts_language_field_name_for_id(impl.get(), id));
        }

        [[nodiscard]] FieldID getFieldIDForName(details::StringViewParameter name) const
        {
            return ts_language_field_id_for_name(impl.get(), name.data(), static_cast<uint32_t>(name.size()));
        }

        ////////////////////////////////////////////////////////////////
        // Type Hierarchy
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<Symbol> getAllSuperTypes() const
        {
            uint32_t count = 0;
            Symbol  *array = ts_language_supertypes(impl.get(), &count);
            if (!array)
            {
                return {};
            }

            if (count == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Symbol> vec(array, array + count);
            std::free(array);

            return vec;
        }

        [[nodiscard]] std::vector<Symbol> getAllSubTypesForSuperType(Symbol supertype) const
        {
            uint32_t count = 0;
            Symbol  *array = ts_language_subtypes(impl.get(), supertype, &count);
            if (!array)
            {
                return {};
            }

            if (count == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Symbol> vec(array, array + count);
            std::free(array);

            return vec;
        }

        ////////////////////////////////////////////////////////////////
        // State Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] StateID getNextState(StateID state, Symbol symbol) const
        {
            return ts_language_next_state(impl.get(), state, symbol);
        }

        [[nodiscard]] LookaheadIterator getLookaheadIterator(StateID state) const;

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getName() const
        {
            return details::make_view(ts_language_name(impl.get()));
        }

        [[nodiscard]] Version getVersion() const
        {
            return ts_language_abi_version(impl.get());
        }

#if defined(TS_CXX_17) || defined(TS_CXX_20)
        [[nodiscard]] std::optional<LanguageMetadata> getMetadata() const
#else
        [[nodiscard]] LanguageMetadata getMetadata() const
#endif
        {
            const TSLanguageMetadata *metadata = ts_language_metadata(impl.get());
            if (!metadata)
            {
#if defined(TS_CXX_17) || defined(TS_CXX_20)
                return std::nullopt;
#else
                return { 0, 0, 0 };
#endif
            }
            return LanguageMetadata{ metadata->major_version, metadata->minor_version, metadata->patch_version };
        }

        [[nodiscard]] bool isWasm() const
        {
            return ts_language_is_wasm(impl.get());
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        Language &operator=(Language other)
        {
            std::swap(impl, other.impl);
            return *this;
        }

        Language &operator=(Language &&other) noexcept = default;

        [[nodiscard]] operator const TSLanguage *() const
        {
            return impl.get();
        }

    private:
        std::shared_ptr<const TSLanguage> impl;
    };

    /////////////////////////////////////////////////////////////////////////////
    // Node
    // A single node in a parse tree, providing navigation and attributes.
    /////////////////////////////////////////////////////////////////////////////

    struct Node
    {
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        explicit Node(TSNode node) : impl{ node }
        {}

        ////////////////////////////////////////////////////////////////
        // Identification & Type
        ////////////////////////////////////////////////////////////////

        // Returns a unique identifier for a node in a parse tree.
        [[nodiscard]] NodeID getID() const
        {
            return reinterpret_cast<NodeID>(impl.id);
        }

        [[nodiscard]] details::StringViewReturn getType() const
        {
            return details::make_view(ts_node_type(impl));
        }

        [[nodiscard]] Symbol getSymbol() const
        {
            return ts_node_symbol(impl);
        }

        [[nodiscard]] details::StringViewReturn getGrammarType() const
        {
            return details::make_view(ts_node_grammar_type(impl));
        }

        [[nodiscard]] Symbol getGrammarSymbol() const
        {
            return ts_node_grammar_symbol(impl);
        }

        [[nodiscard]] Language getLanguage() const
        {
            return Language{ ts_node_language(impl) };
        }

        ////////////////////////////////////////////////////////////////
        // Flag Checks
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool isNull() const
        {
            return ts_node_is_null(impl);
        }

        [[nodiscard]] bool isNamed() const
        {
            return ts_node_is_named(impl);
        }

        [[nodiscard]] bool isMissing() const
        {
            return ts_node_is_missing(impl);
        }

        [[nodiscard]] bool isExtra() const
        {
            return ts_node_is_extra(impl);
        }

        [[nodiscard]] bool isError() const
        {
            return ts_node_is_error(impl);
        }

        [[nodiscard]] bool hasError() const
        {
            return ts_node_has_error(impl);
        }

        [[nodiscard]] bool hasChanges() const
        {
            return ts_node_has_changes(impl);
        }

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Extent<uint32_t> getByteRange() const
        {
            return { ts_node_start_byte(impl), ts_node_end_byte(impl) };
        }

        [[nodiscard]] Extent<Point> getPointRange() const
        {
            return { ts_node_start_point(impl), ts_node_end_point(impl) };
        }

        ////////////////////////////////////////////////////////////////
        // Source Text Access
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getSourceRange(details::StringViewParameter source) const
        {
            if (isNull())
            {
                return {};
            }

            Extent<uint32_t> extents = getByteRange();
            if (extents.end > source.size())
            {
                return "";
            }
            return source.substr(extents.start, extents.end - extents.start);
        }

        [[nodiscard]] std::string getSourceText(details::StringViewParameter source) const
        {
            return std::string(getSourceRange(source));
        }

        // Returns an S-Expression representation of the subtree rooted at this node.
        [[nodiscard]] std::unique_ptr<char, FreeHelper> getSExpr() const
        {
            return std::unique_ptr<char, FreeHelper>{ ts_node_string(impl) };
        }

        ////////////////////////////////////////////////////////////////
        // Navigation (General)
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getParent() const
        {
            return Node{ ts_node_parent(impl) };
        }

        [[nodiscard]] Node getNextSibling() const
        {
            return Node{ ts_node_next_sibling(impl) };
        }

        [[nodiscard]] Node getPreviousSibling() const
        {
            return Node{ ts_node_prev_sibling(impl) };
        }

        [[nodiscard]] uint32_t getChildCount() const
        {
            return ts_node_child_count(impl);
        }

        [[nodiscard]] Node getChild(uint32_t child_index) const
        {
            if (child_index >= getChildCount())
            {
                throw std::out_of_range("Tree-sitter: Child index out of bounds");
            }

            return Node{ ts_node_child(impl, child_index) };
        }

        [[nodiscard]] Node getFirstChild() const
        {
            return getChild(0);
        }

        [[nodiscard]] Node getLastChild() const
        {
            uint32_t count = getChildCount();

            return getChild(count > 0 ? count - 1 : count);
        }

        [[nodiscard]] Node getChildWithDescendant(Node descendant) const
        {
            return Node{ ts_node_child_with_descendant(impl, descendant.impl) };
        }

        ////////////////////////////////////////////////////////////////
        // Navigation (Named Children)
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getNextNamedSibling() const
        {
            return Node{ ts_node_next_named_sibling(impl) };
        }

        [[nodiscard]] Node getPreviousNamedSibling() const
        {
            return Node{ ts_node_prev_named_sibling(impl) };
        }

        [[nodiscard]] uint32_t getNamedChildCount() const
        {
            return ts_node_named_child_count(impl);
        }

        [[nodiscard]] Node getNamedChild(uint32_t child_index) const
        {
            if (child_index >= getNamedChildCount())
            {
                throw std::out_of_range("Tree-sitter: Named Child index out of bounds");
            }

            return Node{ ts_node_named_child(impl, child_index) };
        }

        [[nodiscard]] Node getFirstNamedChild() const
        {
            return getNamedChild(0);
        }

        [[nodiscard]] Node getLastNamedChild() const
        {
            uint32_t count = getNamedChildCount();

            return getNamedChild(count > 0 ? count - 1 : count);
        }

        ////////////////////////////////////////////////////////////////
        // Search & Descendants
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] uint32_t getDescendantsCount() const
        {
            return ts_node_descendant_count(impl);
        }

        [[nodiscard]] Node getFirstChildForByte(uint32_t byte) const
        {
            return Node{ ts_node_first_child_for_byte(impl, byte) };
        }

        [[nodiscard]] Node getFirstNamedChildForByte(uint32_t byte) const
        {
            return Node{ ts_node_first_named_child_for_byte(impl, byte) };
        }

        [[nodiscard]] Node getDescendantForByteRange(Extent<uint32_t> range) const
        {
            return Node{ ts_node_descendant_for_byte_range(impl, range.start, range.end) };
        }

        [[nodiscard]] Node getNamedDescendantForByteRange(Extent<uint32_t> range) const
        {
            return Node{ ts_node_named_descendant_for_byte_range(impl, range.start, range.end) };
        }

        [[nodiscard]] Node getDescendantForPointRange(Extent<Point> range) const
        {
            return Node{ ts_node_descendant_for_point_range(impl, range.start, range.end) };
        }

        [[nodiscard]] Node getNamedDescendantForPointRange(Extent<Point> range) const
        {
            return Node{ ts_node_named_descendant_for_point_range(impl, range.start, range.end) };
        }

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getFieldNameForChild(uint32_t child_index) const
        {
            return details::make_view(ts_node_field_name_for_child(impl, child_index));
        }

        [[nodiscard]] details::StringViewReturn getFieldNameForNamedChild(uint32_t named_child_index) const
        {
            return details::make_view(ts_node_field_name_for_named_child(impl, named_child_index));
        }

        [[nodiscard]] Node getChildByFieldName(details::StringViewParameter name) const
        {
            return Node{ ts_node_child_by_field_name(impl, name.data(), static_cast<uint32_t>(name.size())) };
        }

        [[nodiscard]] Node getChildByFieldID(FieldID field_id) const
        {
            return Node{ ts_node_child_by_field_id(impl, field_id) };
        }

        ////////////////////////////////////////////////////////////////
        // Parsing State
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] StateID getParseState() const
        {
            return ts_node_parse_state(impl);
        }

        [[nodiscard]] StateID getNextParseState() const
        {
            return ts_node_next_parse_state(impl);
        }

        ////////////////////////////////////////////////////////////////
        // Mutations
        ////////////////////////////////////////////////////////////////

        void edit(const InputEdit &edit)
        {
            edit.validate(edit);

            ts_node_edit(impl, &edit);
        }

        ////////////////////////////////////////////////////////////////
        // Cursor
        ////////////////////////////////////////////////////////////////

        // Definition deferred until after the definition of TreeCursor.
        [[nodiscard]] TreeCursor getCursor() const;

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool operator==(const Node &other) const
        {
            return ts_node_eq(impl, other.impl);
        }

        [[nodiscard]] bool operator!=(const Node &other) const
        {
            return !(*this == other);
        }

        TSNode impl;
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

        Tree(TSTree *tree) : impl{ tree, ts_tree_delete }
        {}

        Tree(const Tree &other) : impl{ ts_tree_copy(other.impl.get()), ts_tree_delete }
        {}

        Tree(Tree &&other) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool hasError() const
        {
            return getRootNode().hasError();
        }

        ////////////////////////////////////////////////////////////////
        // Root
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getRootNode() const
        {
            return Node{ ts_tree_root_node(impl.get()) };
        }

        [[nodiscard]] Node getRootNodeWithOffset(uint32_t offset_bytes, Point offset_extent) const
        {
            return Node{ ts_tree_root_node_with_offset(impl.get(), offset_bytes, offset_extent) };
        }

        void edit(const InputEdit &edit)
        {
            edit.validate(edit);

            ts_tree_edit(impl.get(), &edit);
        }

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<Range> getIncludedRanges() const
        {
            uint32_t count = 0;
            TSRange *array = ts_tree_included_ranges(impl.get(), &count);
            if (!array)
            {
                return {};
            }

            if (count == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Range> vec;
            vec.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
            {
                vec.emplace_back(array[i]);
            }

            std::free(array);
            return vec;
        }

        [[nodiscard]] static std::vector<Range> getChangedRanges(const Tree &old_tree, const Tree &new_tree)
        {
            uint32_t count = 0;
            TSRange *array = ts_tree_get_changed_ranges(static_cast<const TSTree *>(old_tree),
                                                        static_cast<const TSTree *>(new_tree),
                                                        &count);
            if (!array)
            {
                return {};
            }

            if (count == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Range> vec;
            vec.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
            {
                vec.emplace_back(array[i]);
            }

            std::free(array);
            return vec;
        }

        ////////////////////////////////////////////////////////////////
        // Language
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Language getLanguage() const
        {
            return Language{ ts_tree_language(impl.get()) };
        }

        ////////////////////////////////////////////////////////////////
        // Copy
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Tree copy() const
        {
            return Tree(*this);
        }

        ////////////////////////////////////////////////////////////////
        // Debugging
        ////////////////////////////////////////////////////////////////

        void printDotGraph(int file_descriptor)
        {
            ts_tree_print_dot_graph(impl.get(), file_descriptor);
        }

        void printDotGraph(FILE *file)
        {
            if (file)
            {
                printDotGraph(TS_FILENO(file));
            }
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        Tree &operator=(const Tree &other)
        {
            if (this != &other)
            {
                impl.reset(ts_tree_copy(other.impl.get()));
            }
            return *this;
        }

        Tree &operator=(Tree &&other) noexcept = default;

        [[nodiscard]] operator const TSTree *() const
        {
            return impl.get();
        }

    private:
        std::unique_ptr<TSTree, decltype(&ts_tree_delete)> impl;
    };

    /////////////////////////////////////////////////////////////////////////////
    // Parser
    // State machine used to produce a syntax tree from source code.
    /////////////////////////////////////////////////////////////////////////////

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

        Parser(Language language) : impl{ ts_parser_new(), ts_parser_delete }
        {
            if (!setLanguage(language))
            {
                throw std::runtime_error("Tree-sitter: Language version mismatch");
            }
        }

        ////////////////////////////////////////////////////////////////
        // Configuration
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool setLanguage(Language language)
        {
            return ts_parser_set_language(impl.get(), language);
        }

        [[nodiscard]] bool setIncludedRanges(const std::vector<Range> &ranges)
        {
            if (ranges.empty())
            {
                return ts_parser_set_included_ranges(impl.get(), nullptr, 0);
            }

            std::vector<TSRange> c_ranges;
            c_ranges.reserve(ranges.size());
            for (const auto &r : ranges)
            {
                c_ranges.push_back(static_cast<TSRange>(r));
            }

            std::sort(c_ranges.begin(),
                      c_ranges.end(),
                      [](const TSRange &a, const TSRange &b) { return a.start_byte < b.start_byte; });

            std::vector<TSRange> merged;
            merged.reserve(c_ranges.size());
            merged.push_back(c_ranges[0]);

            for (size_t i = 1; i < c_ranges.size(); ++i)
            {
                TSRange &last    = merged.back();
                TSRange &current = c_ranges[i];

                // If ranges overlap
                if (current.start_byte <= last.end_byte)
                {
                    // Update end_byte (futher from two)
                    if (current.end_byte > last.end_byte)
                    {
                        last.end_byte  = current.end_byte;
                        last.end_point = current.end_point;
                    }
                }
                else
                {
                    merged.push_back(current);
                }
            }

            return ts_parser_set_included_ranges(impl.get(), merged.data(), static_cast<uint32_t>(merged.size()));
        }

        void setWasmStore(WasmStore &store)
        {
            ts_parser_set_wasm_store(impl.get(), store);
        }

        [[nodiscard]] Language getCurrentLanguage() const
        {
            return Language{ ts_parser_language(impl.get()) };
        }

        [[nodiscard]] std::vector<Range> getIncludedRanges() const
        {
            uint32_t       count = 0;
            const TSRange *array = ts_parser_included_ranges(impl.get(), &count);
            if (!array)
            {
                return {};
            }

            if (count == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Range> vec;
            vec.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
            {
                vec.emplace_back(array[i]);
            }

            return vec;
        }

        [[nodiscard]] WasmStore takeWasmStore()
        {
            TSWasmStore *raw_store = ts_parser_take_wasm_store(impl.get());
            return WasmStore{ raw_store };
        }

        ////////////////////////////////////////////////////////////////
        // Parsing
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Tree parse(Input                                     &input,
                                 details::OptionalParam<Tree>               old_tree = {},
                                 details::OptionalParam<const ParseOptions> options  = {})
        {
            Input::current_input_ptr = &input;

            TSTree               *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);
            const TSParseOptions *raw_options  = nullptr;

            TSParseOptions c_options;
#if defined(TS_CXX_17) || defined(TS_CXX_20)
            if (options.has_value())
            {
                c_options = static_cast<TSParseOptions>(options->get());
#else
            if (options)
            {
                c_options = static_cast<TSParseOptions>(*options);
#endif
                raw_options = &c_options;
            }

            TSTree *new_tree = nullptr;
            if (raw_options)
            {
                new_tree = ts_parser_parse_with_options(impl.get(), raw_old_tree, input, *raw_options);
            }
            else
            {
                new_tree = ts_parser_parse(impl.get(), raw_old_tree, input);
            }

            Input::current_input_ptr = nullptr;
            return Tree(new_tree);
        }

        [[nodiscard]] Tree parseString(details::StringViewParameter buffer, details::OptionalParam<Tree> old_tree = {})
        {
            TSTree *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);

            return Tree{
                ts_parser_parse_string(impl.get(), raw_old_tree, buffer.data(), static_cast<uint32_t>(buffer.size()))
            };
        }

        [[nodiscard]] Tree parseStringEncoded(details::StringViewParameter buffer,
                                              InputEncoding                encoding,
                                              details::OptionalParam<Tree> old_tree = {})
        {
            TSTree *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);

            return Tree{ ts_parser_parse_string_encoding(impl.get(),
                                                         raw_old_tree,
                                                         buffer.data(),
                                                         static_cast<uint32_t>(buffer.size()),
                                                         static_cast<TSInputEncoding>(encoding)) };
        }

        ////////////////////////////////////////////////////////////////
        // Debugging
        ////////////////////////////////////////////////////////////////

        void enableDotGraphs(int file_descriptor)
        {
            ts_parser_print_dot_graphs(impl.get(), file_descriptor);
        }

        void enableDotGraphs(FILE *file)
        {
            if (file)
            {
                enableDotGraphs(TS_FILENO(file));
            }
        }

        void disableDotGraphs()
        {
            enableDotGraphs(-1);
        }

        ////////////////////////////////////////////////////////////////
        // Logging
        ////////////////////////////////////////////////////////////////

        void setLogger(LoggerFunction logger_func)
        {
            current_logger = std::move(logger_func);

            TSLogger ts_logger{};
            ts_logger.payload = this;
            ts_logger.log     = [](void *payload, TSLogType log_type, const char *buffer)
            {
                if (payload)
                {
                    auto *self = static_cast<Parser *>(payload);
                    if (self->current_logger)
                    {
                        self->current_logger(static_cast<LogType>(log_type), buffer);
                    }
                }
            };

            ts_parser_set_logger(impl.get(), ts_logger);
        }

        void removeLogger()
        {
            current_logger = nullptr;
            ts_parser_set_logger(impl.get(), { nullptr, nullptr });
        }

        ////////////////////////////////////////////////////////////////
        // Reset
        ////////////////////////////////////////////////////////////////

        // Does not remove logger
        void reset()
        {
            ts_parser_reset(impl.get());
        }

    private:
        std::unique_ptr<TSParser, decltype(&ts_parser_delete)> impl;
        std::function<void(LogType, const char *)>             current_logger;
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

        TreeCursor(TSNode node) : impl{ ts_tree_cursor_new(node) }
        {}

        TreeCursor(const TSTreeCursor &cursor) : impl{ ts_tree_cursor_copy(&cursor) }
        {}

        // By default avoid copies until the ergonomics are clearer.
        TreeCursor(const TreeCursor &other) = delete;

        TreeCursor(TreeCursor &&other) : impl{}
        {
            std::swap(impl, other.impl);
        }

        ~TreeCursor()
        {
            ts_tree_cursor_delete(&impl);
        }

        ////////////////////////////////////////////////////////////////
        // State Control
        ////////////////////////////////////////////////////////////////

        void reset(Node node)
        {
            ts_tree_cursor_reset(&impl, node.impl);
        }

        void reset(TreeCursor &cursor)
        {
            ts_tree_cursor_reset_to(&impl, &cursor.impl);
        }

        [[nodiscard]] TreeCursor copy() const
        {
            return TreeCursor(impl);
        }

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool gotoParent()
        {
            return ts_tree_cursor_goto_parent(&impl);
        }

        [[nodiscard]] bool gotoFirstChild()
        {
            return ts_tree_cursor_goto_first_child(&impl);
        }

        [[nodiscard]] bool gotoLastChild()
        {
            return ts_tree_cursor_goto_last_child(&impl);
        }

        [[nodiscard]] bool gotoNextSibling()
        {
            return ts_tree_cursor_goto_next_sibling(&impl);
        }

        [[nodiscard]] bool gotoPreviousSibling()
        {
            return ts_tree_cursor_goto_previous_sibling(&impl);
        }

        [[nodiscard]] int64_t gotoFirstChildForByte(uint32_t byte)
        {
            return ts_tree_cursor_goto_first_child_for_byte(&impl, byte);
        }

        [[nodiscard]] int64_t gotoFirstChildForPoint(Point point)
        {
            return ts_tree_cursor_goto_first_child_for_point(&impl, point);
        }

        void gotoDescendant(uint32_t descendant_index)
        {
            ts_tree_cursor_goto_descendant(&impl, descendant_index);
        }

        ////////////////////////////////////////////////////////////////
        // Current Node Attributes
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getCurrentNode() const
        {
            return Node{ ts_tree_cursor_current_node(&impl) };
        }

        [[nodiscard]] details::StringViewReturn getCurrentFieldName() const
        {
            return details::make_view(ts_tree_cursor_current_field_name(&impl));
        }

        [[nodiscard]] FieldID getCurrentFieldID() const
        {
            return ts_tree_cursor_current_field_id(&impl);
        }

        [[nodiscard]] uint32_t getCurrentDescendantIndex() const
        {
            return ts_tree_cursor_current_descendant_index(&impl);
        }

        [[nodiscard]] uint32_t getDepthFromOrigin() const
        {
            return ts_tree_cursor_current_depth(&impl);
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        TreeCursor &operator=(const TreeCursor &other) = delete;

        TreeCursor &operator=(TreeCursor &&other)
        {
            std::swap(impl, other.impl);
            return *this;
        }

    private:
        TSTreeCursor impl;
    };

    // To avoid cyclic dependencies and ODR violations, we define all methods
    // *using* TreeCursors inline after the definition of TreeCursor itself.
    [[nodiscard]] inline TreeCursor Node::getCursor() const
    {
        return TreeCursor{ impl };
    }

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

        Query(Language language, details::StringViewParameter source)
        {
            uint32_t     error_offset;
            TSQueryError error_type;

            TSQuery *query = ts_query_new(language,
                                          source.data(),
                                          static_cast<uint32_t>(source.size()),
                                          &error_offset,
                                          &error_type);

            details::QueryErrorHelper::validate(error_offset, error_type);

            if (!query)
            {
                throw std::runtime_error("Tree-sitter: Failed to create Query");
            }

            impl.reset(query);
        }

        ////////////////////////////////////////////////////////////////
        // Counts
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] uint32_t getPatternCount() const
        {
            return ts_query_pattern_count(impl.get());
        }

        [[nodiscard]] uint32_t getCaptureCount() const
        {
            return ts_query_capture_count(impl.get());
        }

        [[nodiscard]] uint32_t getStringCount() const
        {
            return ts_query_string_count(impl.get());
        }

        ////////////////////////////////////////////////////////////////
        // Pattern Information
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool isPatternRooted(uint32_t pattern_index) const
        {
            return ts_query_is_pattern_rooted(impl.get(), pattern_index);
        }

        [[nodiscard]] bool isPatternNonLocal(uint32_t pattern_index) const
        {
            return ts_query_is_pattern_non_local(impl.get(), pattern_index);
        }

        [[nodiscard]] bool isPatternGuaranteedAtStep(uint32_t byte_offset) const
        {
            return ts_query_is_pattern_guaranteed_at_step(impl.get(), byte_offset);
        }

        [[nodiscard]] Extent<uint32_t> getByteRangeForPattern(uint32_t pattern_index) const
        {
            return { ts_query_start_byte_for_pattern(impl.get(), pattern_index),
                     ts_query_end_byte_for_pattern(impl.get(), pattern_index) };
        }

        ////////////////////////////////////////////////////////////////
        // Predicates
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<QueryPredicateStep> getAllPredicatesForPattern(uint32_t pattern_index) const
        {
            uint32_t                    count = 0;
            const TSQueryPredicateStep *array = ts_query_predicates_for_pattern(impl.get(), pattern_index, &count);

            if (!array || count == 0)
            {
                return {};
            }

            std::vector<QueryPredicateStep> vec;
            vec.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
            {
                vec.emplace_back(array[i]);
            }

            return vec;
        }

        ////////////////////////////////////////////////////////////////
        // Capture & String Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getCaptureNameForID(uint32_t id) const
        {
            uint32_t    length;
            const char *name = ts_query_capture_name_for_id(impl.get(), id, &length);
            return details::make_view(name, length);
        }

        [[nodiscard]] Quantifier getCaptureQuantifierForID(uint32_t pattern_index, uint32_t capture_index) const
        {
            return static_cast<Quantifier>(
                    ts_query_capture_quantifier_for_id(impl.get(), pattern_index, capture_index));
        }

        [[nodiscard]] details::StringViewReturn getStringValueForID(uint32_t id) const
        {
            uint32_t    length;
            const char *name = ts_query_string_value_for_id(impl.get(), id, &length);
            return details::make_view(name, length);
        }

        ////////////////////////////////////////////////////////////////
        // Modification
        ////////////////////////////////////////////////////////////////

        void disableCapture(details::StringViewParameter name) const
        {
            ts_query_disable_capture(impl.get(), name.data(), static_cast<uint32_t>(name.size()));
        }

        void disablePattern(uint32_t pattern_index) const
        {
            ts_query_disable_pattern(impl.get(), pattern_index);
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] operator const TSQuery *() const
        {
            return impl.get();
        }

    private:
        std::unique_ptr<TSQuery, decltype(&ts_query_delete)> impl{ nullptr, ts_query_delete };
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

        QueryCursor() : impl{ ts_query_cursor_new(), ts_query_cursor_delete }
        {}

        // In C API QueryCursor don't have copy function
        QueryCursor(const QueryCursor &)            = delete;
        QueryCursor &operator=(const QueryCursor &) = delete;
        QueryCursor(QueryCursor &&)                 = default;
        QueryCursor &operator=(QueryCursor &&)      = default;

        ////////////////////////////////////////////////////////////////
        // Limits
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] uint32_t getMatchLimit()
        {
            return ts_query_cursor_match_limit(impl.get());
        }

        void setMatchLimit(uint32_t limit)
        {
            ts_query_cursor_set_match_limit(impl.get(), limit);
        }

        [[nodiscard]] bool didExceedMatchLimit()
        {
            return ts_query_cursor_did_exceed_match_limit(impl.get());
        }

        void setMaxStartDepth(uint32_t max_start_depth)
        {
            ts_query_cursor_set_max_start_depth(impl.get(), max_start_depth);
        }

        void resetMaxStartDepth()
        {
            ts_query_cursor_set_max_start_depth(impl.get(), UINT32_MAX);
        }

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool setByteRange(Extent<uint32_t> range)
        {
            return ts_query_cursor_set_byte_range(impl.get(), range.start, range.end);
        }

        [[nodiscard]] bool setPointRange(Extent<Point> range)
        {
            return ts_query_cursor_set_point_range(impl.get(), range.start, range.end);
        }

        [[nodiscard]] bool setContainingByteRange(Extent<uint32_t> range)
        {
            return ts_query_cursor_set_containing_byte_range(impl.get(), range.start, range.end);
        }

        [[nodiscard]] bool setContainingPointRange(Extent<Point> range)
        {
            return ts_query_cursor_set_containing_point_range(impl.get(), range.start, range.end);
        }

        ////////////////////////////////////////////////////////////////
        // Execution
        ////////////////////////////////////////////////////////////////

        void exec(const Query &query, Node node, details::OptionalParam<const QueryCursorOptions> query_options)
        {
            const TSQueryCursorOptions *raw_options = nullptr;

            TSQueryCursorOptions c_options;
#if defined(TS_CXX_17) || defined(TS_CXX_20)
            if (query_options.has_value())
            {
                c_options = static_cast<TSQueryCursorOptions>(query_options->get());
#else
            if (query_options)
            {
                c_options = static_cast<TSQueryCursorOptions>(*query_options);
#endif
                raw_options = &c_options;
            }

            if (raw_options)
            {
                ts_query_cursor_exec_with_options(impl.get(), query, node.impl, raw_options);
            }
            else
            {
                ts_query_cursor_exec(impl.get(), query, node.impl);
            }
        }

        ////////////////////////////////////////////////////////////////
        // Iteration
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool nextMatch(QueryMatch &match)
        {
            TSQueryMatch c_match{};
            if (ts_query_cursor_next_match(impl.get(), &c_match))
            {
                match = QueryMatch(c_match);
                return true;
            }
            return false;
        }

        [[nodiscard]] bool nextCapture(QueryMatch &match, uint32_t &capture_index)
        {
            TSQueryMatch c_match{};
            uint32_t     c_capture_index = 0;

            if (ts_query_cursor_next_capture(impl.get(), &c_match, &c_capture_index))
            {
                match         = QueryMatch(c_match);
                capture_index = c_capture_index;
                return true;
            }
            return false;
        }

        void removeMatch(uint32_t match_id)
        {
            return ts_query_cursor_remove_match(impl.get(), match_id);
        }

    private:
        std::unique_ptr<TSQueryCursor, decltype(&ts_query_cursor_delete)> impl;
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

        LookaheadIterator(Language language, StateID state)
            : impl{ ts_lookahead_iterator_new(language, state), ts_lookahead_iterator_delete }
        {
            if (!impl)
            {
                throw std::runtime_error("Tree-sitter: Invalid parse state for lookahead iterator");
            }
        }

        // Copying is not supported by the underlying C API.
        LookaheadIterator(const LookaheadIterator &)            = delete;
        LookaheadIterator &operator=(const LookaheadIterator &) = delete;

        LookaheadIterator(LookaheadIterator &&other) noexcept            = default;
        LookaheadIterator &operator=(LookaheadIterator &&other) noexcept = default;

        ////////////////////////////////////////////////////////////////
        // State Control
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool resetState(StateID state)
        {
            return ts_lookahead_iterator_reset_state(impl.get(), state);
        }

        [[nodiscard]] bool reset(Language language, StateID state)
        {
            return ts_lookahead_iterator_reset(impl.get(), language, state);
        }

        ////////////////////////////////////////////////////////////////
        // Property Accessors
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Language getLanguage() const
        {
            return Language{ ts_lookahead_iterator_language(impl.get()) };
        }

        [[nodiscard]] Symbol getCurrentSymbol() const
        {
            return ts_lookahead_iterator_current_symbol(impl.get());
        }

        [[nodiscard]] details::StringViewReturn getCurrentSymbolName() const
        {
            return details::make_view(ts_lookahead_iterator_current_symbol_name(impl.get()));
        }

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool next()
        {
            return ts_lookahead_iterator_next(impl.get());
        }

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] operator const TSLookaheadIterator *() const
        {
            return impl.get();
        }

    private:
        std::unique_ptr<TSLookaheadIterator, decltype(&ts_lookahead_iterator_delete)> impl;
    };

    [[nodiscard]] inline LookaheadIterator Language::getLookaheadIterator(StateID state) const
    {
        return LookaheadIterator(impl.get(), state);
    }

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

        WasmStore(WasmEngine *engine)
        {
            TSWasmError  error{ TSWasmErrorKindNone, nullptr };
            TSWasmStore *store = ts_wasm_store_new(engine, &error);

            details::WasmErrorHelper::validate(error);

            if (!store)
            {
                throw std::runtime_error("Tree-sitter: Failed to create Wasm store");
            }
            impl.reset(store);
        }

        explicit WasmStore(TSWasmStore *store) : impl{ store, ts_wasm_store_delete }
        {}

        WasmStore(const WasmStore &)                = delete;
        WasmStore &operator=(const WasmStore &)     = delete;
        WasmStore(WasmStore &&) noexcept            = default;
        WasmStore &operator=(WasmStore &&) noexcept = default;

        //////////////////////////////////////////////////////////////
        // Methods
        //////////////////////////////////////////////////////////////

        [[nodiscard]] Language loadLanguage(details::StringViewParameter name, details::StringViewParameter wasm_buffer)
        {
            TSWasmError       error{ TSWasmErrorKindNone, nullptr };
            const TSLanguage *lang = ts_wasm_store_load_language(impl.get(),
                                                                 name.data(),
                                                                 wasm_buffer.data(),
                                                                 static_cast<uint32_t>(wasm_buffer.size()),
                                                                 &error);

            details::WasmErrorHelper::validate(error);
            return Language{ lang };
        }

        [[nodiscard]] size_t getLanguageCount() const
        {
            return ts_wasm_store_language_count(impl.get());
        }

        //////////////////////////////////////////////////////////////
        // Converters
        //////////////////////////////////////////////////////////////

        [[nodiscard]] operator TSWasmStore *() const
        {
            return impl.get();
        }

    private:
        std::unique_ptr<TSWasmStore, decltype(&ts_wasm_store_delete)> impl{ nullptr, ts_wasm_store_delete };
    };

    /////////////////////////////////////////////////////////////////////////////
    // Child Node Iterators
    // STL-compatible iterators for processing Node children.
    /////////////////////////////////////////////////////////////////////////////

    class ChildIteratorSentinel
    {};

    class ChildIterator
    {
    public:
        ////////////////////////////////////////////////////////////////
        // Aliases
        ////////////////////////////////////////////////////////////////

        using value_type        = ts::Node;
        using difference_type   = int;
        using iterator_category = std::input_iterator_tag;

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        explicit ChildIterator(const ts::Node &node) : cursor{ node.getCursor() }, atEnd{ !cursor.gotoFirstChild() }
        {}

        ////////////////////////////////////////////////////////////////
        // Get
        ////////////////////////////////////////////////////////////////

        value_type operator*() const
        {
            return cursor.getCurrentNode();
        }

        ////////////////////////////////////////////////////////////////
        // Advance
        ////////////////////////////////////////////////////////////////

        ChildIterator &operator++()
        {
            atEnd = !cursor.gotoNextSibling();
            return *this;
        }

        ChildIterator &operator++(int)
        {
            atEnd = !cursor.gotoNextSibling();
            return *this;
        }

        ////////////////////////////////////////////////////////////////
        // Comparision
        ////////////////////////////////////////////////////////////////

        friend bool operator==(const ChildIterator &a, const ChildIteratorSentinel &)
        {
            return a.atEnd;
        }

        friend bool operator!=(const ChildIterator &a, const ChildIteratorSentinel &b)
        {
            return !(a == b);
        }

        friend bool operator==(const ChildIteratorSentinel &b, const ChildIterator &a)
        {
            return a == b;
        }

        friend bool operator!=(const ChildIteratorSentinel &b, const ChildIterator &a)
        {
            return a != b;
        }

    private:
        ts::TreeCursor cursor;
        bool           atEnd;
    };

    struct Children
    {
        using iterator = ChildIterator;
        using sentinel = ChildIteratorSentinel;

        auto begin() const -> iterator
        {
            return ChildIterator{ node };
        }

        auto end() const -> sentinel
        {
            return {};
        }

        const ts::Node &node;
    };

#if defined(TS_CXX_20)
    static_assert(std::input_iterator<ChildIterator>);
    static_assert(std::sentinel_for<ChildIteratorSentinel, ChildIterator>);
#endif

    /////////////////////////////////////////////////////////////////////////////
    // Visitor
    // High-level utility for depth-first traversal of a syntax tree.
    /////////////////////////////////////////////////////////////////////////////

#if defined(TS_CXX_17)
    template <typename T, typename = void>
    struct VisitorConcept : std::false_type
    {};

    template <typename T>
    struct VisitorConcept<
            T,
            std::void_t<std::enable_if_t<std::is_same_v<void, decltype(std::declval<T>()(std::declval<Node>()))>>>>
        : std::true_type
    {};

    template <typename F, std::enable_if_t<VisitorConcept<F>::value, bool> = true>
#elif defined(TS_CXX_20)
    template <typename T>
    concept VisitorConcept = requires {
        { std::declval<T>()(std::declval<Node>()) } -> std::same_as<void>;
    };

    template <VisitorConcept F>
#else
template <typename F>
#endif
    void visit(Node root, F &&callback)
    {
        if (root.isNull())
        {
            return;
        }

        TreeCursor   cursor      = root.getCursor();
        const size_t start_depth = cursor.getDepthFromOrigin();

        while (true)
        {
            callback(cursor.getCurrentNode());

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
#endif // CPP_TREE_SITTER_HPP
