#pragma once
#ifndef CPP_TREE_SITTER_HPP
#define CPP_TREE_SITTER_HPP

// STL
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

#pragma region STANDARD_COMPATIBILITY_MACROS

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

#pragma endregion

// Tree-sitter
#include <tree_sitter/api.h>

namespace ts
{
#pragma region TYPE_ALIASES

    /////////////////////////////////////////////////////////////////////////////
    // Type Aliases
    // Create slightly stricter aliases for some of the core tree-sitter types.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Direct alias of { major_version: uint8_t; minor_version: uint8_t; patch_version: uint8_t }.
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

    /// @brief Callback used to decode source bytes.
    using DecodeFunction = std::function<std::uint32_t(ByteView, std::int32_t *)>;

#pragma endregion

#pragma region HELPERS

    /////////////////////////////////////////////////////////////////////////////
    // Helper Classes & Structs
    // Internal utilities for memory management and range representation.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief An inclusive range representation.
    template <typename T>
    struct Extent
    {
        /// @brief Inclusive start of the range.
        T start;

        /// @brief Inclusive end of the range.
        T end;

        /// @brief Default Constructor.
        constexpr Extent() noexcept(std::is_nothrow_default_constructible<T>::value) : start(), end()
        {}

        /**
         * @brief Default Constructor.
         * @param start_value Value representing inclusive start of the range.
         * @param end_value Value representing inclusive end of the range.
         */
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

        /**
         * @brief Base class for Tree-sitter option wrappers.
         * @tparam T_TSOptions Tree-sitter options structure wrapped by this class.
         * @tparam T_TSState Tree-sitter callback state type associated with the options.
         * @tparam T_CPPState C++ wrapper type constructed from the Tree-sitter callback state.
         */
        template <typename T_TSOptions, typename T_TSState, typename T_CPPState>
        class OptionsBase
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            /// @brief Callback invoked by Tree-sitter while the associated operation is in progress.
            using ProgressCallbackFunction = std::function<bool(T_CPPState *)>;

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Constructs options with no progress callback.
            OptionsBase() : progress_callback_(nullptr), impl_{ std::make_unique<T_TSOptions>() }
            {
                impl_->payload = this;
            }

            /**
             * @brief Constructs options with a progress callback.
             * @param callback Callback invoked by Tree-sitter while the associated operation is in progress.
             */
            OptionsBase(ProgressCallbackFunction callback)
                : progress_callback_(callback ? std::move(callback) : nullptr), impl_{ std::make_unique<T_TSOptions>() }
            {
                impl_->payload = this;
            }

            /**
             * @brief Disables move construction.
             * @param other Options object that would otherwise be used as the source of the move.
             */
            OptionsBase(OptionsBase &&other) = delete;

            /**
             * @brief Disables move assignment.
             * @param other Options object that would otherwise be used as the source of the move.
             * @return Reference to this object.
             */
            OptionsBase &operator=(OptionsBase &&other) = delete;

            /**
             * @brief Disables copy construction.
             * @param other Options object that would otherwise be used as the source of the copy.
             */
            OptionsBase(const OptionsBase &other) = delete;

            /**
             * @brief Disables copy assignment.
             * @param other Options object that would otherwise be used as the source of the copy.
             * @return Reference to this object.
             */
            OptionsBase &operator=(const OptionsBase &other) = delete;

#pragma endregion

#pragma region CALLBACK

            ////////////////////////////////////////////////////////////////
            // Callback
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the currently configured progress callback.
             * @return Reference to the configured progress callback.
             */
            [[nodiscard]] const ProgressCallbackFunction &getProgressCallback() const noexcept
            {
                return progress_callback_;
            }

            /**
             * @brief Sets the progress callback.
             * @param callback Callback invoked by Tree-sitter while the associated operation is in progress.
             *
             * @note Passing an empty callback disables progress reporting.
             */
            void setProgressCallback(ProgressCallbackFunction callback)
            {
                if (!callback)
                {
                    progress_callback_ = nullptr;
                    return;
                }
                progress_callback_ = std::move(callback);
            }

#pragma endregion

#pragma region CONVERSION

            ////////////////////////////////////////////////////////////////
            // Conversion
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Converts the wrapper to a Tree-sitter options value.
             * @return Tree-sitter options structure configured with the current callback.
             */
            [[nodiscard]] operator T_TSOptions() const
            {
                T_TSOptions options{};
                options.payload           = const_cast<OptionsBase *>(this);
                options.progress_callback = progress_callback_ ? &progressCallbackProxy : nullptr;
                return options;
            }

            /**
             * @brief Returns a pointer to the underlying Tree-sitter options structure.
             * @return Pointer to the underlying options structure configured with the current callback.
             */
            [[nodiscard]] operator T_TSOptions *() const noexcept
            {
                impl_->progress_callback = progress_callback_ ? &progressCallbackProxy : nullptr;
                return impl_.get();
            }

#pragma endregion

        private:
#pragma region PRIVATE_METHODS

            ////////////////////////////////////////////////////////////////
            // Private Methods
            ////////////////////////////////////////////////////////////////

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

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            ProgressCallbackFunction     progress_callback_ = nullptr;
            std::unique_ptr<T_TSOptions> impl_{};

#pragma endregion
        };
    } // namespace details

#pragma endregion

#pragma region ENUMS

    /////////////////////////////////////////////////////////////////////////////
    // Enums.
    // Scoped enum wrappers for Tree-sitter constants to ensure type safety.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Specifies the character encoding used for parser input.
    enum class InputEncoding : std::uint8_t
    {
        UTF8    = TSInputEncodingUTF8,    ///< UTF-8 encoded input.
        UTF16LE = TSInputEncodingUTF16LE, ///< UTF-16 little-endian encoded input.
        UTF16BE = TSInputEncodingUTF16BE, ///< UTF-16 big-endian encoded input.
        Custom  = TSInputEncodingCustom   ///< Custom input encoding handled by the caller.
    };

    /// @brief Specifies the type of a symbol in a Tree-sitter grammar.
    enum class SymbolType : std::uint8_t
    {
        TypeRegular   = TSSymbolTypeRegular,   ///< Regular named grammar symbol.
        TypeAnonymous = TSSymbolTypeAnonymous, ///< Anonymous grammar symbol, typically used for punctuation or
                                               ///< literals.
        TypeSupertype = TSSymbolTypeSupertype, ///< Supertype symbol used to group related grammar symbols.
        TypeAuxiliary = TSSymbolTypeAuxiliary, ///< Auxiliary symbol used internally by the grammar.
    };

    /// @brief Specifies the type of diagnostic message produced by the Tree-sitter runtime.
    enum class LogType : std::uint8_t
    {
        Parse = TSLogTypeParse, ///< Message related to parsing.
        Lex   = TSLogTypeLex    ///< Message related to lexical analysis.
    };

    /// @brief Specifies how many times a grammar element may occur.
    enum class Quantifier : std::uint8_t
    {
        Zero       = TSQuantifierZero,       ///< The element occurs zero times.
        ZeroOrOne  = TSQuantifierZeroOrOne,  ///< The element occurs zero or one time.
        ZeroOrMore = TSQuantifierZeroOrMore, ///< The element occurs zero or more times.
        One        = TSQuantifierOne,        ///< The element occurs exactly once.
        OneOrMore  = TSQuantifierOneOrMore,  ///< The element occurs one or more times.
    };

    /// @brief Specifies the kind of step stored in a Tree-sitter query predicate.
    enum class QueryPredicateStepType : std::uint8_t
    {
        Done    = TSQueryPredicateStepTypeDone,    ///< Marks the end of the predicate steps.
        Capture = TSQueryPredicateStepTypeCapture, ///< References a captured node.
        String  = TSQueryPredicateStepTypeString   ///< Contains a string value.
    };

#pragma endregion

#pragma region POINT

    /////////////////////////////////////////////////////////////////////////////
    // Point
    // A location in source code in terms of rows and columns.
    /////////////////////////////////////////////////////////////////////////////

    struct InputEdit;

    /// @brief Represents a position in source code using a row and column.
    struct Point
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Zero-based row of the source position.
        std::uint32_t row;

        /// @brief Zero-based column of the source position.
        std::uint32_t column;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs a point at the origin.
        constexpr Point() noexcept : row(0), column(0)
        {}

        /**
         * @brief Constructs a point from a row and column.
         * @param row_value Zero-based row of the source position.
         * @param column_value Zero-based column of the source position.
         */
        constexpr Point(std::uint32_t row_value, std::uint32_t column_value) noexcept
            : row(row_value), column(column_value)
        {}

        /**
         * @brief Constructs a point from a Tree-sitter source position.
         * @param other Tree-sitter point to copy.
         */
        constexpr Point(const TSPoint &other) noexcept : row(other.row), column(other.column)
        {}

#pragma endregion

#pragma region MANIPULATION

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Applies a source edit to the point and byte offset.
         * @param edit Source edit used to update the point and byte offset.
         * @param byte_offset Byte offset updated according to the source edit.
         */
        void edit(const InputEdit &edit, std::uint32_t &byte_offset);

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether two source positions are equal.
         * @param other Point to compare with.
         * @return True if both points have the same row and column.
         */
        [[nodiscard]] constexpr bool operator==(const Point &other) const noexcept
        {
            return row == other.row && column == other.column;
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Checks whether this point differs from another point.
         * @param other Point to compare with.
         * @return True if the points differ in row or column.
         */
        [[nodiscard]] constexpr bool operator!=(const Point &other) const noexcept
        {
            return !(*this == other);
        }
#endif // !CPP_TS_HAS_CXX20

        /**
         * @brief Checks whether this point is after another point.
         * @param other Point to compare with.
         * @return True if this point has a greater row or, for the same row, a greater column.
         */
        [[nodiscard]] constexpr bool operator>(const Point &other) const noexcept
        {
            return (row > other.row) || (row == other.row && column > other.column);
        }

        /**
         * @brief Checks whether this point is before or equal to another point.
         * @param other Point to compare with.
         * @return True if this point is not after the other point.
         */
        [[nodiscard]] constexpr bool operator<=(const Point &other) const noexcept
        {
            return !(*this > other);
        }

        /**
         * @brief Checks whether this point is before another point.
         * @param other Point to compare with.
         * @return True if this point has a smaller row or, for the same row, a smaller column.
         */
        [[nodiscard]] constexpr bool operator<(const Point &other) const noexcept
        {
            return (row < other.row) || (row == other.row && column < other.column);
        }

        /**
         * @brief Checks whether this point is after or equal to another point.
         * @param other Point to compare with.
         * @return True if this point is not before the other point.
         */
        [[nodiscard]] constexpr bool operator>=(const Point &other) const noexcept
        {
            return !(*this < other);
        }

        /**
         * @brief Checks whether two points form a valid source range.
         * @param start Start point of the range.
         * @param end End point of the range.
         * @return True if the start point is before or equal to the end point.
         */
        [[nodiscard]] static constexpr bool isValidRange(const Point &start, const Point &end) noexcept
        {
            return start <= end;
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the point to a Tree-sitter source position.
         * @return Tree-sitter point containing the same row and column.
         */
        [[nodiscard]] constexpr operator TSPoint() const noexcept
        {
            return { row, column };
        }

#pragma endregion
    };

#pragma endregion

#pragma region INPUT_EDIT

    /////////////////////////////////////////////////////////////////////////////
    // InputEdit
    // Data describing an edit to a source code string.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Describes a change made to a source code string.
     * @details Stores the byte offsets and source positions of the changed range before and after the edit. The old end
     * position must not precede the start position.
     */
    struct InputEdit
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Byte offset at which the edit starts.
        std::uint32_t start_byte;

        /// @brief Byte offset at which the replaced range ends in the old source.
        std::uint32_t old_end_byte;

        /// @brief Byte offset at which the edited range ends in the new source.
        std::uint32_t new_end_byte;

        /// @brief Source position at which the edit starts.
        Point start_point;

        /// @brief Source position at which the replaced range ends in the old source.
        Point old_end_point;

        /// @brief Source position at which the edited range ends in the new source.
        Point new_end_point;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Creates an empty edit with all offsets and positions set to zero.
        constexpr InputEdit() noexcept
            : start_byte(0), old_end_byte(0), new_end_byte(0), start_point{}, old_end_point{}, new_end_point{}
        {}

        /**
         * @brief Creates an edit describing a change to a source code string.
         * @param start_byte_value Byte offset at which the edit starts.
         * @param old_end_byte_value Byte offset at which the replaced range ends in the old source.
         * @param new_end_byte_value Byte offset at which the edited range ends in the new source.
         * @param start_point_value Source position at which the edit starts.
         * @param old_end_point_value Source position at which the replaced range ends in the old source.
         * @param new_end_point_value Source position at which the edited range ends in the new source.
         *
         * @note The old end must be greater than or equal to the start in both byte offset and source position.
         */
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

        /**
         * @brief Creates an edit from its C API representation.
         * @param edit C API edit describing the source change.
         */
        constexpr explicit InputEdit(const TSInputEdit &edit) noexcept
            : start_byte(edit.start_byte)
            , old_end_byte(edit.old_end_byte)
            , new_end_byte(edit.new_end_byte)
            , start_point{ edit.start_point }
            , old_end_point{ edit.old_end_point }
            , new_end_point{ edit.new_end_point }
        {}

#pragma endregion

#pragma region VALIDATION

        ////////////////////////////////////////////////////////////////
        // Validation
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether the edit has valid start and old-end ranges.
         * @return Returns true if the old end does not precede the start in either byte offset or source position,
         * false otherwise.
         */
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return start_byte <= old_end_byte && Point::isValidRange(start_point, old_end_point);
        }

        /**
         * @brief Validates the edit and throws if its ranges are invalid.
         * @throws std::invalid_argument If the old end precedes the start.
         */
        constexpr void validate() const
        {
            if (!isValid())
            {
                throw std::invalid_argument("Tree-sitter: Invalid edit ranges (start > old_end)");
            }
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the edit to its C API representation.
         * @return A `TSInputEdit` containing the same edit information.
         */
        [[nodiscard]] constexpr operator TSInputEdit() const noexcept
        {
            return { start_byte, old_end_byte, new_end_byte, start_point, old_end_point, new_end_point };
        }

#pragma endregion
    };

