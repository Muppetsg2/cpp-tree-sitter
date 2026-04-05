#pragma once
#ifndef CPP_TREE_SITTER_HPP
#define CPP_TREE_SITTER_HPP

#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

#include <tree_sitter/api.h>

// Including the API directly already pollutes the namespace, but the
// functions are prefixed. Anything else that we include should be scoped
// within a namespace.

namespace ts
{

    /////////////////////////////////////////////////////////////////////////////
    // Helper classes.
    // These can be ignored while tring to understand the core APIs on demand.
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

    /////////////////////////////////////////////////////////////////////////////
    // Aliases.
    // Create slightly stricter aliases for some of the core tree-sitter types.
    /////////////////////////////////////////////////////////////////////////////


    // Direct alias of { row: uint32_t; column: uint32_t }
    using Point = TSPoint;

    using Symbol = uint16_t;

    using FieldID = uint16_t;

    using Version = uint32_t;

    using NodeID = uintptr_t;

    // Enums wrappers
    enum class SymbolType : uint8_t
    {
        TypeRegular   = TSSymbolTypeRegular,
        TypeAnonymous = TSSymbolTypeAnonymous,
        TypeSupertype = TSSymbolTypeSupertype,
        TypeAuxiliary = TSSymbolTypeAuxiliary,
    };

    enum class InputEncoding : uint8_t
    {
        UTF8    = TSInputEncodingUTF8,
        UTF16LE = TSInputEncodingUTF16LE,
        UTF16BE = TSInputEncodingUTF16BE
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

    // For types that manage resources, create custom wrappers that ensure
    // clean-up. For types that can benefit from additional API discovery,
    // wrappers with implicit conversion allow for automated method discovery.

    class Language
    {
    public:
        // NOTE: Allowing implicit conversions from TSLanguage to Language
        // improves the ergonomics a bit, as clients will still make use of the
        // custom language functions.

        /* implicit */ Language(const TSLanguage *language)
            : impl{ language, [](const TSLanguage *l) { ts_language_delete(l); } }
        {}

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

        [[nodiscard]] std::string_view getSymbolName(Symbol symbol) const
        {
            return ts_language_symbol_name(impl.get(), symbol);
        }

        [[nodiscard]] SymbolType getSymbolType(Symbol symbol) const
        {
            return static_cast<SymbolType>(ts_language_symbol_type(impl.get(), symbol));
        }

        [[nodiscard]] Symbol getSymbolForName(std::string_view name, bool isNamed) const
        {
            return ts_language_symbol_for_name(impl.get(), &name.front(), static_cast<uint32_t>(name.size()), isNamed);
        }

        [[nodiscard]] std::string_view getFieldNameForId(FieldID id) const
        {
            return ts_language_field_name_for_id(impl.get(), id);
        }

        [[nodiscard]] FieldID getFieldIdForName(std::string_view name) const
        {
            return ts_language_field_id_for_name(impl.get(), &name.front(), static_cast<uint32_t>(name.size()));
        }

        [[nodiscard]] std::vector<Symbol> getAllSuperTypes() const
        {
            uint32_t length = 0;
            Symbol  *array  = ts_language_supertypes(impl.get(), &length);

            return std::vector<Symbol> vec(array, array + length);
        }

        [[nodiscard]] std::vector<Symbol> getAllSubTypesForSuperType(Symbol superType) const
        {
            uint32_t length = 0;
            Symbol  *array  = ts_language_subtypes(impl.get(), superType, &length);

            return std::vector<Symbol> vec(array, array + length);
        }

        [[nodiscard]] std::string_view getName() const
        {
            return ts_language_name(impl.get());
        }

        [[nodiscard]] Version getVersion() const
        {
            return ts_language_abi_version(impl.get());
        }

        [[nodiscard]] operator const TSLanguage *() const
        {
            return impl.get();
        }

    private:
        std::shared_ptr<const TSLanguage> impl;
    };


    class TreeCursor;

    struct Node
    {
        explicit Node(TSNode node) : impl{ node }
        {}

        ////////////////////////////////////////////////////////////////
        // Flag checks on nodes
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
        // Navigation
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

        // Named children

        [[nodiscard]] uint32_t getNumNamedChildren() const
        {
            return ts_node_named_child_count(impl);
        }

        [[nodiscard]] Node getNamedChild(uint32_t position) const
        {
            return Node{ ts_node_named_child(impl, position) };
        }

        // Named fields

        [[nodiscard]] std::string_view getFieldNameForChild(uint32_t child_position) const
        {
            return ts_node_field_name_for_child(impl, child_position);
        }

        [[nodiscard]] Node getChildByFieldName(std::string_view name) const
        {
            return Node{ ts_node_child_by_field_name(impl, &name.front(), static_cast<uint32_t>(name.size())) };
        }

        // Definition deferred until after the definition of TreeCursor.
        [[nodiscard]] TreeCursor getCursor() const;

        ////////////////////////////////////////////////////////////////
        // Node attributes
        ////////////////////////////////////////////////////////////////

        // Returns a unique identifier for a node in a parse tree.
        // NodeIDs are numeric value types.
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

        [[nodiscard]] std::string_view getType() const
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

        [[nodiscard]] std::string_view getSourceRange(std::string_view source) const
        {
            Extent<uint32_t> extents = this->getByteRange();
            return source.substr(extents.start, extents.end - extents.start);
        }

        TSNode impl;
    };

    class Tree
    {
    public:
        Tree(TSTree *tree) : impl{ tree, ts_tree_delete }
        {}

        [[nodiscard]] Node getRootNode() const
        {
            return Node{ ts_tree_root_node(impl.get()) };
        }

        [[nodiscard]] Language getLanguage() const
        {
            return Language{ ts_tree_language(impl.get()) };
        }

