#pragma once
#ifndef CPP_TREE_SITTER_HPP
#define CPP_TREE_SITTER_HPP

#include <algorithm>
#include <cstdint>
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
        constexpr StringView() noexcept : data_(nullptr), size_(0)
        {}

        constexpr StringView(const char *s, size_t count) : data_(s), size_(count)
        {}

        StringView(const char *s) : data_(s), size_(s ? std::strlen(s) : 0)
        {}

        StringView(const std::string &s) noexcept : data_(s.data()), size_(s.size())
        {}

        [[nodiscard]] constexpr const char *data() const noexcept
        {
            return data_;
        }

        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return size_;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return size_ == 0;
        }

        [[nodiscard]] constexpr char operator[](size_t pos) const
        {
            return data_[pos];
        }

        [[nodiscard]] constexpr const char *begin() const noexcept
        {
            return data_;
        }
        [[nodiscard]] constexpr const char *end() const noexcept
        {
            return data_ + size_;
        }

        void remove_prefix(size_t n)
        {
            data_ += n;
            size_ -= n;
        }

        void remove_suffix(size_t n)
        {
            size_ -= n;
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

    namespace details
    {
#if defined(TS_CXX_17) || defined(TS_CXX_20)
        using StringViewParameter = const std::string_view;
        using StringViewReturn    = std::string_view;

        using ByteViewU8_t = const std::basic_string_view<uint8_t>;

        template <typename T>
        using OptionalParam = std::optional<std::reference_wrapper<T>>;

        template <typename T, typename Raw>
        [[nodiscard]] static Raw *get_raw(OptionalParam<T> opt)
        {
            return opt.has_value() ? static_cast<Raw *>(opt->get()) : nullptr;
        }
#else
        using StringViewParameter = const std::string &;
        using StringViewReturn    = StringView;

        template <typename T>
        using OptionalParam = T *;

        template <typename T, typename Raw>
        [[nodiscard]] static Raw *get_raw(OptionalParam<T> opt)
        {
            return opt ? static_cast<Raw *>(*opt) : nullptr;
        }
#endif
    } // namespace details

    // Direct alias of { row: uint32_t; column: uint32_t }
    using Point = TSPoint;

    // Direct alias of { start_byte: uint32_t; old_end_byte: uint32_t, new_end_byte: uint32_t, start_point: Point,
    // old_end_point: Point, new_end_point: Point }
    using InputEdit = TSInputEdit;

    // Direct alias of { major_version: uint8_t; minor_version: uint8_t; patch_version: uint8_t }
    using LanguageMetadata = TSLanguageMetadata;

    using StateID = uint16_t;
    using Symbol  = uint16_t;
    using FieldID = uint16_t;
    using Version = uint32_t;
    using NodeID  = uintptr_t;

#if defined(TS_CXX_17) || defined(TS_CXX_20)
    using DecodeFunction = std::function<uint32_t(ByteViewU8_t, int32_t *)>;
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

    /////////////////////////////////////////////////////////////////////////////
    // Range
    // Representation of a range within the source code using points and bytes.
    /////////////////////////////////////////////////////////////////////////////

    struct Range
    {
        Extent<Point>    point;
        Extent<uint32_t> byte;

        explicit Range(const TSRange &range)
            : point(range.start_point, range.end_point), byte(range.start_byte, range.end_byte)
        {}

        operator TSRange() const
        {
            return { point.start, point.end, byte.start, byte.end };
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // Input
    // ...
    /////////////////////////////////////////////////////////////////////////////

    struct Input
    {
        using ReadFunction = std::function<details::StringViewReturn(uint32_t, Point, uint32_t *)>;

        ReadFunction       read;
        InputEncoding      encoding;
        ts::DecodeFunction decode;

        static const char *read_proxy(void *payload, uint32_t byte_index, TSPoint position, uint32_t *bytes_read)
        {
            auto *self = static_cast<Input *>(payload);
            if (self && self->read)
            {
                details::StringViewReturn result = self->read(byte_index, Point(position), bytes_read);
                return result.data();
            }
            *bytes_read = 0;
            return nullptr;
        }

        static thread_local const Input *current_input_ptr;

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
    // ...
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
    // ...
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
        // Num Accessors
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] size_t getNumSymbols() const
        {
            return ts_language_symbol_count(impl.get());
        }

        [[nodiscard]] size_t getNumStates() const
        {
            return ts_language_state_count(impl.get());
        }

        [[nodiscard]] size_t getNumFields() const
        {
            return ts_language_field_count(impl.get());
        }

        ////////////////////////////////////////////////////////////////
        // Symbol Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getSymbolName(Symbol symbol) const
        {
            return ts_language_symbol_name(impl.get(), symbol);
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

        [[nodiscard]] details::StringViewReturn getFieldNameForId(FieldID id) const
        {
            return ts_language_field_name_for_id(impl.get(), id);
        }

        [[nodiscard]] FieldID getFieldIdForName(details::StringViewParameter name) const
        {
            return ts_language_field_id_for_name(impl.get(), name.data(), static_cast<uint32_t>(name.size()));
        }

        ////////////////////////////////////////////////////////////////
        // Type Hierarchy
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<Symbol> getAllSuperTypes() const
        {
            uint32_t length = 0;
            Symbol  *array  = ts_language_supertypes(impl.get(), &length);
            if (!array)
            {
                return {};
            }

            if (length == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Symbol> vec(array, array + length);
            std::free(array);

            return vec;
        }

        [[nodiscard]] std::vector<Symbol> getAllSubTypesForSuperType(Symbol supertype) const
        {
            uint32_t length = 0;
            Symbol  *array  = ts_language_subtypes(impl.get(), supertype, &length);
            if (!array)
            {
                return {};
            }

            if (length == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Symbol> vec(array, array + length);
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

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getName() const
        {
            return ts_language_name(impl.get());
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

    class TreeCursor;

    struct Node
    {
        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        explicit Node(TSNode node) : impl{ node }
        {}

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

        [[nodiscard]] bool hasError() const
        {
            return ts_node_has_error(impl);
        }

        [[nodiscard]] bool isError() const
        {
            return ts_node_is_error(impl);
        }

        ////////////////////////////////////////////////////////////////
        // Navigation (Direct)
        ////////////////////////////////////////////////////////////////

        // Direct parent/sibling/child connections and cursors

        [[nodiscard]] Node getParent() const
        {
            return Node{ ts_node_parent(impl) };
        }

        [[nodiscard]] Node getPreviousSibling() const
        {
            return Node{ ts_node_prev_sibling(impl) };
        }

        [[nodiscard]] Node getNextSibling() const
        {
            return Node{ ts_node_next_sibling(impl) };
        }

        [[nodiscard]] uint32_t getNumChildren() const
        {
            return ts_node_child_count(impl);
        }

        [[nodiscard]] Node getChild(uint32_t position) const
        {
            return Node{ ts_node_child(impl, position) };
        }

        [[nodiscard]] Node getChildWithDescendant(Node descendant) const
        {
            return Node{ ts_node_child_with_descendant(impl, descendant.impl) };
        }

        ////////////////////////////////////////////////////////////////
        // Navigation (Range-based)
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getFirstChildForByte(uint32_t byte) const
        {
            return Node{ ts_node_first_child_for_byte(impl, byte) };
        }

        [[nodiscard]] uint32_t getNumDescendants() const
        {
            return ts_node_descendant_count(impl);
        }

        [[nodiscard]] Node getDescendantForByteRange(Extent<uint32_t> range) const
        {
            return Node{ ts_node_descendant_for_byte_range(impl, range.start, range.end) };
        }

        [[nodiscard]] Node getDescendantForPointRange(Extent<Point> range) const
        {
            return Node{ ts_node_descendant_for_point_range(impl, range.start, range.end) };
        }

        ////////////////////////////////////////////////////////////////
        // Named Children
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] Node getPreviousNamedSibling() const
        {
            return Node{ ts_node_prev_named_sibling(impl) };
        }

        [[nodiscard]] Node getNextNamedSibling() const
        {
            return Node{ ts_node_next_named_sibling(impl) };
        }

        [[nodiscard]] uint32_t getNumNamedChildren() const
        {
            return ts_node_named_child_count(impl);
        }

        [[nodiscard]] Node getNamedChild(uint32_t position) const
        {
            return Node{ ts_node_named_child(impl, position) };
        }

        [[nodiscard]] Node getFirstNamedChildForByte(uint32_t byte) const
        {
            return Node{ ts_node_first_named_child_for_byte(impl, byte) };
        }

        [[nodiscard]] Node getNamedDescendantForByteRange(Extent<uint32_t> range) const
        {
            return Node{ ts_node_named_descendant_for_byte_range(impl, range.start, range.end) };
        }

        [[nodiscard]] Node getNamedDescendantForPointRange(Extent<Point> range) const
        {
            return Node{ ts_node_named_descendant_for_point_range(impl, range.start, range.end) };
        }

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getFieldNameForChild(uint32_t child_position) const
        {
            return ts_node_field_name_for_child(impl, child_position);
        }

        [[nodiscard]] details::StringViewReturn getFieldNameForNamedChild(uint32_t named_child_index) const
        {
            return ts_node_field_name_for_named_child(impl, child_position);
        }

        [[nodiscard]] Node getChildByFieldName(details::StringViewParameter name) const
        {
            return Node{ ts_node_child_by_field_name(impl, name.data(), static_cast<uint32_t>(name.size())) };
        }

        ////////////////////////////////////////////////////////////////
        // Cursor Creation
        ////////////////////////////////////////////////////////////////

        // Definition deferred until after the definition of TreeCursor.
        [[nodiscard]] TreeCursor getCursor() const;

        ////////////////////////////////////////////////////////////////
        // Attributes
        ////////////////////////////////////////////////////////////////

        // Returns a unique identifier for a node in a parse tree.
        [[nodiscard]] NodeID getID() const
        {
            return reinterpret_cast<NodeID>(impl.id);
        }

        // Returns an S-Expression representation of the subtree rooted at this node.
        [[nodiscard]] std::unique_ptr<char, FreeHelper> getSExpr() const
        {
            return std::unique_ptr<char, FreeHelper>{ ts_node_string(impl) };
        }

        [[nodiscard]] Symbol getSymbol() const
        {
            return ts_node_symbol(impl);
        }

        [[nodiscard]] details::StringViewReturn getType() const
        {
            return ts_node_type(impl);
        }

        [[nodiscard]] Language getLanguage() const
        {
            return Language{ ts_node_language(impl) };
        }

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
            if (this->isNull())
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
            if (edit.start_byte > edit.old_end_byte || edit.start_point > edit.old_end_point)
            {
                throw std::invalid_argument("Tree-sitter: Invalid edit ranges (start > old_end)");
            }

            ts_tree_edit(impl.get(), &edit);
        }

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] std::vector<Range> getIncludedRanges() const
        {
            uint32_t length = 0;
            TSRange *array  = ts_tree_included_ranges(impl.get(), &length);
            if (!array)
            {
                return {};
            }

            if (length == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Range> vec;
            vec.reserve(length);
            for (uint32_t i = 0; i < length; ++i)
            {
                vec.emplace_back(array[i]);
            }

            std::free(array);
            return vec;
        }

        [[nodiscard]] static std::vector<Range> getChangedRanges(const Tree &old_tree, const Tree &new_tree)
        {
            uint32_t length = 0;
            TSRange *array  = ts_tree_get_changed_ranges(static_cast<const TSTree *>(old_tree),
                                                        static_cast<const TSTree *>(new_tree),
                                                        &length);
            if (!array)
            {
                return {};
            }

            if (length == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Range> vec;
            vec.reserve(length);
            for (uint32_t i = 0; i < length; ++i)
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

        [[nodiscard]] Language getCurrentLanguage() const
        {
            return Language{ ts_parser_language(impl.get()) };
        }

        [[nodiscard]] std::vector<Range> getIncludedRanges() const
        {
            uint32_t       length = 0;
            const TSRange *array  = ts_parser_included_ranges(impl.get(), &length);
            if (!array)
            {
                return {};
            }

            if (length == 0)
            {
                std::free(array);
                return {};
            }

            std::vector<Range> vec;
            vec.reserve(length);
            for (uint32_t i = 0; i < length; ++i)
            {
                vec.emplace_back(array[i]);
            }

            return vec;
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

        [[nodiscard]] Node getCurrentNode() const
        {
            return Node{ ts_tree_cursor_current_node(&impl) };
        }

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool gotoParent()
        {
            return ts_tree_cursor_goto_parent(&impl);
        }

        [[nodiscard]] bool gotoNextSibling()
        {
            return ts_tree_cursor_goto_next_sibling(&impl);
        }

        [[nodiscard]] bool gotoPreviousSibling()
        {
            return ts_tree_cursor_goto_previous_sibling(&impl);
        }

        [[nodiscard]] bool gotoFirstChild()
        {
            return ts_tree_cursor_goto_first_child(&impl);
        }

        [[nodiscard]] bool gotoLastChild()
        {
            return ts_tree_cursor_goto_last_child(&impl);
        }

        [[nodiscard]] size_t getDepthFromOrigin() const
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

            if (!query)
            {
                std::string error_type_str = "None";

                switch (error_type)
                {
                    case TSQueryErrorSyntax:
                    {
                        error_type_str = "Syntax";
                        break;
                    }
                    case TSQueryErrorNodeType:
                    {
                        error_type_str = "Node Type";
                        break;
                    }
                    case TSQueryErrorField:
                    {
                        error_type_str = "Field";
                        break;
                    }
                    case TSQueryErrorCapture:
                    {
                        error_type_str = "Capture";
                        break;
                    }
                    case TSQueryErrorStructure:
                    {
                        error_type_str = "Structure";
                        break;
                    }
                    case TSQueryErrorLanguage:
                    {
                        error_type_str = "Language";
                        break;
                    }
                    default:
                        break;
                }

                throw std::runtime_error("Tree-sitter Query Error at offset " + std::to_string(error_offset)
                                         + " (Type: " + error_type_str + ")");
            }
            impl.reset(query);
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

        [[nodiscard]] uint32_t getCaptureCount() const
        {
            return ts_query_capture_count(impl.get());
        }

        [[nodiscard]] uint32_t getPatternCount() const
        {
            return ts_query_pattern_count(impl.get());
        }

        [[nodiscard]] uint32_t getStringCount() const
        {
            return ts_query_string_count(impl.get());
        }

        [[nodiscard]] Extent<uint32_t> getByteRangeForPattern(uint32_t pattern_index) const
        {
            return { ts_query_start_byte_for_pattern(impl.get(), pattern_index),
                     ts_query_end_byte_for_pattern(impl.get(), pattern_index) };
        }

        ////////////////////////////////////////////////////////////////
        // Capture Resolution
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] details::StringViewReturn getCaptureNameForId(uint32_t id) const
        {
            uint32_t    length;
            const char *name = ts_query_capture_name_for_id(impl.get(), id, &length);
            return { name, length };
        }

        [[nodiscard]] Quantifier getCaptureQuantifierForId(uint32_t pattern_index, uint32_t capture_index) const
        {
            return static_cast<Quantifier>(
                    ts_query_capture_quantifier_for_id(impl.get(), pattern_index, capture_index));
        }

        [[nodiscard]] details::StringViewReturn getStringValueForId(uint32_t id) const
        {
            uint32_t    length;
            const char *name = ts_query_string_value_for_id(impl.get(), id, &length);
            return { name, length };
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
    // Query Results
    // Structures representing matches and captures from a Query.
    /////////////////////////////////////////////////////////////////////////////

    struct QueryCapture
    {
        Node     node;
        uint32_t index;

        // For easy conversion from C API
        explicit QueryCapture(const TSQueryCapture &capture) : node(capture.node), index(capture.index)
        {}
    };

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
        // Execution
        ////////////////////////////////////////////////////////////////

        void exec(const Query &query, Node node)
        {
            ts_query_cursor_exec(impl.get(), query, node.impl);
        }

        ////////////////////////////////////////////////////////////////
        // Limits
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] bool didExceedMatchLimit()
        {
            return ts_query_cursor_did_exceed_match_limit(impl.get());
        }

        [[nodiscard]] uint32_t getMatchLimit()
        {
            return ts_query_cursor_match_limit(impl.get());
        }

        void setMatchLimit(uint32_t limit)
        {
            ts_query_cursor_set_match_limit(impl.get(), limit);
        }

        void setMaxStartDepth(uint32_t max_start_depth)
        {
            ts_query_cursor_set_max_start_depth(impl.get(), max_start_depth);
        }

        ////////////////////////////////////////////////////////////////
        // Range Configuration
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