#pragma endregion

#pragma region RANGE

    /////////////////////////////////////////////////////////////////////////////
    // Range
    // Representation of a range within the source code using points and bytes.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Represents a range within the source code using points and byte offsets.
     * @details The range contains both source positions and byte offsets for its start and end points.
     */
    struct Range
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Start and end positions of the range in the source code.
        Extent<Point> point;

        /// @brief Start and end byte offsets of the range in the source code.
        Extent<std::uint32_t> byte;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Creates a range from source positions and byte offsets.
         * @param point_value Start and end positions of the range.
         * @param byte_value Start and end byte offsets of the range.
         */
        constexpr explicit Range(Extent<Point> point_value, Extent<std::uint32_t> byte_value) noexcept
            : point(point_value), byte(byte_value)
        {}

        /**
         * @brief Creates a range from its C API representation.
         * @param range C API range to copy.
         */
        constexpr explicit Range(const TSRange &range) noexcept
            : point(range.start_point, range.end_point), byte(range.start_byte, range.end_byte)
        {}

#pragma endregion

#pragma region MANIPULATION

        ////////////////////////////////////////////////////////////////
        // Manipulation
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Updates the range to reflect a source edit.
         * @param edit Edit applied to the source code.
         * @throws std::invalid_argument If the edit contains an invalid range.
         */
        void edit(const InputEdit &edit);

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the range to its C API representation.
         * @return A `TSRange` containing the same source positions and byte offsets.
         */
        [[nodiscard]] constexpr operator TSRange() const noexcept
        {
            return { point.start, point.end, byte.start, byte.end };
        }

#pragma endregion
    };

#pragma endregion

#pragma region INPUT

    /////////////////////////////////////////////////////////////////////////////
    // Input
    // A structure that defines how to read and decode source code incrementally.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Defines how source code is read and decoded.
     * @details An input can provide source data through a read callback and optionally decode it using a custom
     * decoder.
     */
    class Input
    {
    public:
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Callback used to provide source code data.
         * @details The callback is requested for a byte offset and source position and must provide the corresponding
         * source data and its length.
         */
        using ReadFunction = std::function<details::StringViewReturn(std::uint32_t, Point, std::uint32_t *)>;

#pragma endregion

#pragma region STATIC_INPUT_POINTER

        ////////////////////////////////////////////////////////////////
        // Static Input Pointer
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
        /**
         * @brief Stores the input currently used for custom decoding.
         *
         * @warning This value is managed internally by the wrapper and is not intended to be modified directly.
         */
        static thread_local const Input *current_input_ptr;
#else
        /**
         * @brief Returns the thread-local input used for custom decoding.
         * @return Reference to the thread-local current input pointer.
         *
         * @warning This value is managed internally by the wrapper and is not intended to be modified directly.
         */
        static const Input *&current_ptr() noexcept
        {
            thread_local const Input *ptr = nullptr;
            return ptr;
        }
#endif // CPP_TS_HAS_CXX17

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Creates an input with UTF-8 encoding and no read callback.
        Input();

        /**
         * @brief Creates an input with a source read callback and specified encoding.
         * @param read Callback used to provide source code data.
         * @param encoding Encoding used by the source code.
         */
        Input(ReadFunction read, InputEncoding encoding);

        /**
         * @brief Creates an input with a source read callback and custom decoder.
         * @param read Callback used to provide source code data.
         * @param decode Callback used to decode source bytes into Unicode code points.
         */
        Input(ReadFunction read, DecodeFunction decode);

        /**
         * @brief Input objects cannot be copied.
         * @param other Input to copy.
         */
        Input(const Input &other) = delete;

        /**
         * @brief Input objects cannot be copy-assigned.
         * @param other Input to copy.
         * @return This operation is not available.
         */
        Input &operator=(const Input &other) = delete;

        /**
         * @brief Creates an input by transferring ownership of another input.
         * @param other Input whose configuration is transferred.
         */
        Input(Input &&other) noexcept;

        /**
         * @brief Transfers the configuration from another input.
         * @param other Input whose configuration is transferred.
         * @return Reference to this input.
         */
        Input &operator=(Input &&other) noexcept;

#pragma endregion

#pragma region ACCESSORS

        ////////////////////////////////////////////////////////////////
        // Accessors
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the source read callback.
         * @return Reference to the configured read callback.
         */
        [[nodiscard]] const ReadFunction &getRead() const noexcept
        {
            return read_;
        }

        /**
         * @brief Sets the source read callback.
         * @param read Callback used to provide source code data.
         *
         * @note A null callback is ignored.
         */
        void setRead(ReadFunction read) noexcept
        {
            if (!read)
            {
                return;
            }
            read_ = std::move(read);
        }

        /**
         * @brief Returns the encoding used by the source code.
         * @return Current source encoding.
         */
        [[nodiscard]] InputEncoding getEncoding() const noexcept
        {
            return encoding_;
        }

        /**
         * @brief Sets the encoding used by the source code.
         * @param encoding Source encoding to use.
         *
         * @note Switching away from Custom silently drops any decode function, since it would otherwise be ignored by
         * the C API without any indication of why.
         */
        void setEncoding(InputEncoding encoding) noexcept
        {
            encoding_ = encoding;
            if (encoding_ != InputEncoding::Custom)
            {
                decode_ = nullptr;
            }
        }

        /**
         * @brief Returns the custom decoder.
         * @return Reference to the configured decode callback.
         */
        [[nodiscard]] const DecodeFunction &getDecode() const noexcept
        {
            return decode_;
        }

        /**
         * @brief Sets the custom decoder and switches the input to custom encoding.
         * @param decode Callback used to decode source bytes into Unicode code points.
         *
         * @note A null callback is ignored.
         */
        void setDecode(DecodeFunction decode)
        {
            if (!decode)
            {
                return;
            }
            decode_   = std::move(decode);
            encoding_ = InputEncoding::Custom;
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the input to its C API representation.
         * @return A `TSInput` configured according to this input.
         */
        [[nodiscard]] operator TSInput() const;

        /**
         * @brief Returns the C API input representation managed by this object.
         * @return Pointer to the underlying `TSInput`.
         */
        [[nodiscard]] operator TSInput *() const noexcept;

#pragma endregion

    private:
#pragma region PROXYS

        ////////////////////////////////////////////////////////////////
        // Proxys
        ////////////////////////////////////////////////////////////////

        [[nodiscard]] static const char *read_proxy(void          *payload,
                                                    std::uint32_t  byte_index,
                                                    TSPoint        position,
                                                    std::uint32_t *bytes_read);

        [[nodiscard]] static std::uint32_t decode_proxy(const std::uint8_t *string,
                                                        std::uint32_t       length,
                                                        std::int32_t       *code_point) noexcept;

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        ReadFunction             read_     = nullptr;
        DecodeFunction           decode_   = nullptr;
        InputEncoding            encoding_ = InputEncoding::UTF8;
        std::unique_ptr<TSInput> impl_{};

#pragma endregion
    };

#pragma endregion

#pragma region PARSE_STATE

    /////////////////////////////////////////////////////////////////////////////
    // ParseState
    // A structure representing the state of the parser at a specific position.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Represents the parser state at a specific position in the source code.
    struct ParseState
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Current byte offset in the source code.
        std::uint32_t current_byte_offset;

        /// @brief Indicates whether a parsing error has occurred.
        bool has_error;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Creates a parse state from a C API state pointer.
         * @param state C API parse state to copy.
         */
        constexpr explicit ParseState(const TSParseState *state) noexcept
            : current_byte_offset(state->current_byte_offset), has_error(state->has_error)
        {}

        /**
         * @brief Creates a parse state from a C API state.
         * @param state C API parse state to copy.
         */
        constexpr explicit ParseState(const TSParseState &state) noexcept
            : current_byte_offset(state.current_byte_offset), has_error(state.has_error)
        {}

#pragma endregion
    };

#pragma endregion

#pragma region PARSE_OPTIONS

    /////////////////////////////////////////////////////////////////////////////
    // ParseOptions
    // Options that control the behavior of the parsing process.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Options that control the behavior of the parsing process.
    using ParseOptions = details::OptionsBase<TSParseOptions, TSParseState, ParseState>;

#pragma endregion

#pragma region LANGUAGE_VIEW

    /////////////////////////////////////////////////////////////////////////////
    // LanguageView
    // Non-owning reference to a Tree-sitter grammar. Does NOT free memory.
    // Equivalent to passing `const TSLanguage*` in C.
    /////////////////////////////////////////////////////////////////////////////

    class LookaheadIterator;

    /**
     * @brief Provides non-owning access to a Tree-sitter language.
     * @details The view does not own the language and does not release it when destroyed.
     */
    class LanguageView
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Creates an invalid language view.
        constexpr LanguageView() noexcept : impl_{ nullptr }
        {}

        /**
         * @brief Creates a non-owning view of a Tree-sitter language.
         * @param language Language to reference. Its lifetime must exceed that of the view.
         *
         * @note Allowing implicit conversions from TSLanguage to Language improves ergonomics for clients using
         * external grammar providers.
         */
        constexpr LanguageView(const TSLanguage *language) noexcept : impl_{ language }
        {}

#pragma endregion

#pragma region COUNT_ACCESSORS

        ////////////////////////////////////////////////////////////////
        // Count Accessors
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the number of symbols defined by the language.
         * @return Number of symbols, or `0` if the view is invalid.
         */
        [[nodiscard]] std::uint32_t getSymbolsCount() const noexcept
        {
            return isValid() ? ts_language_symbol_count(impl_) : 0;
        }

        /**
         * @brief Returns the number of parser states defined by the language.
         * @return Number of states, or `0` if the view is invalid.
         */
        [[nodiscard]] std::uint32_t getStatesCount() const noexcept
        {
            return isValid() ? ts_language_state_count(impl_) : 0;
        }

        /**
         * @brief Returns the number of fields defined by the language.
         * @return Number of fields, or `0` if the view is invalid.
         */
        [[nodiscard]] std::uint32_t getFieldsCount() const noexcept
        {
            return isValid() ? ts_language_field_count(impl_) : 0;
        }

#pragma endregion

#pragma region SYMBOL_RESOLUTION

        ////////////////////////////////////////////////////////////////
        // Symbol Resolution
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the name of a symbol.
         * @param symbol Symbol whose name is requested.
         * @return Symbol name, or an empty string if the view is invalid.
         */
        [[nodiscard]] details::StringViewReturn getSymbolName(Symbol symbol) const;

        /**
         * @brief Returns the type of a symbol.
         * @param symbol Symbol whose type is requested.
         * @return Symbol type, or `SymbolType::TypeRegular` if the view is invalid.
         */
        [[nodiscard]] SymbolType getSymbolType(Symbol symbol) const;

        /**
         * @brief Finds a symbol by its name.
         * @param name Symbol name to find.
         * @param isNamed Whether to search for a named symbol.
         * @return Matching symbol, or `0` if the symbol is not found or the view is invalid.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        [[nodiscard]] Symbol getSymbolForName(details::StringViewParameter name, bool isNamed) const;

#pragma endregion

#pragma region FIELD_RESOLUTION

        ////////////////////////////////////////////////////////////////
        // Field Resolution
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the name of a field by its ID.
         * @param id Field ID whose name is requested.
         * @return Field name, or an empty string if the view is invalid.
         */
        [[nodiscard]] details::StringViewReturn getFieldNameForID(FieldID id) const;

        /**
         * @brief Finds a field ID by its name.
         * @param name Field name to find.
         * @return Matching field ID, or `0` if the field is not found or the view is invalid.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        [[nodiscard]] FieldID getFieldIDForName(details::StringViewParameter name) const;

        /**
         * @brief Returns the names of all fields defined by the language.
         * @return Field names, or an empty vector if the view is invalid or the language has no fields.
         */
        [[nodiscard]] std::vector<details::StringViewReturn> getAllFieldsNames() const;

#pragma endregion

#pragma region TYPE_HIERARCHY

        ////////////////////////////////////////////////////////////////
        // Type Hierarchy
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns all supertypes defined by the language.
         * @return Symbols representing all supertypes, or an empty vector if none are defined.
         */
        [[nodiscard]] std::vector<Symbol> getAllSuperTypes() const noexcept;

        /**
         * @brief Returns all subtypes associated with a supertype.
         * @param supertype Supertype whose subtypes are requested.
         * @return Symbols representing the subtypes, or an empty vector if none are defined.
         */
        [[nodiscard]] std::vector<Symbol> getAllSubTypesForSuperType(Symbol supertype) const noexcept;

#pragma endregion