        [[nodiscard]] bool hasError() const
        {
            return getRootNode().hasError();
        }

    private:
        std::unique_ptr<TSTree, decltype(&ts_tree_delete)> impl;
    };

    class Parser
    {
    public:
        Parser(Language language) : impl{ ts_parser_new(), ts_parser_delete }
        {
            if (!setLanguage(language))
            {
                throw std::runtime_error("Tree-sitter: Language version mismatch");
            }
        }

        [[nodiscard]] Language getCurrentLanguage() const
        {
            return Language{ ts_parser_language(impl.get()) };
        }

        [[nodiscard]] Tree parseString(std::string_view buffer)
        {
            return Tree{
                ts_parser_parse_string(impl.get(), nullptr, &buffer.front(), static_cast<uint32_t>(buffer.size()))
            };
        }

        [[nodiscard]] Tree parseStringEncoded(std::string_view buffer, InputEncoding encoding)
        {
            return Tree{ ts_parser_parse_string_encoding(impl.get(),
                                                         nullptr,
                                                         &buffer.front(),
                                                         static_cast<uint32_t>(buffer.size()),
                                                         static_cast<TSInputEncoding>(encoding)) };
        }

        [[nodiscard]] bool setLanguage(Language language)
        {
            return ts_parser_set_language(impl.get(), language);
        }

        void setLogger(std::function<void(LogType, const char *)> logger_func)
        {
            current_logger = std::move(logger_func);

            TSLogger ts_logger{};
            ts_logger.payload = this;
            ts_logger.log     = [](void *payload, TSLogType log_type, const char *buffer)
            {
                auto *self = static_cast<Parser *>(payload);
                if (self->current_logger)
                {
                    self->current_logger(static_cast<LogType>(log_type), buffer);
                }
            };

            ts_parser_set_logger(impl.get(), ts_logger);
        }

        void removeLogger()
        {
            current_logger = nullptr;
            ts_parser_set_logger(impl.get(), { nullptr, nullptr });
        }

        void reset()
        {
            ts_parser_reset(impl.get());
        }

    private:
        std::unique_ptr<TSParser, decltype(&ts_parser_delete)> impl;

        std::function<void(LogType, const char *)> current_logger;
    };

    class TreeCursor
    {
    public:
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

        TreeCursor &operator=(const TreeCursor &other) = delete;

        TreeCursor &operator=(TreeCursor &&other)
        {
            std::swap(impl, other.impl);
            return *this;
        }

        ~TreeCursor()
        {
            ts_tree_cursor_delete(&impl);
        }

        void reset(Node node)
        {
            ts_tree_cursor_reset(&impl, node.impl);
        }

        void reset(Cursor &cursor)
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

        // Navigation

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

    private:
        TSTreeCursor impl;
    };

    // To avoid cyclic dependencies and ODR violations, we define all methods
    // *using* TreeCursors inline after the definition of TreeCursor itself.
    [[nodiscard]] inline TreeCursor Node::getCursor() const
    {
        return TreeCursor{ impl };
    }

    ////////////////////////////////////////////////////////////////
    // Query
    ////////////////////////////////////////////////////////////////

    class Query
    {
    public:
        Query(Language language, std::string_view source)
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

        [[nodiscard]] std::string_view getCaptureNameForId(uint32_t id) const
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

        [[nodiscard]] std::string_view getStringValueForId(uint32_t id) const
        {
            uint32_t    length;
            const char *name = ts_query_string_value_for_id(impl.get(), id, &length);
            return { name, length };
        }

        void disableCapture(std::string_view name) const
        {
            ts_query_disable_capture(impl.get(), &name.front(),
                                                         static_cast<uint32_t>(name.size());
        }

        void disablePattern(uint32_t pattern_index) const
        {
            ts_query_disable_pattern(impl.get(), pattern_index);
        }

        [[nodiscard]] operator const TSQuery *() const
        {
            return impl.get();
        }

    private:
        std::unique_ptr<TSQuery, decltype(&ts_query_delete)> impl{ nullptr, ts_query_delete };
    };

    struct QueryCapture
    {
        Node     node;
        uint32_t index;

        // for easy conversion from C API
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

    class QueryCursor
    {
    public:
        QueryCursor() : impl{ ts_query_cursor_new(), ts_query_cursor_delete }
        {}

        // In C API QueryCursor don't have copy function
        QueryCursor(const QueryCursor &)            = delete;
        QueryCursor &operator=(const QueryCursor &) = delete;
        QueryCursor(QueryCursor &&)                 = default;
        QueryCursor &operator=(QueryCursor &&)      = default;

        void exec(const Query &query, Node node)
        {
            ts_query_cursor_exec(impl.get(), query, node.impl);
        }

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

    ////////////////////////////////////////////////////////////////
    // Child node iterators
    ////////////////////////////////////////////////////////////////

    // These iterators make it possible to use C++ views on Nodes for
    // easy processing.

    class ChildIteratorSentinel
    {};

    class ChildIterator
    {
    public:
        using value_type        = ts::Node;
        using difference_type   = int;
        using iterator_category = std::input_iterator_tag;

        explicit ChildIterator(const ts::Node &node) : cursor{ node.getCursor() }, atEnd{ !cursor.gotoFirstChild() }
        {}

        value_type operator*() const
        {
            return cursor.getCurrentNode();
        }

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

    static_assert(std::input_iterator<ChildIterator>);
    static_assert(std::sentinel_for<ChildIteratorSentinel, ChildIterator>);

    ////////////////////////////////////////////////////////////////
    // Visitor
    ////////////////////////////////////////////////////////////////

    // These visitor concept make it easy to check all nodes in tree
#ifdef TS_CXX_17
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
#elif TS_CXX_20
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
#endif