#pragma region STATE_NAVIGATION

        ////////////////////////////////////////////////////////////////
        // State Navigation
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the next parser state for a symbol.
         * @param state Current parser state.
         * @param symbol Symbol to process.
         * @return Next parser state, or `0` if the view is invalid.
         */
        [[nodiscard]] StateID getNextState(StateID state, Symbol symbol) const noexcept
        {
            return isValid() ? ts_language_next_state(impl_, state, symbol) : 0;
        }

        /**
         * @brief Creates an iterator over symbols valid for a parser state.
         * @param state Parser state whose valid lookahead symbols are requested.
         * @return Iterator over the valid lookahead symbols.
         * @throws std::logic_error If the view is invalid.
         */
        [[nodiscard]] LookaheadIterator getLookaheadIterator(StateID state) const;

#pragma endregion

#pragma region METADATA

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the name of the language.
         * @return Language name, or an empty string if the view is invalid.
         */
        [[nodiscard]] details::StringViewReturn getName() const;

        /**
         * @brief Returns the ABI version of the language.
         * @return Language ABI version, or `0` if the view is invalid.
         */
        [[nodiscard]] Version getVersion() const noexcept
        {
            return isValid() ? ts_language_abi_version(impl_) : 0;
        }

        /**
         * @brief Checks whether the view references a valid language.
         * @return Returns true if the view references a language, false otherwise.
         */
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return impl_ != nullptr;
        }

#if CPP_TS_HAS_CXX17
        /**
         * @brief Returns the version metadata of the language.
         * @return Language metadata, or `std::nullopt` if unavailable.
         */
        [[nodiscard]] std::optional<LanguageMetadata> getMetadata() const noexcept;
#else
        /**
         * @brief Returns the version metadata of the language.
         * @return Language metadata, or a zero-initialized value if unavailable.
         */
        [[nodiscard]] LanguageMetadata getMetadata() const noexcept;
#endif // CPP_TS_HAS_CXX17

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Checks whether the language uses a WebAssembly parser.
         * @return Returns true if the language is a WebAssembly parser, false otherwise.
         */
        [[nodiscard]] bool isWasm() const noexcept
        {
            return isValid() && ts_language_is_wasm(impl_);
        }
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the referenced Tree-sitter language.
         * @return Pointer to the referenced language, or `nullptr` if the view is invalid.
         */
        [[nodiscard]] constexpr operator const TSLanguage *() const noexcept
        {
            return impl_;
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        const TSLanguage *impl_;

#pragma endregion
    };

#pragma endregion

#pragma region LANGUAGE

    /////////////////////////////////////////////////////////////////////////////
    // Language
    // Owning container for a Tree-sitter grammar. Automatically frees memory.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Owns a Tree-sitter language and releases it when destroyed.
     * @details Copies create independent language instances, while moves transfer ownership to the destination object.
     */
    class Language
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Creates an invalid language without an owned resource.
        Language() noexcept : impl_{ nullptr }
        {}

        /**
         * @brief Takes ownership of a Tree-sitter language.
         * @param language Language to take ownership of.
         *
         * @note Protects against the accidental taking over of a pointer that should not be destroyed.
         * @warning The language must be owned by the caller and must not be released separately after ownership is
         * transferred.
         */
        explicit Language(const TSLanguage *language) noexcept : impl_{ language }
        {}

        /**
         * @brief Creates an independent copy of another language.
         * @param other Language to copy.
         */
        Language(const Language &other) noexcept
            : impl_{ other.impl_.get() != nullptr ? ts_language_copy(other.impl_.get()) : nullptr }
        {}

        /**
         * @brief Replaces the current language with an independent copy of another language.
         * @param other Language to copy.
         * @return Reference to this language.
         */
        Language &operator=(const Language &other) noexcept;

        /**
         * @brief Transfers ownership of a language from another object.
         * @param other Language whose resource is transferred.
         */
        Language(Language &&other) noexcept = default;

        /**
         * @brief Transfers ownership of a language from another object.
         * @param other Language whose resource is transferred.
         * @return Reference to this language.
         */
        Language &operator=(Language &&other) noexcept = default;

#pragma endregion

#pragma region METADATA

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns a non-owning view of the language.
         * @return View of the owned language, or an invalid view if no language is owned.
         */
        [[nodiscard]] constexpr LanguageView getView() const noexcept
        {
            return LanguageView(impl_.get());
        }

        /**
         * @brief Checks whether a language is currently owned.
         * @return Returns true if a language is owned, false otherwise.
         */
        [[nodiscard]] bool isValid() const noexcept
        {
            return impl_.get() != nullptr;
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the language to a non-owning view.
         * @return View of the owned language, or an invalid view if no language is owned.
         */
        [[nodiscard]] operator LanguageView() const noexcept
        {
            return LanguageView(impl_.get());
        }

        /**
         * @brief Returns the owned language as a C API pointer.
         * @return Pointer to the owned language, or `nullptr` if no language is owned.
         */
        [[nodiscard]] operator const TSLanguage *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<const TSLanguage, details::FreeHelper> impl_{ nullptr };

#pragma endregion
    };

#pragma endregion

#pragma region NODE

    /////////////////////////////////////////////////////////////////////////////
    // Node
    // A single node in a parse tree, providing navigation and attributes.
    /////////////////////////////////////////////////////////////////////////////

    class TreeCursor;

    /**
     * @brief Represents a node in a parse tree.
     * @details A node provides access to its source range, type, grammar information, children, parent, siblings, and
     * other related parse-tree data.
     */
    class Node
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Creates a null node.
        constexpr Node() noexcept : impl_{}
        {}

        /**
         * @brief Creates a node from its C API representation.
         * @param node C API node to wrap.
         */
        constexpr explicit Node(TSNode node) noexcept : impl_{ node }
        {}

        /**
         * @brief Creates a null node.
         * @return A null node.
         */
        [[nodiscard]] static constexpr Node null() noexcept
        {
            return Node();
        }

#pragma endregion

#pragma region METADATA

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the unique identifier of the node.
         * @return Node identifier, or `0` for a null node.
         */
        [[nodiscard]] constexpr NodeID getID() const noexcept
        {
            return isNull() ? 0 : reinterpret_cast<NodeID>(impl_.id);
        }

        /**
         * @brief Returns the node type.
         * @return Node type, or an empty string for a null node.
         */
        [[nodiscard]] details::StringViewReturn getType() const
        {
            return isNull() ? details::StringViewReturn("") : details::make_view(ts_node_type(impl_));
        }

        /**
         * @brief Returns the grammar symbol associated with the node.
         * @return Node symbol, or `0` for a null node.
         */
        [[nodiscard]] Symbol getSymbol() const noexcept
        {
            return isNull() ? 0 : ts_node_symbol(impl_);
        }

        /**
         * @brief Returns the grammar type of the node.
         * @return Grammar type, or an empty string for a null node.
         */
        [[nodiscard]] details::StringViewReturn getGrammarType() const
        {
            return isNull() ? details::StringViewReturn("") : details::make_view(ts_node_grammar_type(impl_));
        }

        /**
         * @brief Returns the grammar symbol associated with the node.
         * @return Grammar symbol, or `0` for a null node.
         */
        [[nodiscard]] Symbol getGrammarSymbol() const noexcept
        {
            return isNull() ? 0 : ts_node_grammar_symbol(impl_);
        }

        /**
         * @brief Returns the language associated with the node.
         * @return Non-owning view of the node's language, or an invalid view for a null node.
         */
        [[nodiscard]] LanguageView getLanguage() const noexcept
        {
            return isNull() ? LanguageView(nullptr) : LanguageView(ts_node_language(impl_));
        }

#pragma endregion

#pragma region FLAG_CHECKS

        ////////////////////////////////////////////////////////////////
        // Flag Checks
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether the node is null.
         * @return Returns true if the node is null, false otherwise.
         */
        [[nodiscard]] bool isNull() const noexcept
        {
            return ts_node_is_null(impl_);
        }

        /**
         * @brief Checks whether the node is named.
         * @return Returns true if the node is named and not null, false otherwise.
         */
        [[nodiscard]] bool isNamed() const noexcept
        {
            return !isNull() && ts_node_is_named(impl_);
        }

        /**
         * @brief Checks whether the node was inserted as a missing node during parsing.
         * @return Returns true if the node is missing and not null, false otherwise.
         */
        [[nodiscard]] bool isMissing() const noexcept
        {
            return !isNull() && ts_node_is_missing(impl_);
        }

        /**
         * @brief Checks whether the node is an extra node.
         * @return Returns true if the node is extra and not null, false otherwise.
         */
        [[nodiscard]] bool isExtra() const noexcept
        {
            return !isNull() && ts_node_is_extra(impl_);
        }

        /**
         * @brief Checks whether the node represents a syntax error.
         * @return Returns true if the node is an error node and not null, false otherwise.
         */
        [[nodiscard]] bool isError() const noexcept
        {
            return !isNull() && ts_node_is_error(impl_);
        }

        /**
         * @brief Checks whether the node or any of its descendants contains an error.
         * @return Returns true if the node contains an error and is not null, false otherwise.
         */
        [[nodiscard]] bool hasError() const noexcept
        {
            return !isNull() && ts_node_has_error(impl_);
        }

        /**
         * @brief Checks whether the node has been changed by a tree edit.
         * @return Returns true if the node has changes and is not null, false otherwise.
         */
        [[nodiscard]] bool hasChanges() const noexcept
        {
            return !isNull() && ts_node_has_changes(impl_);
        }

#pragma endregion

#pragma region RANGES

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the byte range occupied by the node.
         * @return Start and end byte offsets, or an empty range for a null node.
         */
        [[nodiscard]] Extent<std::uint32_t> getByteRange() const noexcept
        {
            return isNull() ? Extent<std::uint32_t>()
                            : Extent<std::uint32_t>({ ts_node_start_byte(impl_), ts_node_end_byte(impl_) });
        }

        /**
         * @brief Returns the source position range occupied by the node.
         * @return Start and end source positions, or an empty range for a null node.
         */
        [[nodiscard]] Extent<Point> getPointRange() const noexcept
        {
            return isNull() ? Extent<Point>() : Extent<Point>({ ts_node_start_point(impl_), ts_node_end_point(impl_) });
        }

#pragma endregion

#pragma region SOURCE_TEXT_ACCESS

        ////////////////////////////////////////////////////////////////
        // Source Text Access
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the part of the source text covered by the node.
         * @param source Complete source text from which the node originates.
         * @return A view of the source text covered by the node, or an empty view if the node is null or its byte range
         * exceeds the source size.
         */
        [[nodiscard]] details::StringViewReturn getSourceRange(details::StringViewParameter source) const;

        /**
         * @brief Returns a copy of the source text covered by the node.
         * @param source Complete source text from which the node originates.
         * @return Source text covered by the node, or an empty string for a null node or an invalid byte range.
         */
        [[nodiscard]] std::string getSourceText(details::StringViewParameter source) const;

        /**
         * @brief Returns an S-expression representation of the node and its descendants.
         * @return A null pointer for a null node, the S-expression representation otherwise.
         */
        [[nodiscard]] std::unique_ptr<char, details::FreeHelper> getSExpr() const
        {
            return isNull() ? nullptr : std::unique_ptr<char, details::FreeHelper>{ ts_node_string(impl_) };
        }

#pragma endregion

#pragma region NAVIGATION_GENERAL

        ////////////////////////////////////////////////////////////////
        // Navigation (General)
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the parent node.
         * @return Parent node, or a null node if the node has no parent.
         */
        [[nodiscard]] Node getParent() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_parent(impl_) };
        }

        /**
         * @brief Returns the next sibling.
         * @return Next sibling, or a null node if none exists.
         */
        [[nodiscard]] Node getNextSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_next_sibling(impl_) };
        }

        /**
         * @brief Returns the previous sibling.
         * @return Previous sibling, or a null node if none exists.
         */
        [[nodiscard]] Node getPreviousSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_prev_sibling(impl_) };
        }

        /**
         * @brief Returns the number of children.
         * @return Number of children, or `0` for a null node.
         */
        [[nodiscard]] std::uint32_t getChildCount() const noexcept
        {
            return isNull() ? 0 : ts_node_child_count(impl_);
        }

        /**
         * @brief Returns a child by its index.
         * @param child_index Zero-based child index.
         * @return Child at the specified index, or a null node if the index is out of range.
         */
        [[nodiscard]] Node getChild(std::uint32_t child_index) const noexcept
        {
            return (child_index >= getChildCount()) ? null() : Node{ ts_node_child(impl_, child_index) };
        }

        /**
         * @brief Returns the first child.
         * @return First child, or a null node if there are no children.
         */
        [[nodiscard]] Node getFirstChild() const noexcept
        {
            return getChild(0);
        }

        /**
         * @brief Returns the last child.
         * @return Last child, or a null node if there are no children.
         */
        [[nodiscard]] Node getLastChild() const noexcept
        {
            std::uint32_t count = getChildCount();
            return getChild(count > 0 ? count - 1 : count);
        }

        /**
         * @brief Returns the child that contains a specified descendant.
         * @param descendant Descendant node to locate.
         * @return The direct child containing the descendant, or a null node if none is found.
         */
        [[nodiscard]] Node getChildWithDescendant(Node descendant) const noexcept
        {
            return (isNull() || descendant.isNull()) ? null()
                                                     : Node{ ts_node_child_with_descendant(impl_, descendant.impl_) };
        }

        /**
         * @brief Returns the first child with the specified type.
         * @param name Node type name to find.
         * @return Matching child, or a null node if none is found.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        [[nodiscard]] Node getChildByType(details::StringViewParameter name) const;

        /**
         * @brief Returns the first child with the specified symbol.
         * @param sym Symbol to find.
         * @return Matching child, or a null node if none is found.
         */
        [[nodiscard]] Node getChildBySymbol(Symbol sym) const noexcept;

#pragma endregion

#pragma region NAVIGATION_NAMED_CHILDREN

        ////////////////////////////////////////////////////////////////
        // Navigation (Named Children)
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the next named sibling.
         * @return Next named sibling, or a null node if none exists.
         */
        [[nodiscard]] Node getNextNamedSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_next_named_sibling(impl_) };
        }

        /**
         * @brief Returns the previous named sibling.
         * @return Previous named sibling, or a null node if none exists.
         */
        [[nodiscard]] Node getPreviousNamedSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_prev_named_sibling(impl_) };
        }

        /**
         * @brief Returns the number of named children.
         * @return Number of named children, or `0` for a null node.
         */
        [[nodiscard]] std::uint32_t getNamedChildCount() const noexcept
        {
            return isNull() ? 0 : ts_node_named_child_count(impl_);
        }

        /**
         * @brief Returns a named child by its index.
         * @param child_index Zero-based named child index.
         * @return Named child at the specified index, or a null node if the index is out of range.
         */
        [[nodiscard]] Node getNamedChild(std::uint32_t child_index) const noexcept
        {
            return (child_index >= getNamedChildCount()) ? null() : Node{ ts_node_named_child(impl_, child_index) };
        }

        /**
         * @brief Returns the first named child.
         * @return First named child, or a null node if there are no named children.
         */
        [[nodiscard]] Node getFirstNamedChild() const noexcept
        {
            return getNamedChild(0);
        }

        /**
         * @brief Returns the last named child.
         * @return Last named child, or a null node if there are no named children.
         */
        [[nodiscard]] Node getLastNamedChild() const noexcept
        {
            std::uint32_t count = getNamedChildCount();
            return getNamedChild(count > 0 ? count - 1 : count);
        }

        /**
         * @brief Returns the first named child with the specified type.
         * @param name Node type name to find.
         * @return Matching named child, or a null node if none is found.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        [[nodiscard]] Node getNamedChildByType(details::StringViewParameter name) const;

        /**
         * @brief Returns the first named child with the specified symbol.
         * @param sym Symbol to find.
         * @return Matching named child, or a null node if none is found.
         */
        [[nodiscard]] Node getNamedChildBySymbol(Symbol sym) const noexcept;

#pragma endregion

#pragma region SEARCH_AND_DESCENDANTS

        ////////////////////////////////////////////////////////////////
        // Search & Descendants
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the number of descendants of the node.
         * @return Number of descendants, or `0` for a null node.
         */
        [[nodiscard]] std::uint32_t getDescendantsCount() const noexcept
        {
            return isNull() ? 0 : ts_node_descendant_count(impl_);
        }

        /**
         * @brief Returns the first child that extends to the specified byte offset.
         * @param byte Byte offset to search.
         * @return First matching child, or a null node if none is found.
         */
        [[nodiscard]] Node getFirstChildForByte(std::uint32_t byte) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_first_child_for_byte(impl_, byte) };
        }

        /**
         * @brief Returns the first named child that extends to the specified byte offset.
         * @param byte Byte offset to search.
         * @return First matching named child, or a null node if none is found.
         */
        [[nodiscard]] Node getFirstNamedChildForByte(std::uint32_t byte) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_first_named_child_for_byte(impl_, byte) };
        }

        /**
         * @brief Returns the smallest descendant covering a byte range.
         * @param range Byte range to search.
         * @return Matching descendant, or a null node if the node is null.
         */
        [[nodiscard]] Node getDescendantForByteRange(Extent<std::uint32_t> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_descendant_for_byte_range(impl_, range.start, range.end) };
        }

        /**
         * @brief Returns the smallest named descendant covering a byte range.
         * @param range Byte range to search.
         * @return Matching named descendant, or a null node if the node is null.
         */
        [[nodiscard]] Node getNamedDescendantForByteRange(Extent<std::uint32_t> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_named_descendant_for_byte_range(impl_, range.start, range.end) };
        }

        /**
         * @brief Returns the smallest descendant covering a source position range.
         * @param range Source position range to search.
         * @return Matching descendant, or a null node if the node is null.
         */
        [[nodiscard]] Node getDescendantForPointRange(Extent<Point> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_descendant_for_point_range(impl_, range.start, range.end) };
        }

        /**
         * @brief Returns the smallest named descendant covering a source position range.
         * @param range Source position range to search.
         * @return Matching named descendant, or a null node if the node is null.
         */
        [[nodiscard]] Node getNamedDescendantForPointRange(Extent<Point> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_named_descendant_for_point_range(impl_, range.start, range.end) };
        }

#pragma endregion

#pragma region FIELDS_OPERATORS

        ////////////////////////////////////////////////////////////////
        // Fields Operators
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the field name associated with a child.
         * @param child_index Zero-based child index.
         * @return Field name, or an empty string if no field is associated with the child.
         */
        [[nodiscard]] details::StringViewReturn getFieldNameForChild(std::uint32_t child_index) const
        {
            return isNull() ? details::StringViewReturn("")
                            : details::make_view(ts_node_field_name_for_child(impl_, child_index));
        }

        /**
         * @brief Returns the field name associated with a named child.
         * @param named_child_index Zero-based named child index.
         * @return Field name, or an empty string if no field is associated with the child.
         */
        [[nodiscard]] details::StringViewReturn getFieldNameForNamedChild(std::uint32_t named_child_index) const
        {
            return isNull() ? details::StringViewReturn("")
                            : details::make_view(ts_node_field_name_for_named_child(impl_, named_child_index));
        }

        /**
         * @brief Returns the child associated with a field name.
         * @param name Field name to find.
         * @return Child associated with the field, or a null node if none is found.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        [[nodiscard]] Node getChildByFieldName(details::StringViewParameter name) const;

        /**
         * @brief Returns the child associated with a field ID.
         * @param field_id Field ID to find.
         * @return Child associated with the field, or a null node if none is found.
         */
        [[nodiscard]] Node getChildByFieldID(FieldID field_id) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_child_by_field_id(impl_, field_id) };
        }

#pragma endregion

#pragma region PARSING_STATE

        ////////////////////////////////////////////////////////////////
        // Parsing State
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the parser state associated with the node.
         * @return Parser state, or `0` for a null node.
         */
        [[nodiscard]] StateID getParseState() const noexcept
        {
            return isNull() ? 0 : ts_node_parse_state(impl_);
        }

        /**
         * @brief Returns the parser state associated with the next parse step.
         * @return Next parser state, or `0` for a null node.
         */
        [[nodiscard]] StateID getNextParseState() const noexcept
        {
            return isNull() ? 0 : ts_node_next_parse_state(impl_);
        }

#pragma endregion

#pragma region MUTATIONS

        ////////////////////////////////////////////////////////////////
        // Mutations
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Updates the node to reflect an edit to the source code.
         * @param edit Edit applied to the source code.
         * @throws std::logic_error If the node is null.
         * @throws std::invalid_argument If the edit contains invalid ranges.
         */
        void edit(const InputEdit &edit);

#pragma endregion

#pragma region CURSOR

        ////////////////////////////////////////////////////////////////
        // Cursor
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Creates a tree cursor positioned at this node.
         * @return Cursor positioned at this node.
         */
        [[nodiscard]] TreeCursor getCursor() const noexcept;

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether two nodes represent the same parse-tree node.
         * @param other Node to compare with.
         * @return Returns true if both nodes represent the same node, false otherwise.
         */
        [[nodiscard]] bool operator==(const Node &other) const noexcept
        {
            return ts_node_eq(impl_, other.impl_);
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Checks whether two nodes represent different parse-tree nodes.
         * @param other Node to compare with.
         * @return Returns true if the nodes are different, false otherwise.
         */
        [[nodiscard]] bool operator!=(const Node &other) const noexcept
        {
            return !(*this == other);
        }
#endif // !CPP_TS_HAS_CXX20

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the node to its C API representation.
         * @return C API node representation.
         */
        [[nodiscard]] constexpr operator TSNode() const noexcept
        {
            return impl_;
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        TSNode impl_{};

#pragma endregion
    };

#pragma endregion

#pragma region TREE

    /////////////////////////////////////////////////////////////////////////////
    // Tree
    // Manages the lifetime of a complete parse tree.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Owns a complete parse tree and provides access to its root, language, ranges, and edits.
    class Tree
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Takes ownership of a parse tree.
         * @param tree Parse tree to own. May be null.
         */
        explicit Tree(TSTree *tree) noexcept : impl_{ tree, ts_tree_delete }, is_valid_(tree != nullptr)
        {}

        /**
         * @brief Creates an independent copy of another parse tree.
         * @param other Tree to copy.
         */
        Tree(const Tree &other) noexcept;

        /**
         * @brief Replaces this tree with an independent copy of another parse tree.
         * @param other Tree to copy.
         * @return Reference to this tree.
         */
        Tree &operator=(const Tree &other);

        /**
         * @brief Transfers ownership of a parse tree from another object.
         * @param other Tree whose contents are moved.
         */
        Tree(Tree &&other) noexcept;

        /**
         * @brief Replaces this tree by transferring ownership from another object.
         * @param other Tree whose contents are moved.
         * @return Reference to this tree.
         */
        Tree &operator=(Tree &&other) noexcept;

#pragma endregion

#pragma region FLAGS

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether the tree contains a syntax error.
         * @return Returns true if the tree contains an error, false otherwise.
         */
        [[nodiscard]] bool hasError() const noexcept
        {
            return isValid() && getRootNode().hasError();
        }

        /**
         * @brief Checks whether the tree contains a valid parse tree.
         * @return Returns true if the tree is valid, false otherwise.
         */
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return is_valid_;
        }

#pragma endregion

#pragma region ROOT

        ////////////////////////////////////////////////////////////////
        // Root
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the root node of the parse tree.
         * @return Root node, or a null node if the tree is invalid.
         */
        [[nodiscard]] Node getRootNode() const noexcept
        {
            return isValid() ? Node{ ts_tree_root_node(impl_.get()) } : Node::null();
        }

        /**
         * @brief Returns the root node with its position shifted by the given byte and point offsets.
         * @param offset_bytes Byte offset to add to the root node positions.
         * @param offset_extent Point offset to add to the root node positions.
         * @return Root node with adjusted positions, or a null node if the tree is invalid.
         */
        [[nodiscard]] Node getRootNodeWithOffset(std::uint32_t offset_bytes, Point offset_extent) const noexcept
        {
            return isValid() ? Node{ ts_tree_root_node_with_offset(impl_.get(), offset_bytes, offset_extent) }
                             : Node::null();
        }

#pragma endregion

#pragma region MUTATIONS

        ////////////////////////////////////////////////////////////////
        // Mutations
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Applies an edit to the tree so its node positions remain consistent with the edited source.
         * @param edit Description of the source edit.
         * @throws std::logic_error If the tree is invalid.
         * @throws std::invalid_argument If the edit contains invalid ranges.
         */
        void edit(const InputEdit &edit);

#pragma endregion

#pragma region RANGES

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the ranges of source code included in the tree.
         * @return Vector of included source ranges, or an empty vector if the tree is invalid.
         */
        [[nodiscard]] std::vector<Range> getIncludedRanges() const;

        /**
         * @brief Returns the ranges that differ between two parse trees.
         * @param old_tree Previous parse tree.
         * @param new_tree Updated parse tree.
         * @return Vector of ranges that changed between the trees.
         */
        [[nodiscard]] static std::vector<Range> getChangedRanges(const Tree &old_tree, const Tree &new_tree);

#pragma endregion

#pragma region LANGUAGE

        ////////////////////////////////////////////////////////////////
        // Language
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the grammar used to create the tree.
         * @return Non-owning view of the tree's grammar, or an invalid view if the tree is invalid.
         */
        [[nodiscard]] LanguageView getLanguage() const noexcept
        {
            return isValid() ? LanguageView(ts_tree_language(impl_.get())) : LanguageView(nullptr);
        }

#pragma endregion

#pragma region COPY

        ////////////////////////////////////////////////////////////////
        // Copy
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Creates an independent copy of the parse tree.
         * @return Copy of this tree.
         */
        [[nodiscard]] Tree copy() const noexcept
        {
            return Tree(*this);
        }

#pragma endregion

#pragma region DEBUGGING

        ////////////////////////////////////////////////////////////////
        // Debugging
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Writes a graph representation of the tree to a file descriptor.
         * @param file_descriptor File descriptor to which the graph is written.
         */
        void printDotGraph(int file_descriptor) noexcept
        {
            if (!isValid())
            {
                return;
            }
            ts_tree_print_dot_graph(impl_.get(), file_descriptor);
        }

        /**
         * @brief Writes a graph representation of the tree to a file.
         * @param file File to which the graph is written.
         */
        void printDotGraph(FILE *file)
        {
            if (!isValid() || file == nullptr)
            {
                return;
            }

            printDotGraph(CPP_TS_FILENO(file));
        }

        /**
         * @brief Writes a graph representation of the tree to a file at the specified path.
         * @param path Path of the output file.
         */
        void printDotGraph(details::StringViewParameter path);

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the underlying Tree-sitter parse tree.
         * @return Pointer to the underlying parse tree, or nullptr if the tree is invalid.
         */
        [[nodiscard]] operator TSTree *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<TSTree, decltype(&ts_tree_delete)> impl_{ nullptr, ts_tree_delete };
        bool                                               is_valid_ = false;

#pragma endregion
    };

#pragma endregion

#pragma region PARSER

    /////////////////////////////////////////////////////////////////////////////
    // Parser
    // State machine used to produce a syntax tree from source code.
    /////////////////////////////////////////////////////////////////////////////

#if defined(CPP_TS_FEATURE_WASM)
    class WasmStore;
#endif // defined(CPP_TS_FEATURE_WASM)

    /// @brief A state machine used to produce a syntax tree from source code.
    class Parser
    {
    public:
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        /// @brief Function signature used for logging parser operations.
        using LoggerFunction = std::function<void(LogType, const char *)>;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs a new empty parser.
        Parser() noexcept : impl_{ ts_parser_new(), ts_parser_delete }
        {}

        /**
         * @brief Constructs a parser and assigns a language to it.
         * @param language The language to be used by the parser.
         * @throws std::runtime_error If the language is invalid or if there is a version mismatch.
         */
        explicit Parser(LanguageView language);

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Constructs a parser with a WebAssembly language and store.
         * @param language The WebAssembly language to be used.
         * @param store The WebAssembly store.
         * @throws std::runtime_error If the language is invalid or if there is a version mismatch.
         */
        Parser(LanguageView language, WasmStore &store);
#endif // defined(CPP_TS_FEATURE_WASM)

        /**
         * @brief Deleted copy constructor.
         * @param other Another parser.
         */
        Parser(const Parser &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another parser.
         * @return Returns a reference to this parser.
         */
        Parser &operator=(const Parser &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another parser to move from.
         */
        Parser(Parser &&other) noexcept;

        /**
         * @brief Move assignment operator.
         * @param other Another parser to move from.
         * @return Returns a reference to this parser.
         */
        Parser &operator=(Parser &&other) noexcept;

#if CPP_TS_HAS_CXX23
        /**
         * @brief Creates a new parser with the specified language.
         * @param language The language to assign.
         * @return Returns a parser instance, or an error string if the language is invalid or incompatible.
         */
        [[nodiscard]] static std::expected<Parser, std::string> create(LanguageView language) noexcept;

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Creates a new parser with the specified WebAssembly language and store.
         * @param language The WebAssembly language to assign.
         * @param store The WebAssembly store to use.
         * @return Returns a parser instance, or an error string if the language is invalid or incompatible.
         */
        [[nodiscard]] static std::expected<Parser, std::string> create(LanguageView language,
                                                                       WasmStore   &store) noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_CXX23

#pragma endregion

#pragma region CONFIGURATION

        ////////////////////////////////////////////////////////////////
        // Configuration
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX23
        /**
         * @brief Sets the language that the parser will use.
         * @param language The language to assign.
         * @return Returns true on success, or an error string on failure (e.g., ABI incompatibility).
         */
        [[nodiscard]] std::expected<bool, std::string> setLanguage(LanguageView language);
#else
        /**
         * @brief Sets the language that the parser will use.
         * @param language The language to assign.
         * @return Returns true on success, or false on ABI incompatibility.
         * @throws std::logic_error If the language is WebAssembly and the parser lacks a `WasmStore` (only when WASM is
         * enabled).
         */
        [[nodiscard]] bool setLanguage(LanguageView language)
#if defined(CPP_TS_FEATURE_WASM)
                ;
#else
                noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_CXX23

#if CPP_TS_HAS_CXX20
        /**
         * @brief Restricts parsing to a specific set of text ranges.
         * @param ranges A span of ranges indicating the portions of text to parse.
         * @return Returns true if ranges were set successfully.
         * @throws std::length_error If the underlying merged range count exceeds the maximum size (UINT32_MAX).
         */
        [[nodiscard]] bool setIncludedRanges(std::span<const Range> ranges);
#else
        /**
         * @brief Restricts parsing to a specific set of text ranges.
         * @param ranges A vector of ranges indicating the portions of text to parse.
         * @return Returns true if ranges were set successfully.
         * @throws std::length_error If the underlying merged range count exceeds the maximum size (UINT32_MAX).
         */
        [[nodiscard]] bool setIncludedRanges(const std::vector<Range> &ranges);
#endif // CPP_TS_HAS_CXX20

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Assigns a WebAssembly store to the parser.
         * @param store The WebAssembly store to take ownership of.
         *
         * @note Passing a `WasmStore` invalidates it on the caller side. The parser takes control of the C
         * `TSWasmStore`. You can retrieve it using the `takeWasmStore()` function. If you set a new `WasmStore` without
         * retrieving the previous one, the previous instance will be discarded.
         */
        void setWasmStore(WasmStore &store) noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)

        /**
         * @brief Gets the currently assigned language.
         * @return Returns the current language, or an invalid language view if none is set.
         */
        [[nodiscard]] LanguageView getCurrentLanguage() const noexcept
        {
            return hasLanguage() ? LanguageView(ts_parser_language(impl_.get())) : LanguageView(nullptr);
        }

        /**
         * @brief Retrieves the ranges of text that the parser will include when parsing.
         * @return Returns a vector of included ranges.
         */
        [[nodiscard]] std::vector<Range> getIncludedRanges() const noexcept;

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Retrieves the WebAssembly store from the parser.
         * @return Returns the `WasmStore` that was previously owned by this parser.
         *
         * @note If the current language is WebAssembly, it will be removed from the parser during this operation.
         */
        [[nodiscard]] WasmStore takeWasmStore() noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

#pragma region FLAGS

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks if a language has been set for the parser.
         * @return Returns true if a language is currently assigned, false otherwise.
         */
        [[nodiscard]] constexpr bool hasLanguage() const noexcept
        {
            return language_set_;
        }

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Checks if a WebAssembly store has been assigned to the parser.
         * @return Returns true if a store is currently assigned, false otherwise.
         */
        [[nodiscard]] constexpr bool hasWasmStore() const noexcept
        {
            return wasm_store_set_;
        }
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

#pragma region PARSING

        ////////////////////////////////////////////////////////////////
        // Parsing
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Parses source code using a custom input provider.
         * @param input The custom input source to read from.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @param options Optional advanced parsing options (like parsing a specific `TSParseOptions` config).
         * @return Returns the newly generated syntax tree.
         * @throws std::logic_error If no language has been set prior to calling this method.
         */
        [[nodiscard]] Tree parse(Input                                     &input,
                                 details::OptionalParam<Tree>               old_tree = {},
                                 details::OptionalParam<const ParseOptions> options  = {});

        /**
         * @brief Parses source code provided as a contiguous string.
         * @param buffer The string buffer containing the source code to parse.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns the newly generated syntax tree.
         * @throws std::logic_error If no language has been set prior to calling this method.
         * @throws std::length_error If the input buffer size exceeds 4GB.
         */
        [[nodiscard]] Tree parseString(details::StringViewParameter buffer, details::OptionalParam<Tree> old_tree = {});

        /**
         * @brief Parses encoded source code provided as a contiguous string.
         * @param buffer The string buffer containing the encoded source code to parse.
         * @param encoding The character encoding format of the buffer.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns the newly generated syntax tree.
         * @throws std::logic_error If no language has been set prior to calling this method.
         * @throws std::length_error If the input buffer size exceeds 4GB.
         */
        [[nodiscard]] Tree parseStringEncoded(details::StringViewParameter buffer,
                                              InputEncoding                encoding,
                                              details::OptionalParam<Tree> old_tree = {});

#pragma endregion

#pragma region DEBUGGING

        ////////////////////////////////////////////////////////////////
        // Debugging
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Enables generation of DOT graphs into a specific file descriptor for debugging.
         * @param file_descriptor The file descriptor where the DOT graph will be written.
         * @throws std::runtime_error If the file descriptor fails to duplicate.
         */
        void enableDotGraphs(int file_descriptor);

        /**
         * @brief Enables generation of DOT graphs into a C standard file pointer for debugging.
         * @param file The file pointer where the DOT graph will be written.
         */
        void enableDotGraphs(FILE *file)
        {
            if (!file)
            {
                return;
            }

            enableDotGraphs(CPP_TS_FILENO(file));
        }

        /**
         * @brief Enables generation of DOT graphs into a file at the specified path for debugging.
         * @param path The file system path where the DOT graph will be saved.
         */
        void enableDotGraphs(details::StringViewParameter path);

        /// @brief Disables the generation of DOT graphs.
        void disableDotGraphs()
        {
            enableDotGraphs(-1);
        }

#pragma endregion

#pragma region LOGGING

        ////////////////////////////////////////////////////////////////
        // Logging
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Sets a custom logging callback for parser operations.
         * @param logger_func The callable object that will handle incoming log messages.
         */
        void setLogger(LoggerFunction logger_func);

        /// @brief Removes the currently assigned logging callback.
        void removeLogger() noexcept
        {
            current_logger_ = nullptr;
            ts_parser_set_logger(impl_.get(), { nullptr, nullptr });
        }

#pragma endregion

#pragma region RESET

        ////////////////////////////////////////////////////////////////
        // Reset
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Resets the parser to its initial state, discarding any cached data.
         *
         * @note This function is useful when reusing the same parser for disjoint tasks. It does not remove the current
         * logger.
         */
        void reset() noexcept
        {
            ts_parser_reset(impl_.get());
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<TSParser, decltype(&ts_parser_delete)> impl_{ nullptr, ts_parser_delete };
        std::function<void(LogType, const char *)>             current_logger_ = nullptr;
        bool                                                   language_set_   = false;
#if defined(CPP_TS_FEATURE_WASM)
        bool wasm_store_set_ = false;
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion
    };

#pragma endregion

#pragma region TREE_CURSOR

    /////////////////////////////////////////////////////////////////////////////
    // TreeCursor
    // A stateful pointer for walking a syntax tree efficiently.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A stateful pointer for walking a syntax tree efficiently.
    class TreeCursor
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty, invalid tree cursor.
        TreeCursor() noexcept : impl_{}, is_valid_(false)
        {}

        /**
         * @brief Constructs a tree cursor starting at the given native node.
         * @param node The native `TSNode` to start traversing from.
         */
        explicit TreeCursor(TSNode node) noexcept;

        /**
         * @brief Constructs a tree cursor from an existing native cursor.
         * @param cursor The native `TSTreeCursor` to copy from.
         */
        TreeCursor(const TSTreeCursor &cursor) noexcept;

        /**
         * @brief Deleted copy constructor.
         * @param other Another tree cursor.
         */
        TreeCursor(const TreeCursor &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another tree cursor.
         * @return Returns a reference to this tree cursor.
         */
        TreeCursor &operator=(const TreeCursor &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another tree cursor to move from.
         */
        TreeCursor(TreeCursor &&other) noexcept : TreeCursor()
        {
            std::swap(impl_, other.impl_);
            std::swap(is_valid_, other.is_valid_);
        }

        /**
         * @brief Move assignment operator.
         * @param other Another tree cursor to move from.
         * @return Returns a reference to this tree cursor.
         */
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

        /// @brief Destroys the tree cursor and frees underlying resources.
        ~TreeCursor() noexcept
        {
            if (!isValid())
            {
                return;
            }
            ts_tree_cursor_delete(&impl_);
        }

#pragma endregion

#pragma region FLAGS

        ////////////////////////////////////////////////////////////////
        // Flags
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks if the cursor is valid and ready to use.
         * @return Returns true if the cursor is valid, false otherwise.
         */
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return is_valid_;
        }

#pragma endregion

#pragma region STATE_CONTROL

        ////////////////////////////////////////////////////////////////
        // State Control
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Resets the cursor to start pointing to a specific node.
         * @param node The node to reset the cursor to.
         */
        void reset(Node node) noexcept
        {
            if (!isValid() || node.isNull())
            {
                return;
            }
            ts_tree_cursor_reset(&impl_, node);
        }

        /**
         * @brief Resets the cursor's state to match another cursor.
         * @param cursor The cursor to copy the state from.
         */
        void reset(TreeCursor &cursor) noexcept
        {
            if (!isValid() || !cursor.isValid())
            {
                return;
            }
            ts_tree_cursor_reset_to(&impl_, &cursor.impl_);
        }

        /**
         * @brief Creates a new cursor with the exact same state as the current one.
         * @return Returns a newly copied `TreeCursor`.
         */
        [[nodiscard]] TreeCursor copy() const noexcept
        {
            return TreeCursor(impl_);
        }

#pragma endregion

#pragma region NAVIGATION

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Moves the cursor to the parent of its current node.
         * @return Returns true if the cursor successfully moved, false if there is no parent.
         */
        [[nodiscard]] bool gotoParent() noexcept
        {
            return isValid() && ts_tree_cursor_goto_parent(&impl_);
        }

        /**
         * @brief Moves the cursor to the first child of its current node.
         * @return Returns true if the cursor successfully moved, false if the node has no children.
         */
        [[nodiscard]] bool gotoFirstChild() noexcept
        {
            return isValid() && ts_tree_cursor_goto_first_child(&impl_);
        }

        /**
         * @brief Moves the cursor to the last child of its current node.
         * @return Returns true if the cursor successfully moved, false if the node has no children.
         */
        [[nodiscard]] bool gotoLastChild() noexcept
        {
            return isValid() && ts_tree_cursor_goto_last_child(&impl_);
        }

        /**
         * @brief Moves the cursor to the next sibling of its current node.
         * @return Returns true if the cursor successfully moved, false if there is no next sibling.
         */
        [[nodiscard]] bool gotoNextSibling() noexcept
        {
            return isValid() && ts_tree_cursor_goto_next_sibling(&impl_);
        }

        /**
         * @brief Moves the cursor to the previous sibling of its current node.
         * @return Returns true if the cursor successfully moved, false if there is no previous sibling.
         */
        [[nodiscard]] bool gotoPreviousSibling() noexcept
        {
            return isValid() && ts_tree_cursor_goto_previous_sibling(&impl_);
        }

        /**
         * @brief Moves the cursor to the first child of the current node that extends beyond the given byte offset.
         * @param byte The byte offset to search for.
         * @return Returns the index of the child if found, or -1 if the cursor is invalid or no such child exists.
         */
        [[nodiscard]] std::int64_t gotoFirstChildForByte(std::uint32_t byte) noexcept
        {
            return isValid() ? ts_tree_cursor_goto_first_child_for_byte(&impl_, byte) : -1;
        }

        /**
         * @brief Moves the cursor to the first child of the current node that extends beyond the given point.
         * @param point The row and column coordinates to search for.
         * @return Returns the index of the child if found, or -1 if the cursor is invalid or no such child exists.
         */
        [[nodiscard]] std::int64_t gotoFirstChildForPoint(Point point) noexcept
        {
            return isValid() ? ts_tree_cursor_goto_first_child_for_point(&impl_, point) : -1;
        }

        /**
         * @brief Moves the cursor to a specific descendant based on its index.
         * @param descendant_index The internal index of the descendant to navigate to.
         */
        void gotoDescendant(std::uint32_t descendant_index) noexcept
        {
            if (!isValid())
            {
                return;
            }
            ts_tree_cursor_goto_descendant(&impl_, descendant_index);
        }

#pragma endregion

#pragma region CURRENT_NODE_ATTRIBUTES

        ////////////////////////////////////////////////////////////////
        // Current Node Attributes
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the node that the cursor is currently pointing at.
         * @return Returns the current syntax node, or a null node if the cursor is invalid.
         */
        [[nodiscard]] Node getCurrentNode() const noexcept
        {
            return isValid() ? Node{ ts_tree_cursor_current_node(&impl_) } : Node::null();
        }

        /**
         * @brief Retrieves the field name of the current node, if any.
         * @return Returns a view to the field name, or an empty view if there is no field name.
         */
        [[nodiscard]] details::StringViewReturn getCurrentFieldName() const
        {
            return isValid() ? details::make_view(ts_tree_cursor_current_field_name(&impl_))
                             : details::StringViewReturn("");
        }

        /**
         * @brief Retrieves the field ID of the current node, if any.
         * @return Returns the field ID, or 0 if there is no field ID or if the cursor is invalid.
         */
        [[nodiscard]] FieldID getCurrentFieldID() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_field_id(&impl_) : 0;
        }

        /**
         * @brief Retrieves the internal index of the current descendant.
         * @return Returns the index of the descendant.
         */
        [[nodiscard]] std::uint32_t getCurrentDescendantIndex() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_descendant_index(&impl_) : 0;
        }

        /**
         * @brief Retrieves the depth of the current node relative to the origin node.
         * @return Returns the depth level from the origin.
         */
        [[nodiscard]] std::uint32_t getDepthFromOrigin() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_depth(&impl_) : 0;
        }

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Compares two tree cursors to check if they point to the same node.
         * @param other The tree cursor to compare against.
         * @return Returns true if both cursors are invalid, or if both point to the exact same node.
         */
        [[nodiscard]] bool operator==(const TreeCursor &other) const noexcept;

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Compares two tree cursors for inequality.
         * @param other The tree cursor to compare against.
         * @return Returns true if the cursors point to different nodes.
         */
        [[nodiscard]] bool operator!=(const TreeCursor &other) const noexcept
        {
            return !(*this == other);
        }
#endif // !CPP_TS_HAS_CXX20

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        TSTreeCursor impl_{};
        bool         is_valid_ = false;

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_CAPTURE

    /////////////////////////////////////////////////////////////////////////////
    // QueryCapture
    // A capture of a specific node by a specific name within a query pattern.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A capture of a specific node by a specific name within a query pattern.
    struct QueryCapture
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The syntax node that was captured.
        Node node;

        /// @brief The index of the capture in the pattern.
        std::uint32_t index;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty query capture with a null node.
        constexpr QueryCapture() noexcept : node(Node::null()), index(0)
        {}

        /**
         * @brief Constructs a query capture from a native C API object.
         * @param capture capture The native `TSQueryCapture` object to convert.
         */
        constexpr QueryCapture(const TSQueryCapture &capture) noexcept : node(capture.node), index(capture.index)
        {}

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_MATCH

    /////////////////////////////////////////////////////////////////////////////
    // QueryMatch
    // A single match found by a query, containing one or more captures.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A single match found by a query, containing one or more captures.
    struct QueryMatch
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The unique identifier for this match.
        std::uint32_t id;

        /// @brief The index of the matched pattern within the query.
        std::uint16_t pattern_index;

        /**
         * @brief A collection of captures associated with this match.
         * @details A vector is used for easy access, however note that data copying occurs only when the `QueryMatch`
         * object is instantiated.
         */
        std::vector<QueryCapture> captures;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty query match.
        QueryMatch() noexcept : id(0), pattern_index(0), captures()
        {}

        /**
         * @brief Constructs a query match from a native C API object.
         * @param match The native `TSQueryMatch` object to convert.
         */
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

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_CURSOR_STATE

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursorState
    // The current state of an active query cursor.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief The current state of an active query cursor.
    struct QueryCursorState
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The current byte offset being processed by the cursor.
        std::uint32_t current_byte_offset;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Constructs a cursor state from a pointer to a native C API object.
         * @param state A pointer to the native `TSQueryCursorState` object.
         */
        constexpr explicit QueryCursorState(const TSQueryCursorState *state) noexcept
            : current_byte_offset(state->current_byte_offset)
        {}

        /**
         * @brief Constructs a cursor state from a native C API reference.
         * @param state A reference to the native `TSQueryCursorState` object.
         */
        constexpr explicit QueryCursorState(const TSQueryCursorState &state) noexcept
            : current_byte_offset(state.current_byte_offset)
        {}

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_CURSOR_OPTIONS

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursorOptions
    // Configuration options for a query cursor, such as progress callbacks.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Configuration options for a query cursor, such as progress callbacks.
    using QueryCursorOptions = details::OptionsBase<TSQueryCursorOptions, TSQueryCursorState, QueryCursorState>;

#pragma endregion

#pragma region QUERY_PREDICATE_STEP

    /////////////////////////////////////////////////////////////////////////////
    // QueryPredicateStep
    // A single step in a predicate defined within a query pattern.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A single step in a predicate defined within a query pattern.
    struct QueryPredicateStep
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The identifier of the string value associated with this predicate step.
        std::uint32_t value_id;

        /// @brief The type of this predicate step.
        QueryPredicateStepType type;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Constructs a predicate step from a pointer to a native C API object.
         * @param predicate_step A pointer to the native `TSQueryPredicateStep` object.
         */
        constexpr QueryPredicateStep(const TSQueryPredicateStep *predicate_step) noexcept
            : value_id(predicate_step->value_id), type(static_cast<QueryPredicateStepType>(predicate_step->type))
        {}

        /**
         * @brief Constructs a predicate step from a native C API reference.
         * @param predicate_step A reference to the native `TSQueryPredicateStep` object.
         */
        constexpr QueryPredicateStep(const TSQueryPredicateStep &predicate_step) noexcept
            : value_id(predicate_step.value_id), type(static_cast<QueryPredicateStepType>(predicate_step.type))
        {}

#pragma endregion
    };

#pragma endregion

#pragma region QUERY

    /////////////////////////////////////////////////////////////////////////////
    // Query
    // Compiled set of patterns used to search for structures in a syntax tree.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A compiled set of patterns used to search for structures in a syntax tree.
    class Query
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Constructs a query object from a string of source code.
         * @param language The language used to interpret the query.
         * @param source The query source code.
         * @throws std::runtime_error If the language is invalid or if query compilation fails.
         * @throws std::length_error If the source code exceeds the maximum size of 4GB.
         */
        Query(LanguageView language, details::StringViewParameter source);

#if CPP_TS_HAS_CXX23
        /**
         * @brief Creates a query object without throwing exceptions.
         * @param language The language used to interpret the query.
         * @param source The query source code.
         * @return Returns a valid `Query` instance, or an error string detailing why the compilation failed.
         */
        [[nodiscard]] static std::expected<Query, std::string> create(LanguageView                 language,
                                                                      details::StringViewParameter source) noexcept;
#endif // CPP_TS_HAS_CXX23

#pragma endregion

#pragma region COUNTS

        ////////////////////////////////////////////////////////////////
        // Counts
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the total number of patterns within this query.
         * @return Returns the number of patterns.
         */
        [[nodiscard]] std::uint32_t getPatternCount() const noexcept
        {
            return ts_query_pattern_count(impl_.get());
        }

        /**
         * @brief Retrieves the total number of captures within this query.
         * @return Returns the number of captures.
         */
        [[nodiscard]] std::uint32_t getCaptureCount() const noexcept
        {
            return ts_query_capture_count(impl_.get());
        }

        /**
         * @brief Retrieves the total number of string literals defined within this query.
         * @return Returns the number of string literals.
         */
        [[nodiscard]] std::uint32_t getStringCount() const noexcept
        {
            return ts_query_string_count(impl_.get());
        }

#pragma endregion

#pragma region PATTERN_INFORMATION

        ////////////////////////////////////////////////////////////////
        // Pattern Information
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks if a specific pattern is rooted.
         * @param pattern_index The index of the pattern to check.
         * @return Returns true if the pattern is rooted, false otherwise.
         */
        [[nodiscard]] bool isPatternRooted(std::uint32_t pattern_index) const noexcept
        {
            return ts_query_is_pattern_rooted(impl_.get(), pattern_index);
        }

        /**
         * @brief Checks if a specific pattern is non-local.
         * @param pattern_index The index of the pattern to check.
         * @return Returns true if the pattern is non-local, false otherwise.
         */
        [[nodiscard]] bool isPatternNonLocal(std::uint32_t pattern_index) const noexcept
        {
            return ts_query_is_pattern_non_local(impl_.get(), pattern_index);
        }

        /**
         * @brief Checks if a pattern is guaranteed to match at a specific byte offset.
         * @param byte_offset The byte offset to check.
         * @return Returns true if the pattern match is guaranteed.
         */
        [[nodiscard]] bool isPatternGuaranteedAtStep(std::uint32_t byte_offset) const noexcept
        {
            return ts_query_is_pattern_guaranteed_at_step(impl_.get(), byte_offset);
        }

        /**
         * @brief Retrieves the byte range representing where a pattern is defined in the source.
         * @param pattern_index The index of the pattern.
         * @return Returns an extent containing the start and end byte offsets.
         */
        [[nodiscard]] Extent<std::uint32_t> getByteRangeForPattern(std::uint32_t pattern_index) const noexcept
        {
            return { ts_query_start_byte_for_pattern(impl_.get(), pattern_index),
                     ts_query_end_byte_for_pattern(impl_.get(), pattern_index) };
        }

#pragma endregion

#pragma region PREDICATES

        ////////////////////////////////////////////////////////////////
        // Predicates
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves all predicate steps defined for a specific pattern.
         * @param pattern_index The index of the pattern.
         * @return Returns a vector containing all predicate steps for the pattern.
         */
        [[nodiscard]] std::vector<QueryPredicateStep> getAllPredicatesForPattern(std::uint32_t pattern_index) const;

#pragma endregion

#pragma region CAPTURE_AND_STRING_RESOLUTION

        ////////////////////////////////////////////////////////////////
        // Capture & String Resolution
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the name of a capture using its internal ID.
         * @param id The ID of the capture.
         * @return Returns a string view containing the capture name.
         */
        [[nodiscard]] details::StringViewReturn getCaptureNameForID(std::uint32_t id) const noexcept
        {
            std::uint32_t length;
            const char   *name = ts_query_capture_name_for_id(impl_.get(), id, &length);
            return details::make_view(name, static_cast<std::size_t>(length));
        }

        /**
         * @brief Retrieves the quantifier associated with a specific capture within a pattern.
         * @param pattern_index The index of the pattern containing the capture.
         * @param capture_index The index of the capture.
         * @return Returns the quantifier corresponding to the capture.
         */
        [[nodiscard]] Quantifier getCaptureQuantifierForID(std::uint32_t pattern_index,
                                                           std::uint32_t capture_index) const
        {
            return static_cast<Quantifier>(
                    ts_query_capture_quantifier_for_id(impl_.get(), pattern_index, capture_index));
        }

        /**
         * @brief Retrieves the literal string value associated with a specific internal ID.
         * @param id The ID of the string.
         * @return Returns a string view of the string value.
         */
        [[nodiscard]] details::StringViewReturn getStringValueForID(std::uint32_t id) const noexcept
        {
            std::uint32_t length;
            const char   *name = ts_query_string_value_for_id(impl_.get(), id, &length);
            return details::make_view(name, static_cast<std::size_t>(length));
        }

#pragma endregion

#pragma region MODIFICATION

        ////////////////////////////////////////////////////////////////
        // Modification
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Disables a specific capture by name, preventing it from being returned in matches.
         * @param name The name of the capture to disable.
         * @throws std::length_error If the capture name exceeds the maximum size of 4GB.
         */
        void disableCapture(details::StringViewParameter name);

        /**
         * @brief Disables a specific pattern, preventing it from being matched by cursors.
         * @param pattern_index The index of the pattern to disable.
         */
        void disablePattern(std::uint32_t pattern_index) noexcept
        {
            ts_query_disable_pattern(impl_.get(), pattern_index);
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Implicit conversion to the underlying C API structure pointer.
         * @return Returns a pointer to the internal `TSQuery`.
         */
        [[nodiscard]] operator const TSQuery *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Private Lifecycle
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX23
        explicit Query(TSQuery *raw_query) : impl_(raw_query, ts_query_delete)
        {}
#endif // CPP_TS_HAS_CXX23

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<TSQuery, decltype(&ts_query_delete)> impl_{ nullptr, ts_query_delete };

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_CURSOR

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursor
    // Executes queries and iterates over matches.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Executes queries and iterates over matches within a syntax tree.
    class QueryCursor
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs a new empty query cursor.
        QueryCursor() noexcept : impl_{ ts_query_cursor_new() }
        {}

        /**
         * @brief Deleted copy constructor.
         * @param other Another query cursor.
         *
         * @note Copying is not supported by the underlying C API (`TSQueryCursor` cannot be safely duplicated).
         */
        QueryCursor(const QueryCursor &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another query cursor.
         * @return Returns a reference to this query cursor.
         *
         * @note Copying is not supported by the underlying C API.
         */
        QueryCursor &operator=(const QueryCursor &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another query cursor to move from.
         */
        QueryCursor(QueryCursor &&other) noexcept = default;

        /**
         * @brief Move assignment operator.
         * @param other Another query cursor to move from.
         * @return Returns a reference to this query cursor.
         */
        QueryCursor &operator=(QueryCursor &&other) noexcept = default;

#pragma endregion

#pragma region LIMITS

        ////////////////////////////////////////////////////////////////
        // Limits
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the maximum number of in-progress matches for this cursor.
         * @return Returns the current match limit.
         */
        [[nodiscard]] std::uint32_t getMatchLimit() noexcept
        {
            return ts_query_cursor_match_limit(impl_.get());
        }

        /**
         * @brief Sets the maximum number of in-progress matches for this cursor.
         * @param limit The maximum number of matches.
         */
        void setMatchLimit(std::uint32_t limit) noexcept
        {
            ts_query_cursor_set_match_limit(impl_.get(), limit);
        }

        /**
         * @brief Checks if the cursor exceeded its match limit during the last execution.
         * @return Returns true if the match limit was exceeded, false otherwise.
         */
        [[nodiscard]] bool didExceedMatchLimit() noexcept
        {
            return ts_query_cursor_did_exceed_match_limit(impl_.get());
        }

        /**
         * @brief Sets the maximum start depth for the query cursor.
         * @param max_start_depth The maximum depth from the root node where a match can start.
         */
        void setMaxStartDepth(std::uint32_t max_start_depth) noexcept
        {
            ts_query_cursor_set_max_start_depth(impl_.get(), max_start_depth);
        }

        /// @brief Resets the maximum start depth to its default value (no limit).
        void resetMaxStartDepth() noexcept
        {
            ts_query_cursor_set_max_start_depth(impl_.get(), std::numeric_limits<std::uint32_t>::max());
        }

#pragma endregion

#pragma region RANGES

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Restricts the query cursor to a specific byte range.
         * @param range The start and end byte offsets to restrict parsing to.
         * @return Returns true if the range was successfully set.
         */
        [[nodiscard]] bool setByteRange(Extent<std::uint32_t> range) noexcept
        {
            return ts_query_cursor_set_byte_range(impl_.get(), range.start, range.end);
        }

        /**
         * @brief Restricts the query cursor to a specific point range (row and column).
         * @param range The start and end points to restrict parsing to.
         * @return Returns true if the range was successfully set.
         */
        [[nodiscard]] bool setPointRange(Extent<Point> range) noexcept
        {
            return ts_query_cursor_set_point_range(impl_.get(), range.start, range.end);
        }

        /**
         * @brief Restricts the query cursor to matches that intersect a specific byte range.
         * @param range The containing byte range.
         * @return Returns true if the range was successfully set.
         */
        [[nodiscard]] bool setContainingByteRange(Extent<std::uint32_t> range) noexcept
        {
            return ts_query_cursor_set_containing_byte_range(impl_.get(), range.start, range.end);
        }

        /**
         * @brief Restricts the query cursor to matches that intersect a specific point range.
         * @param range The containing point range.
         * @return Returns true if the range was successfully set.
         */
        [[nodiscard]] bool setContainingPointRange(Extent<Point> range) noexcept
        {
            return ts_query_cursor_set_containing_point_range(impl_.get(), range.start, range.end);
        }

#pragma endregion

#pragma region EXECUTION

        ////////////////////////////////////////////////////////////////
        // Execution
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Starts executing a query on a specific syntax node.
         * @param query The compiled query to execute.
         * @param node The syntax tree node to begin the search from.
         * @param query_options Optional advanced configuration (such as progress callbacks) to use during execution.
         */
        void exec(const Query &query, Node node, details::OptionalParam<const QueryCursorOptions> query_options = {});

#pragma endregion

#pragma region ITERATION

        ////////////////////////////////////////////////////////////////
        // Iteration
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Advances to the next match of the currently executing query.
         * @param match A reference to a `QueryMatch` object that will be populated if a match is found.
         * @return Returns true if a new match was found, false if there are no more matches.
         */
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

        /**
         * @brief Advances to the next individual capture of the currently executing query.
         * @param match A reference to a `QueryMatch` object that will be populated.
         * @param capture_index A reference to a variable that will receive the index of the specific capture within the
         * match.
         * @return Returns true if a new capture was found, false if there are no more captures.
         */
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

        /**
         * @brief Removes a currently executing match from the cursor.
         * @param match_id The ID of the match to remove.
         */
        void removeMatch(std::uint32_t match_id) noexcept
        {
            return ts_query_cursor_remove_match(impl_.get(), match_id);
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<TSQueryCursor, details::FreeHelper> impl_{ nullptr };

#pragma endregion
    };

#pragma endregion

#pragma region LOOKAHEAD_ITERATOR

    /////////////////////////////////////////////////////////////////////////////
    // LookaheadIterator
    // Iterates over possible symbols in a given parse state.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Iterates over possible symbols in a given parse state.
    class LookaheadIterator
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Constructs a lookahead iterator for a specific language and parse state.
         * @param language The language to be used by the iterator.
         * @param state The parse state to iterate over.
         * @throws std::runtime_error If the language is invalid or the parse state is invalid (fails to create the
         * underlying `TSLookaheadIterator`).
         */
        LookaheadIterator(LanguageView language, StateID state);

        /**
         * @brief Deleted copy constructor.
         * @param other Another lookahead iterator.
         *
         * @note Copying is not supported by the underlying C API.
         */
        LookaheadIterator(const LookaheadIterator &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another lookahead iterator.
         * @return Returns a reference to this iterator.
         *
         * @note Copying is not supported by the underlying C API.
         */
        LookaheadIterator &operator=(const LookaheadIterator &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another lookahead iterator to move from.
         */
        LookaheadIterator(LookaheadIterator &&other) noexcept = default;

        /**
         * @brief Move assignment operator.
         * @param other Another lookahead iterator to move from.
         * @return Returns a reference to this iterator.
         */
        LookaheadIterator &operator=(LookaheadIterator &&other) noexcept = default;

#pragma endregion

#pragma region STATE_CONTROL

        ////////////////////////////////////////////////////////////////
        // State Control
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Resets the iterator to a new parse state.
         * @param state The new parse state to iterate over.
         * @return Returns true if the state was successfully reset, false otherwise.
         */
        [[nodiscard]] bool resetState(StateID state) noexcept
        {
            return ts_lookahead_iterator_reset_state(impl_.get(), state);
        }

        /**
         * @brief Resets the iterator to a new language and parse state.
         * @param language The new language to use.
         * @param state The new parse state to iterate over.
         * @return Returns true if the language and state were successfully reset, false otherwise.
         */
        [[nodiscard]] bool reset(LanguageView language, StateID state) noexcept
        {
            return ts_lookahead_iterator_reset(impl_.get(), language, state);
        }

#pragma endregion

#pragma region PROPERTY_ACCESSORS

        ////////////////////////////////////////////////////////////////
        // Property Accessors
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the language currently associated with the iterator.
         * @return Returns a view of the current language.
         */
        [[nodiscard]] LanguageView getLanguage() const noexcept
        {
            return LanguageView(ts_lookahead_iterator_language(impl_.get()));
        }

        /**
         * @brief Retrieves the symbol the iterator is currently pointing at.
         * @return Returns the current symbol.
         */
        [[nodiscard]] Symbol getCurrentSymbol() const noexcept
        {
            return ts_lookahead_iterator_current_symbol(impl_.get());
        }

        /**
         * @brief Retrieves the name of the current symbol.
         * @return Returns a string view of the symbol's name.
         */
        [[nodiscard]] details::StringViewReturn getCurrentSymbolName() const
        {
            return details::make_view(ts_lookahead_iterator_current_symbol_name(impl_.get()));
        }

#pragma endregion

#pragma region NAVIGATION

        ////////////////////////////////////////////////////////////////
        // Navigation
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Advances the iterator to the next possible symbol.
         * @return Returns true if the iterator successfully advanced to a new symbol, false if there are no more
         * symbols.
         */
        [[nodiscard]] bool next() noexcept
        {
            return ts_lookahead_iterator_next(impl_.get());
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Implicit conversion to the underlying C API structure pointer.
         * @return Returns a pointer to the internal `TSLookaheadIterator`.
         */
        [[nodiscard]] operator const TSLookaheadIterator *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<TSLookaheadIterator, details::FreeHelper> impl_{ nullptr };

#pragma endregion
    };

#pragma endregion

#pragma region WASM_STORE

#if defined(CPP_TS_FEATURE_WASM)
    /////////////////////////////////////////////////////////////////////////////
    // WasmStore
    // Manages WebAssembly environments and loading Wasm-based languages.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Manages WebAssembly environments and loading Wasm-based languages.
    class WasmStore
    {
    public:
#pragma region LIFECYCLE

        //////////////////////////////////////////////////////////////
        // Lifecycle
        //////////////////////////////////////////////////////////////

        /**
         * @brief Constructs a WebAssembly store using the provided engine.
         * @param engine A pointer to the WebAssembly engine.
         * @throws std::runtime_error If the creation of the underlying `TSWasmStore` fails.
         */
        explicit WasmStore(WasmEngine *engine);

        /**
         * @brief Constructs a WebAssembly store from a native C API pointer.
         * @param store A pointer to a native `TSWasmStore`.
         */
        explicit WasmStore(TSWasmStore *store) noexcept
            : impl_{ store, ts_wasm_store_delete }, is_valid_(store != nullptr)
        {}

        /**
         * @brief Deleted copy constructor.
         * @param other Another WebAssembly store.
         *
         * @note Copying is not supported by the underlying C API.
         */
        WasmStore(const WasmStore &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another WebAssembly store.
         * @return Returns a reference to this WebAssembly store.
         *
         * @note Copying is not supported by the underlying C API.
         */
        WasmStore &operator=(const WasmStore &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another WebAssembly store to move from.
         */
        WasmStore(WasmStore &&other) noexcept;

        /**
         * @brief Move assignment operator.
         * @param other Another WebAssembly store to move from.
         * @return Returns a reference to this WebAssembly store.
         */
        WasmStore &operator=(WasmStore &&other) noexcept;

#if CPP_TS_HAS_CXX23
        /**
         * @brief Creates a WebAssembly store without throwing exceptions.
         * @param engine A pointer to the WebAssembly engine.
         * @return Returns a valid `WasmStore` instance on success, or an error string on failure.
         */
        [[nodiscard]] static std::expected<WasmStore, std::string> create(WasmEngine *engine) noexcept;
#endif // CPP_TS_HAS_CXX23

#pragma endregion

#pragma region LANGUAGE

        //////////////////////////////////////////////////////////////
        // Language
        //////////////////////////////////////////////////////////////

        /**
         * @brief Loads a language from a WebAssembly buffer.
         * @param name The name of the language.
         * @param wasm_buffer The buffer containing the compiled WebAssembly grammar.
         * @return Returns a loaded `Language` object.
         * @throws std::logic_error If the store is currently invalid.
         * @throws std::length_error If the buffer size exceeds 4GB.
         * @throws std::runtime_error If the language fails to load.
         */
        [[nodiscard]] Language loadLanguage(details::StringViewParameter name,
                                            details::StringViewParameter wasm_buffer);

        /**
         * @brief Loads a language from a WebAssembly file on disk.
         * @param file_path The path to the WebAssembly file.
         * @return Returns a loaded `Language` object.
         * @throws std::logic_error If the store is currently invalid.
         * @throws std::length_error If the file path exceeds 4GB.
         * @throws std::runtime_error If the file cannot be read or the language fails to load.
         */
        [[nodiscard]] Language loadLanguage(details::StringViewParameter file_path);

#if CPP_TS_HAS_CXX23
        /**
         * @brief Loads a language from a WebAssembly buffer without throwing exceptions.
         * @param name The name of the language.
         * @param wasm_buffer The buffer containing the compiled WebAssembly grammar.
         * @return Returns a loaded `Language` object on success, or an error string on failure.
         */
        [[nodiscard]] std::expected<Language, std::string> loadLanguageExpected(
                details::StringViewParameter name,
                details::StringViewParameter wasm_buffer) noexcept;

        /**
         * @brief Loads a language from a WebAssembly file on disk without throwing exceptions.
         * @param file_path The path to the WebAssembly file.
         * @return Returns a loaded `Language` object on success, or an error string on failure.
         */
        [[nodiscard]] std::expected<Language, std::string> loadLanguageExpected(details::StringViewParameter file_path);
#endif // CPP_TS_HAS_CXX23

        /**
         * @brief Retrieves the number of languages currently loaded into this store.
         * @return Returns the total count of loaded languages.
         * @throws std::logic_error If the store is currently invalid.
         */
        [[nodiscard]] std::size_t getLanguageCount() const;

#pragma endregion

#pragma region FLAGS

        //////////////////////////////////////////////////////////////
        // Flags
        //////////////////////////////////////////////////////////////

        /**
         * @brief Checks if the WebAssembly store is valid and ready to use.
         * @return Returns true if the store is valid, false otherwise.
         */
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return is_valid_;
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Implicit conversion to the underlying C API structure pointer.
         * @return Returns a pointer to the internal `TSWasmStore`.
         */
        [[nodiscard]] operator TSWasmStore *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        void setInvalid() noexcept
        {
            // We need to set impl as nullptr but don't call deleter on stored pointer.
            impl_.release();
            is_valid_ = false;
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        std::unique_ptr<TSWasmStore, decltype(&ts_wasm_store_delete)> impl_{ nullptr, ts_wasm_store_delete };
        bool                                                          is_valid_ = false;

#pragma endregion

        friend class Parser;
    };
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

#pragma region CHILD_NODE_ITERATORS

    /////////////////////////////////////////////////////////////////////////////
    // Child Node Iterators
    // STL-compatible iterators for processing Node children.
    /////////////////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
    /// @brief A sentinel type used to indicate the end of a child node iteration.
    class ChildIteratorSentinel
    {};
#endif // CPP_TS_HAS_CXX17

    /// @brief An STL-compatible input iterator for traversing the immediate children of a syntax node.
    class ChildIterator
    {
    public:
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        using value_type        = Node;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

#pragma endregion

#pragma region INNER_CLASSES

        ////////////////////////////////////////////////////////////////
        // Inner Classes
        ////////////////////////////////////////////////////////////////

        /// @brief A proxy object representing a child node during iteration.
        class Child
        {
        public:
            /**
             * @brief Constructs a child proxy object.
             * @param v The syntax node to wrap.
             */
            explicit Child(value_type v) noexcept : value_(v)
            {}

#if CPP_TS_HAS_CXX23
            /**
             * @brief Accesses the underlying syntax node.
             * @tparam Self The deduced type of the calling object.
             * @param self The calling object instance.
             * @return Returns a pointer to the syntax node.
             */
            template <typename Self>
            [[nodiscard]] auto *operator->(this Self &&self) noexcept
            {
                return &self.value_;
            }
#else
            /**
             * @brief Accesses the underlying syntax node (const).
             * @return Returns a const pointer to the syntax node.
             */
            [[nodiscard]] const value_type *operator->() const noexcept
            {
                return &value_;
            }

            /**
             * @brief Accesses the underlying syntax node.
             * @return Returns a pointer to the syntax node.
             */
            [[nodiscard]] value_type *operator->() noexcept
            {
                return &value_;
            }
#endif // CPP_TS_HAS_CXX23

        private:
            value_type value_;
        };

        /// @brief A proxy object used to support postfix increment and decrement operators.
        class PostfixProxy
        {
        public:
            /**
             * @brief Constructs a postfix proxy object.
             * @param v The syntax node to preserve.
             */
            explicit PostfixProxy(value_type v) noexcept : value_(v)
            {}

            /**
             * @brief Dereferences the proxy to get the underlying node.
             * @return Returns the preserved syntax node.
             */
            [[nodiscard]] value_type operator*() const noexcept
            {
                return value_;
            }

            /**
             * @brief Accesses the underlying node via a child proxy.
             * @return Returns a proxy object to the preserved syntax node.
             */
            Child operator->() const noexcept
            {
                return Child(value_);
            }

        private:
            value_type value_;
        };

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty iterator pointing to the end.
        ChildIterator() : atEnd_(true)
        {}

        /**
         * @brief Constructs an iterator for traversing the children of a specific node.
         * @param node The parent node whose children will be iterated.
         */
        explicit ChildIterator(const Node &node) noexcept
            : cursor_(node.getCursor()), atEnd_(node.isNull() || !cursor_.gotoFirstChild())
        {}

        /**
         * @brief Deleted copy constructor.
         * @param other Another child iterator.
         *
         * @note Copying is not supported by the underlying C API.
         */
        ChildIterator(const ChildIterator &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another child iterator.
         * @return Returns a reference to this iterator.
         *
         * @note Copying is not supported by the underlying C API.
         */
        ChildIterator &operator=(const ChildIterator &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another child iterator to move from.
         */
        ChildIterator(ChildIterator &&other) noexcept = default;

        /**
         * @brief Move assignment operator.
         * @param other Another child iterator to move from.
         * @return Returns a reference to this iterator.
         */
        ChildIterator &operator=(ChildIterator &&other) noexcept = default;

#pragma endregion

#pragma region GET

        ////////////////////////////////////////////////////////////////
        // Get
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the current child node the iterator is pointing to.
         * @return Returns the current syntax node.
         */
        [[nodiscard]] value_type get() const noexcept
        {
            return cursor_.getCurrentNode();
        }

        /**
         * @brief Dereferences the iterator to get the current child node.
         * @return Returns the current syntax node.
         */
        [[nodiscard]] value_type operator*() const noexcept
        {
            return get();
        }

        /**
         * @brief Accesses the current child node.
         * @return Returns a proxy object to the current syntax node.
         */
        Child operator->() const noexcept
        {
            // Dereference this iterator to get the value,
            // then wrap it in the safe proxy.
            return Child(get());
        }

#pragma endregion

#pragma region ADVANCE

        ////////////////////////////////////////////////////////////////
        // Advance
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Advances the iterator to the next sibling (prefix).
         * @return Returns a reference to this updated iterator.
         */
        ChildIterator &operator++() noexcept
        {
            if (!atEnd_)
            {
                atEnd_ = !cursor_.gotoNextSibling();
            }
            return *this;
        }

        /**
         * @brief Advances the iterator to the next sibling (postfix).
         * @return Returns a proxy to the state of the iterator before advancement.
         */
        PostfixProxy operator++(int) noexcept
        {
            PostfixProxy proxy(get());
            ++(*this);
            return proxy;
        }

#pragma endregion

#pragma region RETREAT

        ////////////////////////////////////////////////////////////////
        // Retreat
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Moves the iterator to the previous sibling (prefix).
         * @return Returns a reference to this updated iterator.
         */
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

        /**
         * @brief Moves the iterator to the previous sibling (postfix).
         * @return Returns a proxy to the state of the iterator before moving back.
         */
        PostfixProxy operator--(int) noexcept
        {
            PostfixProxy proxy(get());
            --(*this);
            return proxy;
        }

#pragma endregion

#pragma region SENTINELS_COMPARISION

        ////////////////////////////////////////////////////////////////
        // Sentinels Comparision
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
        /**
         * @brief Checks if the iterator has reached the end sentinel.
         * @param a The child iterator.
         * @return Returns true if the iterator is at the end.
         */
        [[nodiscard]] friend bool operator==(const ChildIterator &a, const ChildIteratorSentinel &) noexcept
        {
            return a.atEnd_;
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Checks if the iterator has not reached the end sentinel.
         * @param a The child iterator.
         * @param b The end sentinel.
         * @return Returns true if the iterator is not at the end.
         */
        [[nodiscard]] friend bool operator!=(const ChildIterator &a, const ChildIteratorSentinel &b) noexcept
        {
            return !(a == b);
        }
#endif // !CPP_TS_HAS_CXX20

        /**
         * @brief Checks if the sentinel has been reached by the iterator.
         * @param a The end sentinel.
         * @param b The child iterator.
         * @return Returns true if the iterator is at the end.
         */
        [[nodiscard]] friend bool operator==(const ChildIteratorSentinel &a, const ChildIterator &b) noexcept
        {
            return b == a;
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Checks if the sentinel has not been reached by the iterator.
         * @param a The end sentinel.
         * @param b The child iterator.
         * @return Returns true if the iterator is not at the end.
         */
        [[nodiscard]] friend bool operator!=(const ChildIteratorSentinel &a, const ChildIterator &b) noexcept
        {
            return b != a;
        }
#endif // !CPP_TS_HAS_CXX20
#endif // CPP_TS_HAS_CXX17

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Compares two child iterators for equality.
         * @param a The first iterator.
         * @param b The second iterator.
         * @return Returns true if both iterators point to the same child or are both at the end.
         */
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
        /**
         * @brief Compares two child iterators for inequality.
         * @param a The first iterator.
         * @param b The second iterator.
         * @return Returns true if the iterators point to different children.
         */
        [[nodiscard]] friend bool operator!=(const ChildIterator &a, const ChildIterator &b) noexcept
        {
            return !(a == b);
        }
#endif // !CPP_TS_HAS_CXX20

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        TreeCursor cursor_;
        bool       atEnd_ = false;

#pragma endregion
    };

#if CPP_TS_HAS_CXX20
    /// @brief A view representing a collection of child nodes, suitable for range-based for loops.
    struct Children : public std::ranges::view_interface<Children>
#else
    /// @brief A view representing a collection of child nodes, suitable for range-based for loops.
    struct Children
#endif // CPP_TS_HAS_CXX20
    {
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        using iterator = ChildIterator;
#if CPP_TS_HAS_CXX17
        using sentinel = ChildIteratorSentinel;
#else
        using sentinel = ChildIterator;
#endif // CPP_TS_HAS_CXX17

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The parent node whose children are being viewed.
        Node node_ref;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Constructs a children view for a given parent node.
         * @param n The parent node.
         */
        explicit Children(const Node &n) noexcept : node_ref(n)
        {}

#pragma endregion

#pragma region METHODS

        ////////////////////////////////////////////////////////////////
        // Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns an iterator pointing to the first child.
         * @return Returns a child iterator.
         */
        [[nodiscard]] iterator begin() const noexcept
        {
            return iterator(node_ref);
        }

        /**
         * @brief Returns a sentinel indicating the end of the children sequence.
         * @return Returns the end sentinel (or an end iterator for older C++ standards).
         */
        [[nodiscard]] sentinel end() const noexcept
        {
            return sentinel();
        }

        /**
         * @brief Retrieves the total number of children associated with the parent node.
         * @return Returns the child count.
         */
        [[nodiscard]] std::size_t size() const noexcept
        {
            return static_cast<std::size_t>(node_ref.getChildCount());
        }

#pragma endregion
    };

#if CPP_TS_HAS_CXX20
    static_assert(std::input_iterator<ChildIterator>);
    static_assert(std::sentinel_for<ChildIteratorSentinel, ChildIterator>);
#endif // CPP_TS_HAS_CXX20

#pragma endregion

#pragma region VISITOR

    /////////////////////////////////////////////////////////////////////////////
    // Visitor
    // High-level utility for depth-first traversal of a syntax tree.
    /////////////////////////////////////////////////////////////////////////////

#pragma region CONCEPTS

    ////////////////////////////////////////////////////////////////
    // Concepts
    ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX20
    /**
     * @brief A concept ensuring a callable object accepts a syntax node and returns a boolean value.
     * @tparam T The type of the callable visitor.
     */
    template <typename T>
    concept VisitorConcept = requires {
        { std::declval<T>()(std::declval<Node>()) } -> std::convertible_to<bool>;
    };
#elif CPP_TS_HAS_CXX17
    /**
     * @brief A type trait ensuring a callable object accepts a syntax node and returns a boolean value.
     * @tparam T The type of the callable visitor.
     */
    template <typename T, typename = void>
    struct VisitorConcept : std::false_type
    {};

    /**
     * @brief A type trait ensuring a callable object accepts a syntax node and returns a boolean value.
     * @tparam T The type of the callable visitor.
     */
    template <typename T>
    struct VisitorConcept<T,
                          std::void_t<std::enable_if_t<
                                  std::is_convertible_v<decltype(std::declval<T>()(std::declval<Node>())), bool>>>>
        : std::true_type
    {};
#endif // CPP_TS_HAS_CXX20 / CPP_TS_HAS_CXX17

#pragma endregion

#pragma region FUNCTION

    ////////////////////////////////////////////////////////////////
    // Function
    ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX20
    /**
     * @brief Performs a depth-first traversal of the syntax tree starting from a given root node.
     * @tparam F The type of the visitor callback.
     * @param root The starting node for the traversal.
     * @param callback The function to call for each node.
     *
     * @note The callback should return true if it wants to halt the traversal, or false to continue.
     */
    template <VisitorConcept F>
#elif CPP_TS_HAS_CXX17
    /**
     * @brief Performs a depth-first traversal of the syntax tree starting from a given root node.
     * @tparam F The type of the visitor callback.
     * @param root The starting node for the traversal.
     * @param callback The function to call for each node.
     *
     * @note The callback should return true if it wants to halt the traversal, or false to continue.
     */
    template <typename F, std::enable_if_t<VisitorConcept<F>::value, bool> = true>
#else
    /**
     * @brief Performs a depth-first traversal of the syntax tree starting from a given root node.
     * @tparam F The type of the visitor callback.
     * @param root The starting node for the traversal.
     * @param callback The function to call for each node.
     *
     * @note The callback should return true if it wants to halt the traversal, or false to continue.
     */
    template <typename F>
#endif // CPP_TS_HAS_CXX20 / CPP_TS_HAS_CXX17
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

#pragma endregion

#pragma endregion

} // namespace ts

#if defined(CPP_TS_HEADER_ONLY)
#include "cpp-tree-sitter.inl"
#endif // defined(CPP_TS_HEADER_ONLY)

#endif // CPP_TREE_SITTER_HPP
