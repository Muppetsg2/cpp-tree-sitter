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

#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#endif // __has_include(<version>)
#endif // defined(__has_include)

#if defined(CPP_TS_HEADER_ONLY)
#define CPP_TS_API inline
#define CPP_TS_STD_LIMIT 99
#else
#define CPP_TS_API
#if defined(CPP_TS_CXX_STANDARD)
#define CPP_TS_STD_LIMIT CPP_TS_CXX_STANDARD
#else
#define CPP_TS_STD_LIMIT 11
#endif // defined(CPP_TS_CXX_STANDARD)
#endif // defined(CPP_TS_HEADER_ONLY)

#define CPP_TS_HAS_CXX14 (CPP_TS_CXX_LEVEL > 201103L && CPP_TS_STD_LIMIT >= 14)
#define CPP_TS_HAS_CXX17 (CPP_TS_CXX_LEVEL > 201402L && CPP_TS_STD_LIMIT >= 17)
#define CPP_TS_HAS_CXX20 (CPP_TS_CXX_LEVEL > 201703L && CPP_TS_STD_LIMIT >= 20)
#define CPP_TS_HAS_CXX23 (CPP_TS_CXX_LEVEL > 202002L && CPP_TS_STD_LIMIT >= 23)

// Library features
#if CPP_TS_HAS_CXX17 && defined(__has_include)
#if __has_include(<string_view>)
#include <string_view>
#endif // __has_include(<string_view>)
#if __has_include(<optional>)
#include <optional>
#endif // __has_include(<optional>)
#endif // CPP_TS_HAS_CXX17 && defined(__has_include)

#if CPP_TS_HAS_CXX17 && defined(__cpp_lib_string_view)
#define CPP_TS_HAS_STRING_VIEW 1
#else
#define CPP_TS_HAS_STRING_VIEW 0
#endif

#if CPP_TS_HAS_CXX17 && defined(__cpp_lib_optional)
#define CPP_TS_HAS_OPTIONAL 1
#else
#define CPP_TS_HAS_OPTIONAL 0
#endif

#if CPP_TS_HAS_CXX20 && defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#define CPP_TS_HAS_SPAN 1
#else
#define CPP_TS_HAS_SPAN 0
#endif

#if CPP_TS_HAS_CXX20 && defined(__cpp_concepts) && __cpp_concepts >= 201907L && defined(__cpp_lib_concepts)
#define CPP_TS_HAS_CONCEPTS 1
#else
#define CPP_TS_HAS_CONCEPTS 0
#endif

#if CPP_TS_HAS_CONCEPTS && defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L
#define CPP_TS_HAS_RANGES 1
#else
#define CPP_TS_HAS_RANGES 0
#endif

#if CPP_TS_HAS_CXX20 && defined(__cpp_impl_three_way_comparison) && defined(__cpp_lib_three_way_comparison)
#define CPP_TS_HAS_THREE_WAY_COMPARISON 1
#else
#define CPP_TS_HAS_THREE_WAY_COMPARISON 0
#endif

#if CPP_TS_HAS_CXX23 && defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#define CPP_TS_HAS_EXPECTED 1
#else
#define CPP_TS_HAS_EXPECTED 0
#endif

#if CPP_TS_HAS_CXX23 && defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L
#define CPP_TS_HAS_DEDUCING_THIS 1
#else
#define CPP_TS_HAS_DEDUCING_THIS 0
#endif

// Attributes / specifiers that do not exist in every supported standard.
#if CPP_TS_CXX_LEVEL > 201402L
#define CPP_TS_NODISCARD [[nodiscard]]
#else
#define CPP_TS_NODISCARD
#endif // CPP_TS_CXX_LEVEL > 201402L

#if CPP_TS_CXX_LEVEL > 201402L
#define CPP_TS_MAYBE_UNUSED [[maybe_unused]]
#elif defined(__GNUC__) || defined(__clang__)
#define CPP_TS_MAYBE_UNUSED __attribute__((unused))
#else
#define CPP_TS_MAYBE_UNUSED
#endif // CPP_TS_CXX_LEVEL > 201402L

#if defined(_MSC_VER)
#define CPP_TS_COLD __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define CPP_TS_COLD __attribute__((noinline, cold))
#else
#define CPP_TS_COLD
#endif // defined(_MSC_VER)

#if CPP_TS_CXX_LEVEL > 201103L
#define CPP_TS_CONSTEXPR14 constexpr
#else
#define CPP_TS_CONSTEXPR14
#endif // CPP_TS_CXX_LEVEL > 201103L

#if CPP_TS_HAS_CONCEPTS
#include <concepts>
#endif // CPP_TS_HAS_CONCEPTS

#if CPP_TS_HAS_RANGES
#include <ranges>
#endif // CPP_TS_HAS_RANGES

#if CPP_TS_HAS_SPAN
#include <span>
#endif // CPP_TS_HAS_SPAN

#if CPP_TS_HAS_THREE_WAY_COMPARISON
#include <compare>
#endif // CPP_TS_HAS_THREE_WAY_COMPARISON

#if CPP_TS_HAS_EXPECTED
#include <expected>
#endif // CPP_TS_HAS_EXPECTED

#include <exception>

#if !CPP_TS_HAS_STRING_VIEW
#include "string-view.hpp"
#endif // !CPP_TS_HAS_STRING_VIEW

#if !CPP_TS_HAS_SPAN
#include "span.hpp"
#endif // !CPP_TS_HAS_SPAN

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

    /// @brief Identifier of a parse state (`TSStateId`).
    using StateID = std::uint16_t;

    /// @brief Identifier of a grammar symbol (`TSSymbol`).
    using Symbol = std::uint16_t;

    /// @brief Identifier of a grammar field (`TSFieldId`).
    using FieldID = std::uint16_t;

    /// @brief ABI version of a language.
    using Version = std::uint32_t;

    /// @brief Unique identifier of a node within its tree.
    using NodeID = std::uintptr_t;

#if defined(CPP_TS_FEATURE_WASM)
    /// @brief WebAssembly engine (wasmtime).
    using WasmEngine = wasm_engine_t;
#endif // defined(CPP_TS_FEATURE_WASM)

#if CPP_TS_HAS_STRING_VIEW
    /// @brief Non-owning view over a string (`std::string_view`).
    using StringView = std::string_view;
#else
    /// @brief Non-owning view over a string (`std::string_view` replacement for older standards).
    using StringView = details::StringView;
#endif // CPP_TS_HAS_STRING_VIEW

#if CPP_TS_HAS_SPAN
    /// @brief Non-owning view over a contiguous sequence (`std::span`).
    template <typename T>
    using Span = std::span<T>;
#else
    /// @brief Non-owning view over a contiguous sequence (`std::span` replacement for older standards).
    template <typename T>
    using Span = details::Span<T>;
#endif // CPP_TS_HAS_SPAN

    /// @brief Non-owning read-only view over a contiguous byte sequence.
    using ByteView = Span<const std::uint8_t>;

    /// @brief Non-owning read-only view over grammar symbols, returned without copying from the C API.
    using SymbolSpan = Span<const Symbol>;

    /// @brief Callback used to decode source bytes.
    using DecodeFunction = std::function<std::uint32_t(ByteView, std::int32_t *)>;

#pragma endregion

#pragma region ERRORS

    /////////////////////////////////////////////////////////////////////////////
    // Errors
    // Error values returned by the non-throwing (`std::expected`) API and their mapping to exceptions.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Category of a failure reported by the wrapper.
     *
     * @note Every throwing method reports the same failures with an exception. The documented exception type of each
     * kind is listed below; constructors report every failure of the construction as `std::runtime_error`.
     */
    enum class ErrorKind : std::uint8_t
    {
        InvalidLanguage,   ///< An invalid (null) language was passed (`std::runtime_error`).
        LanguageRejected,  ///< tree-sitter rejected the language: incompatible ABI or not parseable
                           ///< (`std::runtime_error`).
        WasmStoreRequired, ///< A Wasm language needs a parser with a `WasmStore` (`std::logic_error`).
        NoLanguage,        ///< The parser has no language (`std::logic_error`).
        InvalidObject,     ///< The object cannot be used: invalid tree or store, null node, invalid language view
                           ///< (`std::logic_error`).
        InvalidArgument,   ///< An argument cannot be used: invalid (moved-from) query, null root node
                           ///< (`std::invalid_argument`).
        InvalidEdit,       ///< The old end of an `InputEdit` precedes its start (`std::invalid_argument`).
        OldTreeLanguage,   ///< `old_tree` was parsed with another language than the parser (`std::invalid_argument`).
        InputTooLarge,     ///< A size exceeds the 32-bit lengths of the C API (`std::length_error`).
        ParseFailed,       ///< tree-sitter returned no tree, e.g. the parse was cancelled by the progress callback.
                           ///< The throwing methods return an invalid `Tree` instead of throwing.
        InvalidParseState, ///< No lookahead iterator exists for the parse state (`std::runtime_error`).
        QuerySyntax,       ///< Query compilation: syntax error (`std::runtime_error`).
        QueryNodeType,     ///< Query compilation: unknown node type (`std::runtime_error`).
        QueryField,        ///< Query compilation: unknown field (`std::runtime_error`).
        QueryCapture,      ///< Query compilation: invalid capture (`std::runtime_error`).
        QueryStructure,    ///< Query compilation: impossible pattern structure (`std::runtime_error`).
        QueryLanguage,     ///< Query compilation: incompatible language (`std::runtime_error`).
        WasmParse,         ///< Wasm module parsing failed (`std::runtime_error`).
        WasmCompile,       ///< Wasm module compilation failed (`std::runtime_error`).
        WasmInstantiate,   ///< Wasm module instantiation failed (`std::runtime_error`).
        WasmAllocate,      ///< Wasm memory allocation failed (`std::runtime_error`).
        FileError,         ///< A file could not be opened or read (`std::runtime_error`).
        SystemError,       ///< A system call failed, e.g. duplicating a file descriptor (`std::runtime_error`).
        Unknown            ///< Failure without a more specific category (`std::runtime_error`).
    };

    /**
     * @brief Returns the name of an error kind (e.g. "QuerySyntax").
     * @param kind Error kind.
     * @return Null-terminated name with static storage duration.
     */
    CPP_TS_NODISCARD inline const char *getErrorKindName(ErrorKind kind) noexcept
    {
        switch (kind)
        {
            case ErrorKind::InvalidLanguage:
                return "InvalidLanguage";
            case ErrorKind::LanguageRejected:
                return "LanguageRejected";
            case ErrorKind::WasmStoreRequired:
                return "WasmStoreRequired";
            case ErrorKind::NoLanguage:
                return "NoLanguage";
            case ErrorKind::InvalidObject:
                return "InvalidObject";
            case ErrorKind::InvalidArgument:
                return "InvalidArgument";
            case ErrorKind::InvalidEdit:
                return "InvalidEdit";
            case ErrorKind::OldTreeLanguage:
                return "OldTreeLanguage";
            case ErrorKind::InputTooLarge:
                return "InputTooLarge";
            case ErrorKind::ParseFailed:
                return "ParseFailed";
            case ErrorKind::InvalidParseState:
                return "InvalidParseState";
            case ErrorKind::QuerySyntax:
                return "QuerySyntax";
            case ErrorKind::QueryNodeType:
                return "QueryNodeType";
            case ErrorKind::QueryField:
                return "QueryField";
            case ErrorKind::QueryCapture:
                return "QueryCapture";
            case ErrorKind::QueryStructure:
                return "QueryStructure";
            case ErrorKind::QueryLanguage:
                return "QueryLanguage";
            case ErrorKind::WasmParse:
                return "WasmParse";
            case ErrorKind::WasmCompile:
                return "WasmCompile";
            case ErrorKind::WasmInstantiate:
                return "WasmInstantiate";
            case ErrorKind::WasmAllocate:
                return "WasmAllocate";
            case ErrorKind::FileError:
                return "FileError";
            case ErrorKind::SystemError:
                return "SystemError";
            case ErrorKind::Unknown:
                break;
        }
        return "Unknown";
    }

    /**
     * @brief Failure reported by the non-throwing API (`std::expected<T, ts::Error>` since C++23).
     *
     * @note The message is the same as the `what()` of the exception thrown by the throwing variant.
     */
    struct Error
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Category of the failure.
        ErrorKind kind;

        /// @brief Human readable description.
        std::string message;

        /// @brief Byte offset of the failure in the input (query compilation), 0 otherwise.
        std::uint32_t offset;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Constructs an error.
         * @param error_kind Category of the failure.
         * @param error_message Human readable description.
         * @param error_offset Byte offset of the failure in the input (query compilation), 0 otherwise.
         */
        Error(ErrorKind error_kind, std::string error_message, std::uint32_t error_offset = 0)
            : kind(error_kind), message(std::move(error_message)), offset(error_offset)
        {}

#pragma endregion

#pragma region ACCESSORS

        ////////////////////////////////////////////////////////////////
        // Accessors
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the message, like `std::exception::what()`.
         * @return Null-terminated message, valid as long as the error is.
         */
        CPP_TS_NODISCARD const char *what() const noexcept
        {
            return message.c_str();
        }

#pragma endregion
    };

    namespace details
    {
        /**
         * @brief Outcome of an operation shared by its throwing and non-throwing variants.
         * @details The implementation of a fallible operation returns a status. The throwing method raises the
         * exception mapped from the kind, the `try*`/`create` methods turn it into `ts::Error`. Success and the
         * failures with a constant message do not allocate.
         */
        class Status
        {
        public:
#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Constructs a successful status.
            Status() noexcept : failed_(false), kind_(ErrorKind::Unknown), offset_(0), literal_(nullptr)
            {}

            /**
             * @brief Constructs a failure with a message of static storage duration.
             * @param kind Category of the failure.
             * @param message String literal.
             * @param offset Byte offset of the failure in the input.
             */
            Status(ErrorKind kind, const char *message, std::uint32_t offset = 0) noexcept
                : failed_(true), kind_(kind), offset_(offset), literal_(message)
            {}

            /**
             * @brief Constructs a failure with a computed message.
             * @param kind Category of the failure.
             * @param message Message.
             * @param offset Byte offset of the failure in the input.
             */
            Status(ErrorKind kind, std::string message, std::uint32_t offset = 0) noexcept
                : failed_(true), kind_(kind), offset_(offset), literal_(nullptr), owned_(std::move(message))
            {}

#pragma endregion

#pragma region ACCESSORS

            ////////////////////////////////////////////////////////////////
            // Accessors
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Checks whether the operation succeeded.
             * @return Returns true for a successful status, false for a failure.
             */
            CPP_TS_NODISCARD bool ok() const noexcept
            {
                return !failed_;
            }

            /**
             * @brief Returns the category of the failure.
             * @return Returns the kind of the failure (meaningless for a successful status).
             */
            CPP_TS_NODISCARD ErrorKind kind() const noexcept
            {
                return kind_;
            }

            /**
             * @brief Returns the message of the failure.
             * @return Returns the message (empty for a successful status).
             */
            CPP_TS_NODISCARD std::string message() const
            {
                return literal_ ? std::string(literal_) : owned_;
            }

#pragma endregion

#pragma region CONVERSION

            ////////////////////////////////////////////////////////////////
            // Conversion
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Converts the failure to the error value of the non-throwing API.
             * @return Returns the `ts::Error` describing the failure.
             */
            CPP_TS_NODISCARD Error toError() const
            {
                return Error(kind_, message(), offset_);
            }

#pragma endregion

#pragma region RAISING

            ////////////////////////////////////////////////////////////////
            // Raising
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Throws the exception documented for the kind of the failure (see `ErrorKind`).
             * @throws std::logic_error, std::invalid_argument, std::length_error or std::runtime_error.
             */
            [[noreturn]] void raise() const
            {
                switch (kind_)
                {
                    case ErrorKind::WasmStoreRequired:
                    case ErrorKind::NoLanguage:
                    case ErrorKind::InvalidObject:
                        raiseAs<std::logic_error>();
                    case ErrorKind::InvalidArgument:
                    case ErrorKind::InvalidEdit:
                    case ErrorKind::OldTreeLanguage:
                        raiseAs<std::invalid_argument>();
                    case ErrorKind::InputTooLarge:
                        raiseAs<std::length_error>();
                    default:
                        raiseAs<std::runtime_error>();
                }
            }

            /**
             * @brief Throws a specific exception type with the message of the failure.
             * @tparam Exception Exception type constructible from `std::string`.
             * @throws Exception Always.
             */
            template <typename Exception>
            [[noreturn]] void raiseAs() const
            {
                throw Exception(message());
            }

            /**
             * @brief Raises the failure, if any.
             * @throws See `raise()`, only for a failed status.
             */
            void throwIfFailed() const
            {
                if (failed_)
                {
                    raise();
                }
            }

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief True for a failure.
            bool failed_;

            /// @brief Category of the failure.
            ErrorKind kind_;

            /// @brief Byte offset of the failure in the input.
            std::uint32_t offset_;

            /// @brief Message with static storage duration, or null when the message is stored in `owned_`.
            const char *literal_;

            /// @brief Computed message (used when `literal_` is null).
            std::string owned_;

#pragma endregion
        };

        /**
         * @brief Builds the `InputTooLarge` failure.
         * @param what Description of the input used in the message.
         * @return Returns the failure.
         *
         * @note Kept out of line (`CPP_TS_COLD`): it is never on the hot path of its callers.
         */
        CPP_TS_NODISCARD CPP_TS_COLD inline Status input_too_large(const char *what)
        {
            return Status(ErrorKind::InputTooLarge,
                          std::string("Tree-sitter: ") + what + " exceeds maximum size of 4GB");
        }

        /**
         * @brief Checks whether a size fits into the 32-bit lengths used by the C API.
         * @param size Size to check.
         * @param what Description used in the message.
         * @return Success, or an `InputTooLarge` failure.
         */
        CPP_TS_NODISCARD inline Status check_length(std::size_t size, const char *what)
        {
            if (size > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
            {
                return input_too_large(what);
            }
            return Status();
        }

        /// @brief Message of an `InputEdit` whose old end precedes its start.
        constexpr const char *invalid_edit_message = "Tree-sitter: Invalid edit ranges (start > old_end)";

        /// @brief Message of an invalid language passed to a `Parser`.
        constexpr const char *invalid_parser_language_message = "Tree-sitter: Passed Language for Parser was invalid";

        /// @brief Message of a lookahead iterator requested from an invalid `LanguageView`.
        constexpr const char
                *invalid_lookahead_view_message = "Tree-sitter: Can't create LookaheadIterator for invalid Language";

        /// @brief Message of a null root node passed to `fold` / `tryFold`.
        constexpr const char *null_fold_root_message = "Tree-sitter: Cannot fold a null node";

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Converts a failed status to the error of a `std::expected`.
         * @param status Failed status.
         * @return Unexpected value holding the error.
         */
        CPP_TS_NODISCARD inline std::unexpected<Error> to_unexpected(const Status &status)
        {
            return std::unexpected<Error>(status.toError());
        }

        /**
         * @brief Converts a status to `std::expected<void, Error>`.
         * @param status Status of the operation.
         * @return Empty value on success, the error otherwise.
         */
        CPP_TS_NODISCARD inline std::expected<void, Error> to_expected(const Status &status)
        {
            if (!status.ok())
            {
                return to_unexpected(status);
            }
            return {};
        }
#endif // CPP_TS_HAS_EXPECTED
    } // namespace details

#pragma endregion

#pragma region HELPERS

    /////////////////////////////////////////////////////////////////////////////
    // Helper Classes & Structs
    // Internal utilities for memory management and range representation.
    /////////////////////////////////////////////////////////////////////////////

    static_assert(std::is_same<Symbol, TSSymbol>::value, "ts::Symbol must match TSSymbol");
    static_assert(std::is_same<FieldID, TSFieldId>::value, "ts::FieldID must match TSFieldId");
    static_assert(std::is_same<StateID, TSStateId>::value, "ts::StateID must match TSStateId");

    /**
     * @brief A half-open range representation `[start, end)`.
     * @details Tree-sitter ranges never include their end: a node spanning bytes `[2, 5)` covers bytes 2, 3 and 4.
     */
    template <typename T>
    struct Extent
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Inclusive start of the range.
        T start;

        /// @brief Exclusive end of the range.
        T end;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Default Constructor.
        constexpr Extent() noexcept(std::is_nothrow_default_constructible<T>::value) : start(), end()
        {}

        /**
         * @brief Constructs a range from its bounds.
         * @param start_value Value representing the inclusive start of the range.
         * @param end_value Value representing the exclusive end of the range.
         */
        constexpr Extent(T start_value, T end_value) noexcept(std::is_nothrow_move_constructible<T>::value)
            : start(std::move(start_value)), end(std::move(end_value))
        {}

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_THREE_WAY_COMPARISON
        /**
         * @brief Compares two ranges lexicographically (start first, then end).
         * @param lhs First range.
         * @param rhs Second range.
         * @return Ordering of the two ranges. All relational operators are synthesized from it.
         */
        friend constexpr auto operator<=>(const Extent &lhs, const Extent &rhs) = default;

        /**
         * @brief Checks whether two ranges have the same bounds.
         * @param lhs First range.
         * @param rhs Second range.
         * @return True if both bounds are equal.
         */
        friend constexpr bool operator==(const Extent &lhs, const Extent &rhs) = default;
#else
        /**
         * @brief Checks whether two ranges have the same bounds.
         * @param lhs First range.
         * @param rhs Second range.
         * @return True if both bounds are equal.
         */
        friend constexpr bool operator==(const Extent &lhs, const Extent &rhs)
        {
            return lhs.start == rhs.start && lhs.end == rhs.end;
        }

        /**
         * @brief Checks whether two ranges differ.
         * @param lhs First range.
         * @param rhs Second range.
         * @return True if any bound differs.
         */
        friend constexpr bool operator!=(const Extent &lhs, const Extent &rhs)
        {
            return !(lhs == rhs);
        }
#endif // CPP_TS_HAS_THREE_WAY_COMPARISON

#pragma endregion
    };

    class LanguageView;

    namespace details
    {
        /// @brief Type of the string parameters of the API (`ts::StringView`).
        using StringViewParameter = const StringView;

        /// @brief Type of the strings returned by the API without copying (`ts::StringView`).
        using StringViewReturn = StringView;

        /**
         * @brief Optional, non-owning reference used for optional arguments (old tree, options, ...).
         * @details Accepts the same arguments in every standard: an object (`tree`), a pointer (`&tree`), `nullptr`,
         * `{}` and, since C++17, `std::nullopt`, `std::ref(tree)` or `std::optional<std::reference_wrapper<T>>`.
         * Older versions of the wrapper used a different parameter type per standard, all of those calls still compile.
         * @tparam T Referenced type.
         */
        template <typename T>
        class OptionalParam
        {
        public:
#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Creates an empty reference.
            constexpr OptionalParam() noexcept : ptr_(nullptr)
            {}

            /// @brief Creates an empty reference.
            constexpr OptionalParam(std::nullptr_t) noexcept : ptr_(nullptr)
            {}

            /**
             * @brief References an object.
             * @param ref Referenced object. Must outlive the call it is passed to.
             */
            constexpr OptionalParam(T &ref) noexcept : ptr_(&ref)
            {}

            /**
             * @brief References an object through a pointer.
             * @param ptr Referenced object, or nullptr for an empty reference.
             */
            constexpr OptionalParam(T *ptr) noexcept : ptr_(ptr)
            {}

            /**
             * @brief References the object held by a reference wrapper.
             * @param ref Reference wrapper.
             */
            OptionalParam(std::reference_wrapper<T> ref) noexcept : ptr_(&ref.get())
            {}

#if CPP_TS_HAS_OPTIONAL
            /// @brief Creates an empty reference.
            constexpr OptionalParam(std::nullopt_t) noexcept : ptr_(nullptr)
            {}

            /**
             * @brief References the object held by an optional reference wrapper (former C++17 parameter type).
             * @param ref Optional reference.
             */
            OptionalParam(const std::optional<std::reference_wrapper<T>> &ref) noexcept
                : ptr_(ref ? &ref->get() : nullptr)
            {}
#endif // CPP_TS_HAS_OPTIONAL

#pragma endregion

#pragma region ACCESSORS

            ////////////////////////////////////////////////////////////////
            // Accessors
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the referenced object.
             * @return Pointer to the referenced object, or nullptr when empty.
             */
            CPP_TS_NODISCARD constexpr T *get() const noexcept
            {
                return ptr_;
            }

            /**
             * @brief Checks whether an object is referenced.
             * @return True if an object is referenced.
             */
            constexpr explicit operator bool() const noexcept
            {
                return ptr_ != nullptr;
            }

            /**
             * @brief Accesses the referenced object.
             * @return Pointer to the referenced object. Must not be called on an empty reference.
             */
            CPP_TS_NODISCARD constexpr T *operator->() const noexcept
            {
                return ptr_;
            }

            /**
             * @brief Accesses the referenced object.
             * @return Reference to the referenced object. Must not be called on an empty reference.
             */
            CPP_TS_NODISCARD constexpr T &operator*() const noexcept
            {
                return *ptr_;
            }

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Referenced object, or nullptr when empty.
            T *ptr_;

#pragma endregion
        };

        /**
         * @brief Returns the C API object of an optional argument.
         * @tparam T Wrapper type, convertible to `Raw *` (e.g. `Tree::operator TSTree *()`).
         * @tparam Raw C API type.
         * @param opt Optional argument.
         * @return The converted object, or nullptr when the argument is empty.
         */
        template <typename T, typename Raw>
        CPP_TS_NODISCARD inline Raw *get_raw(OptionalParam<T> opt)
        {
            return opt ? static_cast<Raw *>(*opt) : nullptr;
        }

        /**
         * @brief Stateless deleter for every resource handed out by the C API.
         * @details Unlike a function pointer deleter (`decltype(&ts_tree_delete)`), a stateless deleter does not
         * increase the size of `std::unique_ptr` and the call can be resolved (and inlined) at compile time.
         * Memory returned by the C API that has no dedicated destructor (strings, range arrays) is released with
         * `std::free`.
         */
        struct FreeHelper
        {
#pragma region OPERATORS

            ////////////////////////////////////////////////////////////////
            // Operators
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Releases memory allocated by the C API without a dedicated destructor.
             * @tparam T Type of the released memory.
             * @param raw_pointer Released memory. May be null.
             */
            template <typename T>
            void operator()(T *raw_pointer) const noexcept
            {
                std::free(const_cast<typename std::remove_cv<T>::type *>(raw_pointer));
            }

            /**
             * @brief Releases a language (`ts_language_delete`).
             * @param raw_pointer Released language. May be null.
             */
            void operator()(const TSLanguage *raw_pointer) const noexcept
            {
                ts_language_delete(raw_pointer);
            }

            /**
             * @brief Releases a tree (`ts_tree_delete`).
             * @param raw_pointer Released tree. May be null.
             */
            void operator()(TSTree *raw_pointer) const noexcept
            {
                ts_tree_delete(raw_pointer);
            }

            /**
             * @brief Releases a parser (`ts_parser_delete`).
             * @param raw_pointer Released parser. May be null.
             */
            void operator()(TSParser *raw_pointer) const noexcept
            {
                ts_parser_delete(raw_pointer);
            }

            /**
             * @brief Releases a query (`ts_query_delete`).
             * @param raw_pointer Released query. May be null.
             */
            void operator()(TSQuery *raw_pointer) const noexcept
            {
                ts_query_delete(raw_pointer);
            }

            /**
             * @brief Releases a query cursor (`ts_query_cursor_delete`).
             * @param raw_pointer Released query cursor. May be null.
             */
            void operator()(TSQueryCursor *raw_pointer) const noexcept
            {
                ts_query_cursor_delete(raw_pointer);
            }

            /**
             * @brief Releases a lookahead iterator (`ts_lookahead_iterator_delete`).
             * @param raw_pointer Released lookahead iterator. May be null.
             */
            void operator()(TSLookaheadIterator *raw_pointer) const noexcept
            {
                ts_lookahead_iterator_delete(raw_pointer);
            }

#if defined(CPP_TS_FEATURE_WASM)
            /**
             * @brief Releases a Wasm store (`ts_wasm_store_delete`).
             * @param raw_pointer Released Wasm store. May be null.
             */
            void operator()(TSWasmStore *raw_pointer) const noexcept
            {
                ts_wasm_store_delete(raw_pointer);
            }
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion
        };

        /// @brief Owning pointer to a C API resource released with the matching C API function.
        template <typename T>
        using Owned = std::unique_ptr<T, FreeHelper>;

        /**
         * @brief `std::make_unique` replacement available in C++11.
         * @tparam T Type of the created object.
         * @tparam Args Types of the constructor arguments.
         * @param args Constructor arguments.
         * @return Owning pointer to the value-initialized object.
         */
        template <typename T, typename... Args>
        CPP_TS_NODISCARD inline std::unique_ptr<T> make_unique(Args &&...args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        /**
         * @brief Returns a pointer that is safe to pass to the C API together with the view length.
         * @details A default-constructed view has a null `data()`, which the C API does not accept even for an empty
         * string (it is forwarded to `strncmp`/`memcmp`).
         * @tparam View String view type.
         * @param view View whose data is requested.
         * @return `view.data()`, or a pointer to an empty string literal when the data pointer is null.
         */
        template <typename View>
        CPP_TS_NODISCARD inline const char *safe_data(const View &view) noexcept
        {
            return view.data() != nullptr ? view.data() : "";
        }

        /**
         * @brief Checks whether a size fits into the 32-bit lengths used by the C API.
         * @param size Size to check.
         * @param what Description used in the exception message.
         * @return The size converted to `std::uint32_t`.
         * @throws std::length_error If the size exceeds `UINT32_MAX`.
         */
        inline std::uint32_t checked_length(std::size_t size, const char *what)
        {
            check_length(size, what).throwIfFailed();
            return static_cast<std::uint32_t>(size);
        }

        /**
         * @brief Rethrows (and clears) an exception captured inside a callback invoked from C code.
         * @details Exceptions must not propagate through the C frames of tree-sitter (undefined behavior), so every
         * callback trampoline catches them, asks tree-sitter to stop and stores them here. The wrapper rethrows the
         * exception once control is back in C++.
         * @param error Captured exception. Cleared before rethrowing.
         */
        inline void rethrow_if_set(std::exception_ptr &error)
        {
            if (error)
            {
                std::exception_ptr pending = error;
                error                      = nullptr;
                std::rethrow_exception(pending);
            }
        }

        /// @brief Translates query compilation errors of the C API.
        struct QueryErrorHelper
        {
#pragma region CONVERSION

            ////////////////////////////////////////////////////////////////
            // Conversion
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Formats the message of a query compilation error.
             * @param error_offset Byte offset of the error in the query source.
             * @param error Error reported by `ts_query_new`.
             * @return Returns the message.
             */
            CPP_TS_NODISCARD static std::string format_error(std::uint32_t error_offset, const TSQueryError &error);

            /**
             * @brief Converts a query compilation error to a status.
             * @param error_offset Byte offset of the error in the query source.
             * @param error Error reported by `ts_query_new`.
             * @return Returns success for `TSQueryErrorNone`, the failure of the matching `ErrorKind` otherwise.
             */
            CPP_TS_NODISCARD static Status to_status(std::uint32_t error_offset, TSQueryError error);

#pragma endregion
        };

#if defined(CPP_TS_FEATURE_WASM)
        /// @brief Translates WebAssembly errors of the C API.
        struct WasmErrorHelper
        {
#pragma region CONVERSION

            ////////////////////////////////////////////////////////////////
            // Conversion
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Formats the message of a WebAssembly error.
             * @param error Error reported by the C API.
             * @return Returns the message.
             */
            CPP_TS_NODISCARD static std::string format_error(const TSWasmError &error);

            /**
             * @brief Converts a WebAssembly error to a status and releases its message.
             * @param error Error reported by the C API. Its message is freed and reset to null.
             * @return Returns success for `TSWasmErrorKindNone`, the failure of the matching `ErrorKind` otherwise.
             */
            CPP_TS_NODISCARD static Status to_status(TSWasmError &error);

#pragma endregion
        };

        /**
         * @brief Returns the grammar name of a WebAssembly file (`path/tree-sitter-json.wasm` -> `json`).
         * @param path Path of the file.
         * @return Returns the file name without the directory, the extension and the `tree-sitter-` prefix.
         */
        CPP_TS_NODISCARD std::string get_grammar_name_from_file(StringViewParameter path);
#endif // defined(CPP_TS_FEATURE_WASM)

        /**
         * @brief Assigns a language to a parser and reports why it was rejected.
         * @details tree-sitter alone decides whether a language is accepted (and clears the current language when it
         * is not). The language is only inspected after a failure, to explain it.
         * @param parser Parser receiving the language.
         * @param language Assigned language. An invalid view clears the language.
         * @param has_wasm_store True if the parser has a WebAssembly store.
         * @return Returns success, or a `WasmStoreRequired` / `LanguageRejected` failure.
         */
        CPP_TS_NODISCARD Status assign_language(TSParser *parser, LanguageView language, bool has_wasm_store) noexcept;

        /**
         * @brief Checks whether every field of a C API tree cursor is zero (e.g. `TSTreeCursor{}`).
         * @details Such a cursor has an empty stack, the C API reads out of bounds when copying it or asking for its
         * node. Cursors created by the C API always reference their tree, so they are never all-zero.
         * @param cursor Checked cursor.
         * @return Returns true for an all-zero cursor.
         */
        CPP_TS_NODISCARD bool is_zero_initialized(const TSTreeCursor &cursor) noexcept;

        /**
         * @brief Creates a view of a null-terminated string returned by the C API.
         * @param str String returned by the C API. May be null.
         * @return View of the string, or an empty view for a null pointer.
         */
        CPP_TS_NODISCARD inline StringViewReturn make_view(const char *str) noexcept
        {
            return str ? StringViewReturn(str) : StringViewReturn();
        }

        /**
         * @brief Creates a view of a string with a known length returned by the C API.
         * @param str String returned by the C API. May be null.
         * @param length Length of the string.
         * @return View of the string, or an empty view for a null pointer or zero length.
         */
        CPP_TS_NODISCARD inline StringViewReturn make_view(const char *str, std::size_t length) noexcept
        {
            return (str && length > 0) ? StringViewReturn(str, length) : StringViewReturn();
        }

        /**
         * @brief Base class for Tree-sitter option wrappers.
         * @tparam T_TSOptions Tree-sitter options structure wrapped by this class.
         * @tparam T_TSState Tree-sitter callback state type associated with the options.
         * @tparam T_CPPState C++ wrapper type constructed from the Tree-sitter callback state.
         *
         * @note If the progress callback throws, the operation is cancelled and the exception is rethrown by the
         * wrapper once control returns from tree-sitter.
         */
        template <typename T_TSOptions, typename T_TSState, typename T_CPPState>
        class OptionsBase
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Callback invoked by Tree-sitter while the associated operation is in progress.
             * @note Returning true cancels the operation.
             */
            using ProgressCallbackFunction = std::function<bool(T_CPPState *)>;

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Constructs options with no progress callback.
            OptionsBase() : progress_callback_(nullptr), impl_{ details::make_unique<T_TSOptions>() }
            {
                impl_->payload = this;
            }

            /**
             * @brief Constructs options with a progress callback.
             * @param callback Callback invoked by Tree-sitter while the associated operation is in progress.
             */
            OptionsBase(ProgressCallbackFunction callback)
                : progress_callback_(callback ? std::move(callback) : nullptr)
                , impl_{ details::make_unique<T_TSOptions>() }
            {
                impl_->payload = this;
            }

            /**
             * @brief Disables move construction.
             * @param other Options object that would otherwise be used as the source of the move.
             *
             * @note The C API keeps a pointer to the options object as the callback payload.
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
            CPP_TS_NODISCARD const ProgressCallbackFunction &getProgressCallback() const noexcept
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

            /**
             * @brief Rethrows (and clears) an exception thrown by the progress callback during the last operation.
             * @throws Any exception previously thrown by the progress callback.
             *
             * @note Called automatically by the wrapper after the operation using these options finishes.
             */
            void rethrowPendingException() const
            {
                details::rethrow_if_set(pending_exception_);
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
            CPP_TS_NODISCARD operator T_TSOptions() const
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
            CPP_TS_NODISCARD operator T_TSOptions *() const noexcept
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

            /**
             * @brief Progress callback passed to the C API; forwards to the stored callback.
             * @param state State of the operation. Its payload is the options object.
             * @return Returns true to cancel the operation (also when the callback throws: the exception is stored and
             * rethrown by the wrapper).
             */
            static bool progressCallbackProxy(T_TSState *state) noexcept
            {
                if (state && state->payload)
                {
                    auto *self = static_cast<OptionsBase *>(state->payload);
                    if (self->progress_callback_)
                    {
                        try
                        {
                            T_CPPState cpp_state(state);
                            return self->progress_callback_(&cpp_state);
                        }
                        catch (...)
                        {
                            // Cancel the operation, the exception is rethrown by the wrapper.
                            self->pending_exception_ = std::current_exception();
                            return true;
                        }
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

            /// @brief User callback reporting the progress of the operation.
            ProgressCallbackFunction progress_callback_ = nullptr;

            /// @brief Options structure handed to the C API.
            std::unique_ptr<T_TSOptions> impl_{};

            /// @brief Exception thrown by the callback, rethrown once the C API returns.
            mutable std::exception_ptr pending_exception_{};

#pragma endregion
        };
    } // namespace details

#pragma endregion

#pragma region ENUMS

    /////////////////////////////////////////////////////////////////////////////
    // Enums
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
         * @throws std::invalid_argument If the edit contains an invalid range.
         */
        void edit(const InputEdit &edit, std::uint32_t &byte_offset);

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Applies a source edit to the point and byte offset without throwing.
         * @param edit Source edit used to update the point and byte offset.
         * @param byte_offset Byte offset updated according to the source edit.
         * @return Nothing on success, an `InvalidEdit` error if the edit contains an invalid range (nothing is
         * changed).
         */
        CPP_TS_NODISCARD std::expected<void, Error> tryEdit(const InputEdit &edit, std::uint32_t &byte_offset);
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_THREE_WAY_COMPARISON
        /**
         * @brief Compares two source positions (row first, then column).
         * @param lhs First point.
         * @param rhs Second point.
         * @return Ordering of the two points. All relational operators are synthesized from it.
         */
        CPP_TS_NODISCARD friend constexpr std::strong_ordering operator<=>(const Point &lhs,
                                                                           const Point &rhs) noexcept = default;

        /**
         * @brief Checks whether two source positions are equal.
         * @param lhs First point.
         * @param rhs Second point.
         * @return True if both points have the same row and column.
         */
        CPP_TS_NODISCARD friend constexpr bool operator==(const Point &lhs, const Point &rhs) noexcept = default;
#else
        /**
         * @brief Checks whether two source positions are equal.
         * @param other Point to compare with.
         * @return True if both points have the same row and column.
         */
        CPP_TS_NODISCARD constexpr bool operator==(const Point &other) const noexcept
        {
            return row == other.row && column == other.column;
        }

        /**
         * @brief Checks whether this point differs from another point.
         * @param other Point to compare with.
         * @return True if the points differ in row or column.
         */
        CPP_TS_NODISCARD constexpr bool operator!=(const Point &other) const noexcept
        {
            return !(*this == other);
        }

        /**
         * @brief Checks whether this point is after another point.
         * @param other Point to compare with.
         * @return True if this point has a greater row or, for the same row, a greater column.
         */
        CPP_TS_NODISCARD constexpr bool operator>(const Point &other) const noexcept
        {
            return (row > other.row) || (row == other.row && column > other.column);
        }

        /**
         * @brief Checks whether this point is before or equal to another point.
         * @param other Point to compare with.
         * @return True if this point is not after the other point.
         */
        CPP_TS_NODISCARD constexpr bool operator<=(const Point &other) const noexcept
        {
            return !(*this > other);
        }

        /**
         * @brief Checks whether this point is before another point.
         * @param other Point to compare with.
         * @return True if this point has a smaller row or, for the same row, a smaller column.
         */
        CPP_TS_NODISCARD constexpr bool operator<(const Point &other) const noexcept
        {
            return (row < other.row) || (row == other.row && column < other.column);
        }

        /**
         * @brief Checks whether this point is after or equal to another point.
         * @param other Point to compare with.
         * @return True if this point is not before the other point.
         */
        CPP_TS_NODISCARD constexpr bool operator>=(const Point &other) const noexcept
        {
            return !(*this < other);
        }
#endif // CPP_TS_HAS_THREE_WAY_COMPARISON

        /**
         * @brief Checks whether two points form a valid source range.
         * @param start Start point of the range.
         * @param end End point of the range.
         * @return True if the start point is before or equal to the end point.
         */
        CPP_TS_NODISCARD static constexpr bool isValidRange(const Point &start, const Point &end) noexcept
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
        CPP_TS_NODISCARD constexpr operator TSPoint() const noexcept
        {
            return { row, column };
        }

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Shared implementation of `edit()` and `tryEdit()`.
         * @param edit Source edit used to update the point and byte offset.
         * @param byte_offset Byte offset updated according to the source edit.
         * @return Returns success, or an `InvalidEdit` failure (nothing is changed).
         */
        details::Status editImpl(const InputEdit &edit, std::uint32_t &byte_offset) noexcept;

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
        CPP_TS_NODISCARD constexpr bool isValid() const noexcept
        {
            return start_byte <= old_end_byte && Point::isValidRange(start_point, old_end_point);
        }

        /**
         * @brief Validates the edit and throws if its ranges are invalid.
         * @throws std::invalid_argument If the old end precedes the start.
         */
        CPP_TS_CONSTEXPR14 void validate() const
        {
            if (!isValid())
            {
                throw std::invalid_argument(details::invalid_edit_message);
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
        CPP_TS_NODISCARD constexpr operator TSInputEdit() const noexcept
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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Updates the range to reflect a source edit without throwing.
         * @param edit Edit applied to the source code.
         * @return Nothing on success, an `InvalidEdit` error if the edit contains an invalid range (nothing is
         * changed).
         */
        CPP_TS_NODISCARD std::expected<void, Error> tryEdit(const InputEdit &edit);
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Converts the range to its C API representation.
         * @return A `TSRange` containing the same source positions and byte offsets.
         */
        CPP_TS_NODISCARD constexpr operator TSRange() const noexcept
        {
            return { point.start, point.end, byte.start, byte.end };
        }

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether two ranges cover the same source.
         * @param lhs First range.
         * @param rhs Second range.
         * @return True if both the positions and the byte offsets are equal.
         */
        CPP_TS_NODISCARD friend constexpr bool operator==(const Range &lhs, const Range &rhs) noexcept
        {
            return lhs.byte == rhs.byte && lhs.point == rhs.point;
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Checks whether two ranges differ.
         * @param lhs First range.
         * @param rhs Second range.
         * @return True if the positions or the byte offsets differ.
         */
        CPP_TS_NODISCARD friend constexpr bool operator!=(const Range &lhs, const Range &rhs) noexcept
        {
            return !(lhs == rhs);
        }
#endif // !CPP_TS_HAS_CXX20

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Shared implementation of `edit()` and `tryEdit()`.
         * @param edit Edit applied to the source code.
         * @return Returns success, or an `InvalidEdit` failure (nothing is changed).
         */
        details::Status editImpl(const InputEdit &edit) noexcept;

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
        CPP_TS_NODISCARD const ReadFunction &getRead() const noexcept
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
        CPP_TS_NODISCARD InputEncoding getEncoding() const noexcept
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
        CPP_TS_NODISCARD const DecodeFunction &getDecode() const noexcept
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
        CPP_TS_NODISCARD operator TSInput() const;

        /**
         * @brief Returns the C API input representation managed by this object.
         * @return Pointer to the underlying `TSInput`.
         */
        CPP_TS_NODISCARD operator TSInput *() const noexcept;

#pragma endregion

    private:
#pragma region CURRENT_INPUT

        ////////////////////////////////////////////////////////////////
        // Current Input
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the input being parsed by the current thread.
         * @details The C decode callback receives no payload, so the input is published here for the duration of a
         * parse.
         * @return Reference to the thread-local pointer to the current input (null outside of a parse).
         */
        CPP_TS_NODISCARD static const Input *&current() noexcept
        {
            static thread_local const Input *ptr = nullptr;
            return ptr;
        }

        /// @brief Publishes an input as the current one for a scope and restores the previous one afterwards.
        class ScopedCurrent
        {
        public:
#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Makes an input the current one.
             * @param input Input being parsed.
             */
            explicit ScopedCurrent(const Input &input) noexcept : previous_(current())
            {
                current() = &input;
            }

            /// @brief Restores the previous current input.
            ~ScopedCurrent()
            {
                current() = previous_;
            }

            /**
             * @brief Deleted copy constructor.
             * @param other Another scope.
             */
            ScopedCurrent(const ScopedCurrent &other) = delete;

            /**
             * @brief Deleted copy assignment operator.
             * @param other Another scope.
             * @return Returns a reference to this scope.
             */
            ScopedCurrent &operator=(const ScopedCurrent &other) = delete;

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Current input before this scope.
            const Input *previous_;

#pragma endregion
        };

        friend class Parser;

#pragma endregion

#pragma region PROXIES

        ////////////////////////////////////////////////////////////////
        // Proxies
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Read callback passed to the C API; forwards to the stored read function.
         * @param payload The input.
         * @param byte_index Byte offset of the requested chunk.
         * @param position Position of the requested chunk.
         * @param bytes_read Receives the length of the returned chunk.
         * @return Returns the chunk, or an empty chunk at the end of the input or when the callback throws (the
         * exception is stored and rethrown by the parser).
         */
        static const char *read_proxy(void          *payload,
                                      std::uint32_t  byte_index,
                                      TSPoint        position,
                                      std::uint32_t *bytes_read) noexcept;

        /**
         * @brief Decode callback passed to the C API; forwards to the decode function of the current input.
         * @param string Bytes to decode.
         * @param length Number of bytes available.
         * @param code_point Receives the decoded code point.
         * @return Returns the number of bytes consumed. Without a decoder, or when the callback throws (the exception
         * is stored and rethrown by the parser), reports an invalid character consuming one byte.
         */
        static std::uint32_t decode_proxy(const std::uint8_t *string,
                                          std::uint32_t       length,
                                          std::int32_t       *code_point) noexcept;

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Function returning the chunks of the source.
        ReadFunction read_ = nullptr;

        /// @brief Function decoding the source when the encoding is `InputEncoding::Custom`.
        DecodeFunction decode_ = nullptr;

        /// @brief Encoding of the source.
        InputEncoding encoding_ = InputEncoding::UTF8;

        /// @brief Input structure handed to the C API.
        std::unique_ptr<TSInput> impl_{};

        /// @brief Exception thrown by a callback, rethrown once the parse returns.
        mutable std::exception_ptr pending_exception_{};

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
        CPP_TS_NODISCARD std::uint32_t getSymbolsCount() const noexcept
        {
            return isValid() ? ts_language_symbol_count(impl_) : 0;
        }

        /**
         * @brief Returns the number of parser states defined by the language.
         * @return Number of states, or `0` if the view is invalid.
         */
        CPP_TS_NODISCARD std::uint32_t getStatesCount() const noexcept
        {
            return isValid() ? ts_language_state_count(impl_) : 0;
        }

        /**
         * @brief Returns the number of fields defined by the language.
         * @return Number of fields, or `0` if the view is invalid.
         */
        CPP_TS_NODISCARD std::uint32_t getFieldsCount() const noexcept
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
        CPP_TS_NODISCARD details::StringViewReturn getSymbolName(Symbol symbol) const;

        /**
         * @brief Returns the type of a symbol.
         * @param symbol Symbol whose type is requested.
         * @return Symbol type, or `SymbolType::TypeRegular` if the view is invalid.
         */
        CPP_TS_NODISCARD SymbolType getSymbolType(Symbol symbol) const;

        /**
         * @brief Finds a symbol by its name.
         * @param name Symbol name to find.
         * @param isNamed Whether to search for a named symbol.
         * @return Matching symbol, or `0` if the symbol is not found or the view is invalid.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        CPP_TS_NODISCARD Symbol getSymbolForName(details::StringViewParameter name, bool isNamed) const;

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
        CPP_TS_NODISCARD details::StringViewReturn getFieldNameForID(FieldID id) const;

        /**
         * @brief Finds a field ID by its name.
         * @param name Field name to find.
         * @return Matching field ID, or `0` if the field is not found or the view is invalid.
         * @throws std::length_error If `name` exceeds the maximum supported size.
         */
        CPP_TS_NODISCARD FieldID getFieldIDForName(details::StringViewParameter name) const;

        /**
         * @brief Returns the names of all fields defined by the language.
         * @return Field names, or an empty vector if the view is invalid or the language has no fields.
         */
        CPP_TS_NODISCARD std::vector<details::StringViewReturn> getAllFieldsNames() const;

#pragma endregion

#pragma region TYPE_HIERARCHY

        ////////////////////////////////////////////////////////////////
        // Type Hierarchy
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns all supertypes defined by the language.
         * @return Symbols representing all supertypes, or an empty vector if none are defined.
         */
        CPP_TS_NODISCARD std::vector<Symbol> getAllSuperTypes() const noexcept;

        /**
         * @brief Returns all subtypes associated with a supertype.
         * @param supertype Supertype whose subtypes are requested.
         * @return Symbols representing the subtypes, or an empty vector if none are defined.
         */
        CPP_TS_NODISCARD std::vector<Symbol> getAllSubTypesForSuperType(Symbol supertype) const noexcept;

        /**
         * @brief Returns all supertypes defined by the language without copying them.
         * @return View of the language's supertype table, or an empty view if none are defined.
         *
         * @note The view points into the language and is valid as long as the language is alive (for statically
         * linked grammars, for the whole program). Requires a grammar with ABI 15 or newer.
         */
        CPP_TS_NODISCARD SymbolSpan getSuperTypesView() const noexcept
        {
            std::uint32_t   count = 0;
            const TSSymbol *array = isValid() ? ts_language_supertypes(impl_, &count) : nullptr;
            return array ? SymbolSpan(array, count) : SymbolSpan();
        }

        /**
         * @brief Returns all subtypes associated with a supertype without copying them.
         * @param supertype Supertype whose subtypes are requested.
         * @return View of the subtypes, or an empty view if none are defined.
         *
         * @note The view points into the language and is valid as long as the language is alive.
         */
        CPP_TS_NODISCARD SymbolSpan getSubTypesView(Symbol supertype) const noexcept
        {
            std::uint32_t   count = 0;
            const TSSymbol *array = isValid() ? ts_language_subtypes(impl_, supertype, &count) : nullptr;
            return array ? SymbolSpan(array, count) : SymbolSpan();
        }

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
        CPP_TS_NODISCARD StateID getNextState(StateID state, Symbol symbol) const noexcept
        {
            return isValid() ? ts_language_next_state(impl_, state, symbol) : 0;
        }

        /**
         * @brief Creates an iterator over symbols valid for a parser state.
         * @param state Parser state whose valid lookahead symbols are requested.
         * @return Iterator over the valid lookahead symbols.
         * @throws std::logic_error If the view is invalid.
         * @throws std::runtime_error If the parse state is invalid.
         */
        CPP_TS_NODISCARD LookaheadIterator getLookaheadIterator(StateID state) const;

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Creates an iterator over symbols valid for a parser state without throwing.
         * @param state Parser state whose valid lookahead symbols are requested.
         * @return Iterator over the valid lookahead symbols, or an error: `InvalidObject` (invalid view) or
         * `InvalidParseState`.
         */
        CPP_TS_NODISCARD std::expected<LookaheadIterator, Error> tryGetLookaheadIterator(StateID state) const;
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region METADATA

        ////////////////////////////////////////////////////////////////
        // Metadata
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the name of the language.
         * @return Language name, or an empty string if the view is invalid.
         */
        CPP_TS_NODISCARD details::StringViewReturn getName() const;

        /**
         * @brief Returns the ABI version of the language.
         * @return Language ABI version, or `0` if the view is invalid.
         */
        CPP_TS_NODISCARD Version getVersion() const noexcept
        {
            return isValid() ? ts_language_abi_version(impl_) : 0;
        }

        /**
         * @brief Checks whether the view references a valid language.
         * @return Returns true if the view references a language, false otherwise.
         */
        CPP_TS_NODISCARD constexpr bool isValid() const noexcept
        {
            return impl_ != nullptr;
        }

        /**
         * @brief Checks whether the language can be assigned to a parser.
         * @return Returns true if the language can be used for parsing, false otherwise (or if the view is invalid).
         *
         * @note Languages obtained from a syntax tree can always be used to inspect that tree, but are not
         * necessarily usable for parsing (e.g. when tree-sitter is compiled to WebAssembly).
         */
        CPP_TS_NODISCARD bool isParseable() const noexcept
        {
            return isValid() && ts_language_is_parseable(impl_);
        }

#if CPP_TS_HAS_OPTIONAL
        /**
         * @brief Returns the version metadata of the language.
         * @return Language metadata, or `std::nullopt` if unavailable.
         */
        CPP_TS_NODISCARD std::optional<LanguageMetadata> getMetadata() const noexcept;
#else
        /**
         * @brief Returns the version metadata of the language.
         * @return Language metadata, or a zero-initialized value if unavailable.
         */
        CPP_TS_NODISCARD LanguageMetadata getMetadata() const noexcept;
#endif // CPP_TS_HAS_OPTIONAL

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Checks whether the language uses a WebAssembly parser.
         * @return Returns true if the language is a WebAssembly parser, false otherwise.
         */
        CPP_TS_NODISCARD bool isWasm() const noexcept
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
        CPP_TS_NODISCARD constexpr operator const TSLanguage *() const noexcept
        {
            return impl_;
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Viewed language, or null for an invalid view.
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
        CPP_TS_NODISCARD LanguageView getView() const noexcept
        {
            return LanguageView(impl_.get());
        }

        /**
         * @brief Checks whether a language is currently owned.
         * @return Returns true if a language is owned, false otherwise.
         */
        CPP_TS_NODISCARD bool isValid() const noexcept
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
        CPP_TS_NODISCARD operator LanguageView() const noexcept
        {
            return LanguageView(impl_.get());
        }

        /**
         * @brief Returns the owned language as a C API pointer.
         * @return Pointer to the owned language, or `nullptr` if no language is owned.
         */
        CPP_TS_NODISCARD operator const TSLanguage *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned language (released with `ts_language_delete`), or null for an invalid language.
        details::Owned<const TSLanguage> impl_{};

#pragma endregion
    };

#pragma endregion

#pragma region NODE

    /////////////////////////////////////////////////////////////////////////////
    // Node
    // A single node in a parse tree, providing navigation and attributes.
    /////////////////////////////////////////////////////////////////////////////

    class TreeCursor;
    struct Children;

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
        CPP_TS_NODISCARD static constexpr Node null() noexcept
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
        CPP_TS_NODISCARD NodeID getID() const noexcept
        {
            return reinterpret_cast<NodeID>(impl_.id);
        }

        /**
         * @brief Returns the node type.
         * @return Node type, or an empty string for a null node.
         */
        CPP_TS_NODISCARD details::StringViewReturn getType() const
        {
            return isNull() ? details::StringViewReturn("") : details::make_view(ts_node_type(impl_));
        }

        /**
         * @brief Returns the grammar symbol associated with the node.
         * @return Node symbol, or `0` for a null node.
         */
        CPP_TS_NODISCARD Symbol getSymbol() const noexcept
        {
            return isNull() ? 0 : ts_node_symbol(impl_);
        }

        /**
         * @brief Returns the grammar type of the node.
         * @return Grammar type, or an empty string for a null node.
         */
        CPP_TS_NODISCARD details::StringViewReturn getGrammarType() const
        {
            return isNull() ? details::StringViewReturn("") : details::make_view(ts_node_grammar_type(impl_));
        }

        /**
         * @brief Returns the grammar symbol associated with the node.
         * @return Grammar symbol, or `0` for a null node.
         */
        CPP_TS_NODISCARD Symbol getGrammarSymbol() const noexcept
        {
            return isNull() ? 0 : ts_node_grammar_symbol(impl_);
        }

        /**
         * @brief Returns the language associated with the node.
         * @return Non-owning view of the node's language, or an invalid view for a null node.
         */
        CPP_TS_NODISCARD LanguageView getLanguage() const noexcept
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
        CPP_TS_NODISCARD bool isNull() const noexcept
        {
            return ts_node_is_null(impl_);
        }

        /**
         * @brief Checks whether the node is named.
         * @return Returns true if the node is named and not null, false otherwise.
         */
        CPP_TS_NODISCARD bool isNamed() const noexcept
        {
            return !isNull() && ts_node_is_named(impl_);
        }

        /**
         * @brief Checks whether the node was inserted as a missing node during parsing.
         * @return Returns true if the node is missing and not null, false otherwise.
         */
        CPP_TS_NODISCARD bool isMissing() const noexcept
        {
            return !isNull() && ts_node_is_missing(impl_);
        }

        /**
         * @brief Checks whether the node is an extra node.
         * @return Returns true if the node is extra and not null, false otherwise.
         */
        CPP_TS_NODISCARD bool isExtra() const noexcept
        {
            return !isNull() && ts_node_is_extra(impl_);
        }

        /**
         * @brief Checks whether the node represents a syntax error.
         * @return Returns true if the node is an error node and not null, false otherwise.
         */
        CPP_TS_NODISCARD bool isError() const noexcept
        {
            return !isNull() && ts_node_is_error(impl_);
        }

        /**
         * @brief Checks whether the node or any of its descendants contains an error.
         * @return Returns true if the node contains an error and is not null, false otherwise.
         */
        CPP_TS_NODISCARD bool hasError() const noexcept
        {
            return !isNull() && ts_node_has_error(impl_);
        }

        /**
         * @brief Checks whether the node has been changed by a tree edit.
         * @return Returns true if the node has changes and is not null, false otherwise.
         */
        CPP_TS_NODISCARD bool hasChanges() const noexcept
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
        CPP_TS_NODISCARD Extent<std::uint32_t> getByteRange() const noexcept
        {
            return isNull() ? Extent<std::uint32_t>()
                            : Extent<std::uint32_t>({ ts_node_start_byte(impl_), ts_node_end_byte(impl_) });
        }

        /**
         * @brief Returns the source position range occupied by the node.
         * @return Start and end source positions, or an empty range for a null node.
         */
        CPP_TS_NODISCARD Extent<Point> getPointRange() const noexcept
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
        CPP_TS_NODISCARD details::StringViewReturn getSourceRange(details::StringViewParameter source) const;

        /**
         * @brief Returns a copy of the source text covered by the node.
         * @param source Complete source text from which the node originates.
         * @return Source text covered by the node, or an empty string for a null node or an invalid byte range.
         */
        CPP_TS_NODISCARD std::string getSourceText(details::StringViewParameter source) const;

        /**
         * @brief Returns an S-expression representation of the node and its descendants.
         * @return A null pointer for a null node, the S-expression representation otherwise.
         */
        CPP_TS_NODISCARD details::Owned<char> getSExpr() const
        {
            return details::Owned<char>(isNull() ? nullptr : ts_node_string(impl_));
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
        CPP_TS_NODISCARD Node getParent() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_parent(impl_) };
        }

        /**
         * @brief Returns the next sibling.
         * @return Next sibling, or a null node if none exists.
         */
        CPP_TS_NODISCARD Node getNextSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_next_sibling(impl_) };
        }

        /**
         * @brief Returns the previous sibling.
         * @return Previous sibling, or a null node if none exists.
         */
        CPP_TS_NODISCARD Node getPreviousSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_prev_sibling(impl_) };
        }

        /**
         * @brief Returns the number of children.
         * @return Number of children, or `0` for a null node.
         */
        CPP_TS_NODISCARD std::uint32_t getChildCount() const noexcept
        {
            return isNull() ? 0 : ts_node_child_count(impl_);
        }

        /**
         * @brief Returns a child by its index.
         * @param child_index Zero-based child index.
         * @return Child at the specified index, or a null node if the index is out of range.
         */
        CPP_TS_NODISCARD Node getChild(std::uint32_t child_index) const noexcept
        {
            return (child_index >= getChildCount()) ? null() : Node{ ts_node_child(impl_, child_index) };
        }

        /**
         * @brief Returns the first child.
         * @return First child, or a null node if there are no children.
         */
        CPP_TS_NODISCARD Node getFirstChild() const noexcept
        {
            return getChild(0);
        }

        /**
         * @brief Returns the last child.
         * @return Last child, or a null node if there are no children.
         */
        CPP_TS_NODISCARD Node getLastChild() const noexcept
        {
            std::uint32_t count = getChildCount();
            return getChild(count > 0 ? count - 1 : count);
        }

        /**
         * @brief Returns the child that contains a specified descendant.
         * @param descendant Descendant node to locate.
         * @return The direct child containing the descendant, or a null node if none is found.
         */
        CPP_TS_NODISCARD Node getChildWithDescendant(Node descendant) const noexcept
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
        CPP_TS_NODISCARD Node getChildByType(details::StringViewParameter name) const;

        /**
         * @brief Returns the first child with the specified symbol.
         * @param sym Symbol to find.
         * @return Matching child, or a null node if none is found.
         */
        CPP_TS_NODISCARD Node getChildBySymbol(Symbol sym) const noexcept;

#pragma endregion

#pragma region NAVIGATION_NAMED_CHILDREN

        ////////////////////////////////////////////////////////////////
        // Navigation (Named Children)
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the next named sibling.
         * @return Next named sibling, or a null node if none exists.
         */
        CPP_TS_NODISCARD Node getNextNamedSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_next_named_sibling(impl_) };
        }

        /**
         * @brief Returns the previous named sibling.
         * @return Previous named sibling, or a null node if none exists.
         */
        CPP_TS_NODISCARD Node getPreviousNamedSibling() const noexcept
        {
            return isNull() ? null() : Node{ ts_node_prev_named_sibling(impl_) };
        }

        /**
         * @brief Returns the number of named children.
         * @return Number of named children, or `0` for a null node.
         */
        CPP_TS_NODISCARD std::uint32_t getNamedChildCount() const noexcept
        {
            return isNull() ? 0 : ts_node_named_child_count(impl_);
        }

        /**
         * @brief Returns a named child by its index.
         * @param child_index Zero-based named child index.
         * @return Named child at the specified index, or a null node if the index is out of range.
         */
        CPP_TS_NODISCARD Node getNamedChild(std::uint32_t child_index) const noexcept
        {
            return (child_index >= getNamedChildCount()) ? null() : Node{ ts_node_named_child(impl_, child_index) };
        }

        /**
         * @brief Returns the first named child.
         * @return First named child, or a null node if there are no named children.
         */
        CPP_TS_NODISCARD Node getFirstNamedChild() const noexcept
        {
            return getNamedChild(0);
        }

        /**
         * @brief Returns the last named child.
         * @return Last named child, or a null node if there are no named children.
         */
        CPP_TS_NODISCARD Node getLastNamedChild() const noexcept
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
        CPP_TS_NODISCARD Node getNamedChildByType(details::StringViewParameter name) const;

        /**
         * @brief Returns the first named child with the specified symbol.
         * @param sym Symbol to find.
         * @return Matching named child, or a null node if none is found.
         */
        CPP_TS_NODISCARD Node getNamedChildBySymbol(Symbol sym) const noexcept;

#pragma endregion

#pragma region SEARCH_AND_DESCENDANTS

        ////////////////////////////////////////////////////////////////
        // Search & Descendants
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the number of descendants of the node.
         * @return Number of descendants, or `0` for a null node.
         */
        CPP_TS_NODISCARD std::uint32_t getDescendantsCount() const noexcept
        {
            return isNull() ? 0 : ts_node_descendant_count(impl_);
        }

        /**
         * @brief Returns the first child that extends to the specified byte offset.
         * @param byte Byte offset to search.
         * @return First matching child, or a null node if none is found.
         */
        CPP_TS_NODISCARD Node getFirstChildForByte(std::uint32_t byte) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_first_child_for_byte(impl_, byte) };
        }

        /**
         * @brief Returns the first named child that extends to the specified byte offset.
         * @param byte Byte offset to search.
         * @return First matching named child, or a null node if none is found.
         */
        CPP_TS_NODISCARD Node getFirstNamedChildForByte(std::uint32_t byte) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_first_named_child_for_byte(impl_, byte) };
        }

        /**
         * @brief Returns the smallest descendant covering a byte range.
         * @param range Byte range to search.
         * @return Matching descendant, or a null node if the node is null.
         */
        CPP_TS_NODISCARD Node getDescendantForByteRange(Extent<std::uint32_t> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_descendant_for_byte_range(impl_, range.start, range.end) };
        }

        /**
         * @brief Returns the smallest named descendant covering a byte range.
         * @param range Byte range to search.
         * @return Matching named descendant, or a null node if the node is null.
         */
        CPP_TS_NODISCARD Node getNamedDescendantForByteRange(Extent<std::uint32_t> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_named_descendant_for_byte_range(impl_, range.start, range.end) };
        }

        /**
         * @brief Returns the smallest descendant covering a source position range.
         * @param range Source position range to search.
         * @return Matching descendant, or a null node if the node is null.
         */
        CPP_TS_NODISCARD Node getDescendantForPointRange(Extent<Point> range) const noexcept
        {
            return isNull() ? null() : Node{ ts_node_descendant_for_point_range(impl_, range.start, range.end) };
        }

        /**
         * @brief Returns the smallest named descendant covering a source position range.
         * @param range Source position range to search.
         * @return Matching named descendant, or a null node if the node is null.
         */
        CPP_TS_NODISCARD Node getNamedDescendantForPointRange(Extent<Point> range) const noexcept
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
        CPP_TS_NODISCARD details::StringViewReturn getFieldNameForChild(std::uint32_t child_index) const
        {
            return isNull() ? details::StringViewReturn("")
                            : details::make_view(ts_node_field_name_for_child(impl_, child_index));
        }

        /**
         * @brief Returns the field name associated with a named child.
         * @param named_child_index Zero-based named child index.
         * @return Field name, or an empty string if no field is associated with the child.
         */
        CPP_TS_NODISCARD details::StringViewReturn getFieldNameForNamedChild(std::uint32_t named_child_index) const
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
        CPP_TS_NODISCARD Node getChildByFieldName(details::StringViewParameter name) const;

        /**
         * @brief Returns the child associated with a field ID.
         * @param field_id Field ID to find.
         * @return Child associated with the field, or a null node if none is found.
         */
        CPP_TS_NODISCARD Node getChildByFieldID(FieldID field_id) const noexcept
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
        CPP_TS_NODISCARD StateID getParseState() const noexcept
        {
            return isNull() ? 0 : ts_node_parse_state(impl_);
        }

        /**
         * @brief Returns the parser state associated with the next parse step.
         * @return Next parser state, or `0` for a null node.
         */
        CPP_TS_NODISCARD StateID getNextParseState() const noexcept
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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Updates the node to reflect an edit to the source code without throwing.
         * @param edit Edit applied to the source code.
         * @return Nothing on success; an `InvalidObject` error for a null node or an `InvalidEdit` error for invalid
         * ranges (nothing is changed).
         */
        CPP_TS_NODISCARD std::expected<void, Error> tryEdit(const InputEdit &edit);
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region CURSOR

        ////////////////////////////////////////////////////////////////
        // Cursor
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Creates a tree cursor positioned at this node.
         * @return Cursor positioned at this node.
         */
        CPP_TS_NODISCARD TreeCursor getCursor() const noexcept;

#pragma endregion

#pragma region CHILDREN_VIEWS

        ////////////////////////////////////////////////////////////////
        // Children Views
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns a lazy range over all children of the node.
         * @return Range yielding every child (named and anonymous) in order.
         */
        CPP_TS_NODISCARD Children getChildren() const noexcept;

        /**
         * @brief Returns a lazy range over the named children of the node.
         * @return Range yielding only the named children in order.
         */
        CPP_TS_NODISCARD Children getNamedChildren() const noexcept;

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
        CPP_TS_NODISCARD bool operator==(const Node &other) const noexcept
        {
            return ts_node_eq(impl_, other.impl_);
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Checks whether two nodes represent different parse-tree nodes.
         * @param other Node to compare with.
         * @return Returns true if the nodes are different, false otherwise.
         */
        CPP_TS_NODISCARD bool operator!=(const Node &other) const noexcept
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
        CPP_TS_NODISCARD constexpr operator TSNode() const noexcept
        {
            return impl_;
        }

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Shared implementation of `edit()` and `tryEdit()`.
         * @param edit Edit applied to the source code.
         * @return Returns success, or an `InvalidObject` (null node) / `InvalidEdit` failure (nothing is changed).
         */
        details::Status editImpl(const InputEdit &edit) noexcept;

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Underlying C API node (a plain value, owned by its tree).
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
        explicit Tree(TSTree *tree) noexcept : impl_{ tree }
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
        CPP_TS_NODISCARD bool hasError() const noexcept
        {
            return isValid() && getRootNode().hasError();
        }

        /**
         * @brief Checks whether the tree contains a valid parse tree.
         * @return Returns true if the tree is valid, false otherwise.
         */
        CPP_TS_NODISCARD bool isValid() const noexcept
        {
            return impl_ != nullptr;
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
        CPP_TS_NODISCARD Node getRootNode() const noexcept
        {
            return isValid() ? Node{ ts_tree_root_node(impl_.get()) } : Node::null();
        }

        /**
         * @brief Returns the root node with its position shifted by the given byte and point offsets.
         * @param offset_bytes Byte offset to add to the root node positions.
         * @param offset_extent Point offset to add to the root node positions.
         * @return Root node with adjusted positions, or a null node if the tree is invalid.
         */
        CPP_TS_NODISCARD Node getRootNodeWithOffset(std::uint32_t offset_bytes, Point offset_extent) const noexcept
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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Applies an edit to the tree without throwing.
         * @param edit Description of the source edit.
         * @return Nothing on success; an `InvalidObject` error for an invalid tree or an `InvalidEdit` error for
         * invalid ranges (nothing is changed).
         */
        CPP_TS_NODISCARD std::expected<void, Error> tryEdit(const InputEdit &edit);
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region RANGES

        ////////////////////////////////////////////////////////////////
        // Ranges
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the ranges of source code included in the tree.
         * @return Vector of included source ranges, or an empty vector if the tree is invalid.
         */
        CPP_TS_NODISCARD std::vector<Range> getIncludedRanges() const;

        /**
         * @brief Returns the ranges that differ between two parse trees.
         * @param old_tree Previous parse tree.
         * @param new_tree Updated parse tree.
         * @return Vector of ranges that changed between the trees.
         */
        CPP_TS_NODISCARD static std::vector<Range> getChangedRanges(const Tree &old_tree, const Tree &new_tree);

#pragma endregion

#pragma region LANGUAGE

        ////////////////////////////////////////////////////////////////
        // Language
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the grammar used to create the tree.
         * @return Non-owning view of the tree's grammar, or an invalid view if the tree is invalid.
         */
        CPP_TS_NODISCARD LanguageView getLanguage() const noexcept
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
        CPP_TS_NODISCARD Tree copy() const noexcept
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
        CPP_TS_NODISCARD operator TSTree *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Shared implementation of `edit()` and `tryEdit()`.
         * @param edit Description of the source edit.
         * @return Returns success, or an `InvalidObject` (invalid tree) / `InvalidEdit` failure (nothing is changed).
         */
        details::Status editImpl(const InputEdit &edit) noexcept;

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned C API tree, or null for an invalid tree.
        details::Owned<TSTree> impl_{};

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
        Parser() noexcept : impl_{ ts_parser_new() }
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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Creates a new parser with the specified language without throwing.
         * @param language The language to assign.
         * @return Returns a parser, or an error: `InvalidLanguage`, `LanguageRejected` or `WasmStoreRequired`.
         */
        CPP_TS_NODISCARD static std::expected<Parser, Error> create(LanguageView language);

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Creates a new parser with the specified WebAssembly language and store without throwing.
         * @param language The WebAssembly language to assign.
         * @param store The WebAssembly store to use. Taken over on success, left to the caller on failure.
         * @return Returns a parser, or an error: `InvalidLanguage` or `LanguageRejected`.
         */
        CPP_TS_NODISCARD static std::expected<Parser, Error> create(LanguageView language, WasmStore &store);
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region CONFIGURATION

        ////////////////////////////////////////////////////////////////
        // Configuration
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Sets the language that the parser will use.
         * @param language The language to assign.
         * @return Returns true on success, or false when tree-sitter rejects the language (e.g. ABI incompatibility).
         * @throws std::logic_error If the language is WebAssembly and the parser lacks a `WasmStore` (only when WASM is
         * enabled).
         *
         * @note tree-sitter decides whether the language is accepted. On failure the parser has no language any more
         * (tree-sitter clears it); an invalid view intentionally clears the language.
         */
        CPP_TS_NODISCARD bool setLanguage(LanguageView language)
#if defined(CPP_TS_FEATURE_WASM)
                ;
#else
                noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Sets the language that the parser will use, reporting why it was rejected.
         * @param language The language to assign. An invalid view clears the language (success).
         * @return Nothing on success, or an error: `LanguageRejected` (ABI incompatible or not parseable) or
         * `WasmStoreRequired`.
         *
         * @note tree-sitter decides whether the language is accepted. On failure the parser has no language any more
         * (tree-sitter clears it).
         */
        CPP_TS_NODISCARD std::expected<void, Error> trySetLanguage(LanguageView language);
#endif // CPP_TS_HAS_EXPECTED

#if CPP_TS_HAS_SPAN
        /**
         * @brief Restricts parsing to a specific set of text ranges.
         * @param ranges A span of ranges indicating the portions of text to parse.
         * @return Returns true if ranges were set successfully.
         * @throws std::length_error If the underlying merged range count exceeds the maximum size (UINT32_MAX).
         */
        CPP_TS_NODISCARD bool setIncludedRanges(std::span<const Range> ranges);
#else
        /**
         * @brief Restricts parsing to a specific set of text ranges.
         * @param ranges A vector of ranges indicating the portions of text to parse.
         * @return Returns true if ranges were set successfully.
         * @throws std::length_error If the underlying merged range count exceeds the maximum size (UINT32_MAX).
         */
        CPP_TS_NODISCARD bool setIncludedRanges(const std::vector<Range> &ranges);
#endif // CPP_TS_HAS_SPAN

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
        CPP_TS_NODISCARD LanguageView getCurrentLanguage() const noexcept
        {
            return hasLanguage() ? LanguageView(ts_parser_language(impl_.get())) : LanguageView(nullptr);
        }

        /**
         * @brief Retrieves the ranges of text that the parser will include when parsing.
         * @return Returns a vector of included ranges.
         */
        CPP_TS_NODISCARD std::vector<Range> getIncludedRanges() const noexcept;

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Retrieves the WebAssembly store from the parser.
         * @return Returns the `WasmStore` that was previously owned by this parser.
         *
         * @note If the current language is WebAssembly, it will be removed from the parser during this operation.
         */
        CPP_TS_NODISCARD WasmStore takeWasmStore() noexcept;
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
        CPP_TS_NODISCARD bool hasLanguage() const noexcept
        {
            return impl_ && ts_parser_language(impl_.get()) != nullptr;
        }

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Checks if a WebAssembly store has been assigned to the parser.
         * @return Returns true if a store is currently assigned, false otherwise.
         */
        CPP_TS_NODISCARD bool hasWasmStore() const noexcept
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
         * @throws std::invalid_argument If `old_tree` was parsed with a different language.
         * @throws Any exception thrown by the read/decode callbacks of `input`, the progress callback of `options` or
         * the logger (the parse is stopped and the exception is rethrown here).
         *
         * @note Optional arguments accept an object, a pointer, `nullptr` or `{}` (and `std::nullopt` since C++17).
         */
        CPP_TS_NODISCARD Tree parse(Input                                     &input,
                                    details::OptionalParam<Tree>               old_tree = {},
                                    details::OptionalParam<const ParseOptions> options  = {});

        /**
         * @brief Parses source code provided as a contiguous string.
         * @param buffer The string buffer containing the source code to parse.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns the newly generated syntax tree.
         * @throws std::logic_error If no language has been set prior to calling this method.
         * @throws std::invalid_argument If `old_tree` was parsed with a different language.
         * @throws std::length_error If the input buffer size exceeds 4GB.
         */
        CPP_TS_NODISCARD Tree parseString(details::StringViewParameter buffer,
                                          details::OptionalParam<Tree> old_tree = {});

        /**
         * @brief Parses encoded source code provided as a contiguous string.
         * @param buffer The string buffer containing the encoded source code to parse.
         * @param encoding The character encoding format of the buffer.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns the newly generated syntax tree.
         * @throws std::logic_error If no language has been set prior to calling this method.
         * @throws std::invalid_argument If `old_tree` was parsed with a different language.
         * @throws std::length_error If the input buffer size exceeds 4GB.
         */
        CPP_TS_NODISCARD Tree parseStringEncoded(details::StringViewParameter buffer,
                                                 InputEncoding                encoding,
                                                 details::OptionalParam<Tree> old_tree = {});

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Parses source code using a custom input provider, without throwing for parse failures.
         * @param input The custom input source to read from.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @param options Optional advanced parsing options.
         * @return Returns the syntax tree, or an error: `NoLanguage`, `OldTreeLanguage` or `ParseFailed` (tree-sitter
         * returned no tree, e.g. the progress callback cancelled the parse).
         * @throws Any exception thrown by the read/decode callbacks of `input`, the progress callback of `options` or
         * the logger. Exceptions of user code are never turned into errors.
         */
        CPP_TS_NODISCARD std::expected<Tree, Error> tryParse(Input                                     &input,
                                                             details::OptionalParam<Tree>               old_tree = {},
                                                             details::OptionalParam<const ParseOptions> options  = {});

        /**
         * @brief Parses source code provided as a contiguous string, without throwing for parse failures.
         * @param buffer The string buffer containing the source code to parse.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns the syntax tree, or an error: `NoLanguage`, `InputTooLarge`, `OldTreeLanguage` or
         * `ParseFailed`.
         * @throws Any exception thrown by the logger.
         */
        CPP_TS_NODISCARD std::expected<Tree, Error> tryParseString(details::StringViewParameter buffer,
                                                                   details::OptionalParam<Tree> old_tree = {});

        /**
         * @brief Parses encoded source code provided as a contiguous string, without throwing for parse failures.
         * @param buffer The string buffer containing the encoded source code to parse.
         * @param encoding The character encoding format of the buffer.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns the syntax tree, or an error: `NoLanguage`, `InputTooLarge`, `OldTreeLanguage` or
         * `ParseFailed`.
         * @throws Any exception thrown by the logger.
         */
        CPP_TS_NODISCARD std::expected<Tree, Error> tryParseStringEncoded(details::StringViewParameter buffer,
                                                                          InputEncoding                encoding,
                                                                          details::OptionalParam<Tree> old_tree = {});
#endif // CPP_TS_HAS_EXPECTED

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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Enables generation of DOT graphs into a file descriptor without throwing.
         * @param file_descriptor The file descriptor where the DOT graph will be written, or -1 to disable.
         * @return Nothing on success, or a `SystemError` if the file descriptor fails to duplicate.
         */
        CPP_TS_NODISCARD std::expected<void, Error> tryEnableDotGraphs(int file_descriptor);
#endif // CPP_TS_HAS_EXPECTED

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
         * @param logger_func The callable object that will handle incoming log messages. An empty callable removes
         * the logger.
         *
         * @note If the logger throws, logging is suspended for the rest of the parse and the exception is rethrown by
         * the parse method once tree-sitter returns.
         */
        void setLogger(LoggerFunction logger_func);

        /// @brief Removes the currently assigned logging callback.
        void removeLogger() noexcept
        {
            if (impl_)
            {
                ts_parser_set_logger(impl_.get(), { nullptr, nullptr });
            }
            logger_.reset();
        }

        /**
         * @brief Checks whether a logging callback is assigned.
         * @return Returns true if a logger is assigned, false otherwise.
         */
        CPP_TS_NODISCARD bool hasLogger() const noexcept
        {
            return logger_ != nullptr;
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
            if (impl_)
            {
                ts_parser_reset(impl_.get());
            }
        }

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the underlying Tree-sitter parser (escape hatch to the C API).
         * @return Pointer to the parser owned by this object, or nullptr for a moved-from parser.
         *
         * @warning Ownership is retained. Changing the language or the logger directly bypasses the wrapper state.
         */
        CPP_TS_NODISCARD TSParser *raw() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_TYPES

        ////////////////////////////////////////////////////////////////
        // Private Types
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Logger callback and the exception it threw.
         * @details Heap allocated, so the payload passed to tree-sitter stays valid when the Parser is moved.
         */
        struct LoggerState
        {
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief User logging callback.
            LoggerFunction function;

            /// @brief Exception thrown by the callback, rethrown once the parse returns.
            std::exception_ptr pending_exception;

#pragma endregion
        };

#pragma endregion

#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Logger callback passed to the C API; forwards to the stored logger.
         * @param payload The `LoggerState` of the parser.
         * @param log_type Type of the message.
         * @param buffer Message.
         *
         * @note If the logger throws, logging is suspended and the exception is rethrown by the parse method.
         */
        static void loggerProxy(void *payload, TSLogType log_type, const char *buffer) noexcept;

        /**
         * @brief Shared implementation of the constructor and `create()`.
         * @param language The language to assign.
         * @return Returns success, or an `InvalidLanguage`, `LanguageRejected` or `WasmStoreRequired` failure.
         */
        details::Status init(LanguageView language) noexcept;

#if defined(CPP_TS_FEATURE_WASM)
        /**
         * @brief Shared implementation of the constructor and `create()` with a WebAssembly store.
         * @param language The WebAssembly language to assign.
         * @param store The WebAssembly store. Taken over on success, given back on failure.
         * @return Returns success, or an `InvalidLanguage` or `LanguageRejected` failure.
         */
        details::Status init(LanguageView language, WasmStore &store) noexcept;
#endif // defined(CPP_TS_FEATURE_WASM)

        /**
         * @brief Shared implementation of `setLanguage()` and `trySetLanguage()`.
         * @param language The language to assign. An invalid view clears the language.
         * @return Returns success, or a `LanguageRejected` or `WasmStoreRequired` failure.
         */
        details::Status assignLanguage(LanguageView language) noexcept;

        /**
         * @brief Shared implementation of `enableDotGraphs(int)` and `tryEnableDotGraphs()`.
         * @param file_descriptor The file descriptor where the DOT graph will be written, or -1 to disable.
         * @return Returns success, or a `SystemError` failure if the descriptor cannot be duplicated.
         */
        details::Status enableDotGraphsImpl(int file_descriptor) noexcept;

        /**
         * @brief Checks the preconditions shared by every parse method.
         * @return Returns success, or a `NoLanguage` failure.
         */
        details::Status checkCanParse() const noexcept;

        /**
         * @brief Owns the result of a parse, rethrows exceptions captured in the logger and explains a missing tree.
         * @param result Receives the parsed tree (invalid when tree-sitter returned no tree).
         * @param raw_tree Tree returned by the C API. May be null.
         * @param old_tree Old tree passed to the C API. May be null.
         * @return Returns success, or an `OldTreeLanguage` or `ParseFailed` failure.
         * @throws Any exception thrown by the logger.
         */
        details::Status finishParse(Tree &result, TSTree *raw_tree, const TSTree *old_tree);

        /**
         * @brief Shared implementation of `parse()` and `tryParse()`.
         * @param result Receives the parsed tree.
         * @param input The custom input source to read from.
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @param options Optional parsing options.
         * @return Returns success, or a `NoLanguage`, `OldTreeLanguage` or `ParseFailed` failure.
         * @throws Any exception thrown by the callbacks of `input` and `options` or by the logger.
         */
        details::Status parseImpl(Tree                                      &result,
                                  Input                                     &input,
                                  details::OptionalParam<Tree>               old_tree,
                                  details::OptionalParam<const ParseOptions> options);

        /**
         * @brief Shared implementation of the `parseString*()` and `tryParseString*()` methods.
         * @param result Receives the parsed tree.
         * @param buffer The string buffer containing the source code to parse.
         * @param encoding The encoding of the buffer, or null for `ts_parser_parse_string` (UTF-8).
         * @param old_tree An optional previous syntax tree to use for incremental parsing.
         * @return Returns success, or a `NoLanguage`, `InputTooLarge`, `OldTreeLanguage` or `ParseFailed` failure.
         * @throws Any exception thrown by the logger.
         */
        details::Status parseStringImpl(Tree                        &result,
                                        details::StringViewParameter buffer,
                                        const InputEncoding         *encoding,
                                        details::OptionalParam<Tree> old_tree);

        /**
         * @brief Completes a throwing parse method.
         * @param result Parsed tree.
         * @param status Status of the parse.
         * @return Returns the tree; a missing tree (`ParseFailed`) is returned as an invalid tree.
         * @throws The exception of `status` for every other failure.
         */
        static Tree treeOrThrow(Tree &result, const details::Status &status);

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned C API parser, or null for a moved-from parser.
        details::Owned<TSParser> impl_{};

        /// @brief Logger state, or null when no logger is set.
        std::unique_ptr<LoggerState> logger_{};

#if defined(CPP_TS_FEATURE_WASM)
        /// @brief True while the parser owns a WebAssembly store.
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
        CPP_TS_NODISCARD bool isValid() const noexcept
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
        CPP_TS_NODISCARD TreeCursor copy() const noexcept
        {
            // Copying an empty TSTreeCursor would read past its (empty) stack.
            return isValid() ? TreeCursor(impl_) : TreeCursor();
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
        CPP_TS_NODISCARD bool gotoParent() noexcept
        {
            return isValid() && ts_tree_cursor_goto_parent(&impl_);
        }

        /**
         * @brief Moves the cursor to the first child of its current node.
         * @return Returns true if the cursor successfully moved, false if the node has no children.
         */
        CPP_TS_NODISCARD bool gotoFirstChild() noexcept
        {
            return isValid() && ts_tree_cursor_goto_first_child(&impl_);
        }

        /**
         * @brief Moves the cursor to the last child of its current node.
         * @return Returns true if the cursor successfully moved, false if the node has no children.
         */
        CPP_TS_NODISCARD bool gotoLastChild() noexcept
        {
            return isValid() && ts_tree_cursor_goto_last_child(&impl_);
        }

        /**
         * @brief Moves the cursor to the next sibling of its current node.
         * @return Returns true if the cursor successfully moved, false if there is no next sibling.
         */
        CPP_TS_NODISCARD bool gotoNextSibling() noexcept
        {
            return isValid() && ts_tree_cursor_goto_next_sibling(&impl_);
        }

        /**
         * @brief Moves the cursor to the previous sibling of its current node.
         * @return Returns true if the cursor successfully moved, false if there is no previous sibling.
         */
        CPP_TS_NODISCARD bool gotoPreviousSibling() noexcept
        {
            return isValid() && ts_tree_cursor_goto_previous_sibling(&impl_);
        }

        /**
         * @brief Moves the cursor to the first child of the current node that extends beyond the given byte offset.
         * @param byte The byte offset to search for.
         * @return Returns the index of the child if found, or -1 if the cursor is invalid or no such child exists.
         */
        CPP_TS_NODISCARD std::int64_t gotoFirstChildForByte(std::uint32_t byte) noexcept
        {
            return isValid() ? ts_tree_cursor_goto_first_child_for_byte(&impl_, byte) : -1;
        }

        /**
         * @brief Moves the cursor to the first child of the current node that extends beyond the given point.
         * @param point The row and column coordinates to search for.
         * @return Returns the index of the child if found, or -1 if the cursor is invalid or no such child exists.
         */
        CPP_TS_NODISCARD std::int64_t gotoFirstChildForPoint(Point point) noexcept
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
        CPP_TS_NODISCARD Node getCurrentNode() const noexcept
        {
            return isValid() ? Node{ ts_tree_cursor_current_node(&impl_) } : Node::null();
        }

        /**
         * @brief Retrieves the field name of the current node, if any.
         * @return Returns a view to the field name, or an empty view if there is no field name.
         */
        CPP_TS_NODISCARD details::StringViewReturn getCurrentFieldName() const
        {
            return isValid() ? details::make_view(ts_tree_cursor_current_field_name(&impl_))
                             : details::StringViewReturn("");
        }

        /**
         * @brief Retrieves the field ID of the current node, if any.
         * @return Returns the field ID, or 0 if there is no field ID or if the cursor is invalid.
         */
        CPP_TS_NODISCARD FieldID getCurrentFieldID() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_field_id(&impl_) : 0;
        }

        /**
         * @brief Retrieves the internal index of the current descendant.
         * @return Returns the index of the descendant.
         */
        CPP_TS_NODISCARD std::uint32_t getCurrentDescendantIndex() const noexcept
        {
            return isValid() ? ts_tree_cursor_current_descendant_index(&impl_) : 0;
        }

        /**
         * @brief Retrieves the depth of the current node relative to the origin node.
         * @return Returns the depth level from the origin.
         */
        CPP_TS_NODISCARD std::uint32_t getDepthFromOrigin() const noexcept
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
        CPP_TS_NODISCARD bool operator==(const TreeCursor &other) const noexcept;

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Compares two tree cursors for inequality.
         * @param other The tree cursor to compare against.
         * @return Returns true if the cursors point to different nodes.
         */
        CPP_TS_NODISCARD bool operator!=(const TreeCursor &other) const noexcept
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

        /// @brief Underlying C API cursor (holds an internal stack released by `ts_tree_cursor_delete`).
        TSTreeCursor impl_{};

        /// @brief True if `impl_` was created by the C API.
        bool is_valid_ = false;

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
        explicit QueryMatch(const TSQueryMatch &match) : id(0), pattern_index(0)
        {
            assign(match);
        }

#pragma endregion

#pragma region MODIFICATION

        ////////////////////////////////////////////////////////////////
        // Modification
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Replaces the content of this match with a native C API match.
         * @param match The native `TSQueryMatch` object to copy.
         *
         * @note The capture storage is reused, so iterating with the same `QueryMatch` object does not allocate once
         * its capacity is large enough.
         */
        void assign(const TSQueryMatch &match)
        {
            id            = match.id;
            pattern_index = match.pattern_index;
            if (match.capture_count > 0 && match.captures != nullptr)
            {
                captures.assign(match.captures, match.captures + match.capture_count);
            }
            else
            {
                captures.clear();
            }
        }

#pragma endregion

#pragma region LOOKUP

        ////////////////////////////////////////////////////////////////
        // Lookup
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the first node captured with a capture index.
         * @param capture_index Index of the capture name in the query (see `Query::getCaptureIndexForName`).
         * @return The first captured node, or a null node if this match has no such capture.
         *
         * @note Quantified captures (`+`, `*`) can capture several nodes, iterate `captures` to see all of them.
         */
        CPP_TS_NODISCARD Node getNodeFor(std::uint32_t capture_index) const noexcept
        {
            for (const QueryCapture &capture : captures)
            {
                if (capture.index == capture_index)
                {
                    return capture.node;
                }
            }
            return Node::null();
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

        /**
         * @brief Creates an independent copy of another query (`ts_query_copy`).
         * @param other Query to copy. Disabled captures and patterns are copied too.
         */
        Query(const Query &other) : impl_{ other.impl_ ? ts_query_copy(other.impl_.get()) : nullptr }
        {}

        /**
         * @brief Replaces this query with an independent copy of another query.
         * @param other Query to copy.
         * @return Reference to this query.
         */
        Query &operator=(const Query &other)
        {
            if (this != &other)
            {
                impl_.reset(other.impl_ ? ts_query_copy(other.impl_.get()) : nullptr);
            }
            return *this;
        }

        /**
         * @brief Transfers ownership of a query.
         * @param other Query to move from.
         */
        Query(Query &&other) noexcept = default;

        /**
         * @brief Transfers ownership of a query.
         * @param other Query to move from.
         * @return Reference to this query.
         */
        Query &operator=(Query &&other) noexcept = default;

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Creates a query object without throwing exceptions.
         * @param language The language used to interpret the query.
         * @param source The query source code.
         * @return Returns a valid `Query`, or an error: `InvalidLanguage`, `InputTooLarge` or one of the `Query*`
         * kinds, with the byte offset of the error in `Error::offset`.
         */
        CPP_TS_NODISCARD static std::expected<Query, Error> create(LanguageView                 language,
                                                                   details::StringViewParameter source);
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region COUNTS

        ////////////////////////////////////////////////////////////////
        // Counts
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieves the total number of patterns within this query.
         * @return Returns the number of patterns.
         */
        CPP_TS_NODISCARD std::uint32_t getPatternCount() const noexcept
        {
            return ts_query_pattern_count(impl_.get());
        }

        /**
         * @brief Retrieves the total number of captures within this query.
         * @return Returns the number of captures.
         */
        CPP_TS_NODISCARD std::uint32_t getCaptureCount() const noexcept
        {
            return ts_query_capture_count(impl_.get());
        }

        /**
         * @brief Retrieves the total number of string literals defined within this query.
         * @return Returns the number of string literals.
         */
        CPP_TS_NODISCARD std::uint32_t getStringCount() const noexcept
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
        CPP_TS_NODISCARD bool isPatternRooted(std::uint32_t pattern_index) const noexcept
        {
            return ts_query_is_pattern_rooted(impl_.get(), pattern_index);
        }

        /**
         * @brief Checks if a specific pattern is non-local.
         * @param pattern_index The index of the pattern to check.
         * @return Returns true if the pattern is non-local, false otherwise.
         */
        CPP_TS_NODISCARD bool isPatternNonLocal(std::uint32_t pattern_index) const noexcept
        {
            return ts_query_is_pattern_non_local(impl_.get(), pattern_index);
        }

        /**
         * @brief Checks if a pattern is guaranteed to match at a specific byte offset.
         * @param byte_offset The byte offset to check.
         * @return Returns true if the pattern match is guaranteed.
         */
        CPP_TS_NODISCARD bool isPatternGuaranteedAtStep(std::uint32_t byte_offset) const noexcept
        {
            return ts_query_is_pattern_guaranteed_at_step(impl_.get(), byte_offset);
        }

        /**
         * @brief Retrieves the byte range representing where a pattern is defined in the source.
         * @param pattern_index The index of the pattern.
         * @return Returns an extent containing the start and end byte offsets.
         */
        CPP_TS_NODISCARD Extent<std::uint32_t> getByteRangeForPattern(std::uint32_t pattern_index) const noexcept
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
        CPP_TS_NODISCARD std::vector<QueryPredicateStep> getAllPredicatesForPattern(std::uint32_t pattern_index) const;

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
        CPP_TS_NODISCARD details::StringViewReturn getCaptureNameForID(std::uint32_t id) const noexcept
        {
            std::uint32_t length = 0;
            const char   *name   = ts_query_capture_name_for_id(impl_.get(), id, &length);
            return details::make_view(name, static_cast<std::size_t>(length));
        }

        /**
         * @brief Finds the index of a capture by its name (without the leading `@`).
         * @param name Capture name to find.
         * @return Index of the capture, or `UINT32_MAX` if the query has no capture with this name.
         *
         * @note Resolve names once, before iterating matches, and compare indices inside the loop.
         */
        CPP_TS_NODISCARD std::uint32_t getCaptureIndexForName(details::StringViewParameter name) const noexcept
        {
            const std::uint32_t count = getCaptureCount();
            for (std::uint32_t i = 0; i < count; ++i)
            {
                if (getCaptureNameForID(i) == name)
                {
                    return i;
                }
            }
            return std::numeric_limits<std::uint32_t>::max();
        }

        /**
         * @brief Retrieves the quantifier associated with a specific capture within a pattern.
         * @param pattern_index The index of the pattern containing the capture.
         * @param capture_index The index of the capture.
         * @return Returns the quantifier corresponding to the capture.
         */
        CPP_TS_NODISCARD Quantifier getCaptureQuantifierForID(std::uint32_t pattern_index,
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
        CPP_TS_NODISCARD details::StringViewReturn getStringValueForID(std::uint32_t id) const noexcept
        {
            std::uint32_t length = 0;
            const char   *name   = ts_query_string_value_for_id(impl_.get(), id, &length);
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
        CPP_TS_NODISCARD operator const TSQuery *() const noexcept
        {
            return impl_.get();
        }

        /**
         * @brief Checks whether the object owns a compiled query (false only for a moved-from query).
         * @return Returns true if a query is owned, false otherwise.
         */
        CPP_TS_NODISCARD bool isValid() const noexcept
        {
            return impl_ != nullptr;
        }

#pragma endregion

    private:
#pragma region PRIVATE_LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Private Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Takes ownership of a compiled C API query.
         * @param raw_query Query created by `ts_query_new`.
         */
        explicit Query(TSQuery *raw_query) noexcept : impl_(raw_query)
        {}

        /**
         * @brief Shared implementation of the constructor and `create()`.
         * @param language The language used to interpret the query.
         * @param source The query source code.
         * @param result Receives the compiled query on success.
         * @return Returns success, or an `InvalidLanguage`, `InputTooLarge`, `Query*` or `Unknown` failure.
         */
        static details::Status compile(LanguageView                 language,
                                       details::StringViewParameter source,
                                       details::Owned<TSQuery>     &result);

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned C API query, or null for a moved-from query.
        details::Owned<TSQuery> impl_{};

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_CURSOR

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursor
    // Executes queries and iterates over matches.
    /////////////////////////////////////////////////////////////////////////////

    class QueryMatches;
    class QueryCaptures;

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
        CPP_TS_NODISCARD std::uint32_t getMatchLimit() const noexcept
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
        CPP_TS_NODISCARD bool didExceedMatchLimit() const noexcept
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
        CPP_TS_NODISCARD bool setByteRange(Extent<std::uint32_t> range) noexcept
        {
            return ts_query_cursor_set_byte_range(impl_.get(), range.start, range.end);
        }

        /**
         * @brief Restricts the query cursor to a specific point range (row and column).
         * @param range The start and end points to restrict parsing to.
         * @return Returns true if the range was successfully set.
         */
        CPP_TS_NODISCARD bool setPointRange(Extent<Point> range) noexcept
        {
            return ts_query_cursor_set_point_range(impl_.get(), range.start, range.end);
        }

        /**
         * @brief Restricts the query cursor to matches that intersect a specific byte range.
         * @param range The containing byte range.
         * @return Returns true if the range was successfully set.
         */
        CPP_TS_NODISCARD bool setContainingByteRange(Extent<std::uint32_t> range) noexcept
        {
            return ts_query_cursor_set_containing_byte_range(impl_.get(), range.start, range.end);
        }

        /**
         * @brief Restricts the query cursor to matches that intersect a specific point range.
         * @param range The containing point range.
         * @return Returns true if the range was successfully set.
         */
        CPP_TS_NODISCARD bool setContainingPointRange(Extent<Point> range) noexcept
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
         * @throws std::invalid_argument If the query is not valid (moved-from).
         *
         * @note The progress callback is copied into the cursor: tree-sitter keeps using it for the whole execution
         * (every `nextMatch`/`nextCapture`), so the options object itself does not have to outlive this call.
         * Returning true from the callback cancels the execution (see `wasCancelled()`).
         */
        void exec(const Query &query, Node node, details::OptionalParam<const QueryCursorOptions> query_options = {});

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Starts executing a query on a specific syntax node without throwing.
         * @param query The compiled query to execute.
         * @param node The syntax tree node to begin the search from.
         * @param query_options Optional advanced configuration (such as progress callbacks).
         * @return Nothing on success, or an `InvalidArgument` error if the query is not valid (moved-from).
         */
        CPP_TS_NODISCARD std::expected<void, Error> tryExec(
                const Query                                     &query,
                Node                                             node,
                details::OptionalParam<const QueryCursorOptions> query_options = {});
#endif // CPP_TS_HAS_EXPECTED

        /**
         * @brief Checks whether the current execution was cancelled by the progress callback.
         * @return Returns true if the progress callback requested cancellation, false otherwise.
         *
         * @note When cancelled, matches already produced form a prefix of the complete result set.
         */
        CPP_TS_NODISCARD bool wasCancelled() const noexcept
        {
            return state_ && state_->cancelled;
        }

#pragma endregion

#pragma region ITERATION

        ////////////////////////////////////////////////////////////////
        // Iteration
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Advances to the next match of the currently executing query.
         * @param match A reference to a `QueryMatch` object that will be populated if a match is found.
         * @return Returns true if a new match was found, false if there are no more matches.
         * @throws Any exception thrown by the progress callback of the current execution.
         *
         * @note Reusing the same `QueryMatch` object across calls reuses its capture storage (no allocation per match).
         */
        CPP_TS_NODISCARD bool nextMatch(QueryMatch &match)
        {
            TSQueryMatch c_match{};
            const bool   found = ts_query_cursor_next_match(impl_.get(), &c_match);
            rethrowPendingException();
            if (found)
            {
                match.assign(c_match);
            }
            return found;
        }

        /**
         * @brief Advances to the next individual capture of the currently executing query.
         * @param match A reference to a `QueryMatch` object that will be populated.
         * @param capture_index A reference to a variable that will receive the index of the specific capture within the
         * match.
         * @return Returns true if a new capture was found, false if there are no more captures.
         * @throws Any exception thrown by the progress callback of the current execution.
         */
        CPP_TS_NODISCARD bool nextCapture(QueryMatch &match, std::uint32_t &capture_index)
        {
            TSQueryMatch  c_match{};
            std::uint32_t c_capture_index = 0;

            const bool found = ts_query_cursor_next_capture(impl_.get(), &c_match, &c_capture_index);
            rethrowPendingException();
            if (found)
            {
                match.assign(c_match);
                capture_index = c_capture_index;
            }
            return found;
        }

        /**
         * @brief Removes a currently executing match from the cursor.
         * @param match_id The ID of the match to remove.
         */
        void removeMatch(std::uint32_t match_id) noexcept
        {
            ts_query_cursor_remove_match(impl_.get(), match_id);
        }

        /**
         * @brief Returns a single-pass range over the remaining matches of the current execution.
         * @return Range of `QueryMatch` objects. Each step reuses the same storage, so copy a match to keep it.
         *
         * @code
         * cursor.exec(query, root);
         * for (const ts::QueryMatch &match : cursor.matches()) { ... }
         * @endcode
         */
        CPP_TS_NODISCARD QueryMatches matches() noexcept;

        /**
         * @brief Returns a single-pass range over the remaining captures of the current execution, in source order.
         * @return Range of `QueryCaptureEntry` objects. Each step reuses the same storage.
         */
        CPP_TS_NODISCARD QueryCaptures captures() noexcept;

#pragma endregion

#pragma region CONVERSION

        ////////////////////////////////////////////////////////////////
        // Conversion
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the underlying Tree-sitter query cursor (escape hatch to the C API).
         * @return Pointer to the cursor owned by this object.
         */
        CPP_TS_NODISCARD TSQueryCursor *raw() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_TYPES

        ////////////////////////////////////////////////////////////////
        // Private Types
        ////////////////////////////////////////////////////////////////

        /**
         * @brief State referenced by tree-sitter during an execution with options.
         * @details Heap allocated, so it stays at the same address when the cursor is moved.
         */
        struct ExecState
        {
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Options handed to the C API for the whole execution.
            TSQueryCursorOptions raw_options{};

            /// @brief Copy of the progress callback of the execution.
            QueryCursorOptions::ProgressCallbackFunction progress{};

            /// @brief Exception thrown by the callback, rethrown by `nextMatch` / `nextCapture`.
            std::exception_ptr pending_exception{};

            /// @brief True once the callback cancelled the execution.
            bool cancelled = false;

#pragma endregion
        };

#pragma endregion

#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Progress callback passed to the C API; forwards to the callback of the execution.
         * @param state State of the execution. Its payload is the `ExecState` of the cursor.
         * @return Returns true to cancel the execution (also when the callback throws: the exception is stored and
         * rethrown by `nextMatch` / `nextCapture`).
         */
        static bool progressProxy(TSQueryCursorState *state) noexcept;

        /**
         * @brief Shared implementation of `exec()` and `tryExec()`.
         * @param query The compiled query to execute.
         * @param node The syntax tree node to begin the search from.
         * @param query_options Optional configuration (progress callback).
         * @return Returns success, or an `InvalidArgument` failure for an invalid query.
         */
        details::Status execImpl(const Query                                     &query,
                                 Node                                             node,
                                 details::OptionalParam<const QueryCursorOptions> query_options);

        /**
         * @brief Rethrows (and clears) the exception thrown by the progress callback, if any.
         * @throws Any exception thrown by the progress callback.
         */
        void rethrowPendingException()
        {
            if (state_)
            {
                details::rethrow_if_set(state_->pending_exception);
            }
        }

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned C API query cursor.
        details::Owned<TSQueryCursor> impl_{};

        /// @brief State of the current execution with options, or null.
        std::unique_ptr<ExecState> state_{};

#pragma endregion
    };

#pragma endregion

#pragma region QUERY_RESULT_RANGES

    /////////////////////////////////////////////////////////////////////////////
    // Query Result Ranges
    // Single-pass ranges over the results of a QueryCursor execution.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A capture produced while iterating captures in source order.
    struct QueryCaptureEntry
    {
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The match containing the capture.
        QueryMatch match;

        /// @brief Index of the capture inside `match.captures`.
        std::uint32_t capture_index = 0;

#pragma endregion

#pragma region ACCESSORS

        ////////////////////////////////////////////////////////////////
        // Accessors
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the capture this entry refers to.
         * @return Reference to the capture inside `match`.
         */
        CPP_TS_NODISCARD const QueryCapture &capture() const noexcept
        {
            return match.captures[capture_index];
        }

#pragma endregion
    };

    namespace details
    {
        /**
         * @brief Single-pass iterator pulling results out of a QueryCursor.
         * @tparam T_Value Result type stored in the iterator (reused for every step).
         * @tparam T_Step Policy fetching the next result: `static bool next(QueryCursor &, T_Value &)`.
         */
        template <typename T_Value, typename T_Step>
        class QueryResultIterator
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            /// @brief Type of the results.
            using value_type = T_Value;

            /// @brief Difference type (required by the iterator concepts, never used).
            using difference_type = std::ptrdiff_t;

            /// @brief Pointer to the current result.
            using pointer = const T_Value *;

            /// @brief Reference to the current result.
            using reference = const T_Value &;

            /// @brief Iterator category (single-pass).
            using iterator_category = std::input_iterator_tag;

#if CPP_TS_HAS_RANGES
            /// @brief Iterator concept (single-pass).
            using iterator_concept = std::input_iterator_tag;
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Creates an end iterator.
            QueryResultIterator() noexcept : cursor_(nullptr), value_(), at_end_(true)
            {}

            /**
             * @brief Creates an iterator positioned at the next result of a cursor.
             * @param cursor Cursor whose current execution is iterated.
             */
            explicit QueryResultIterator(QueryCursor &cursor) : cursor_(&cursor), value_(), at_end_(false)
            {
                advance();
            }

#pragma endregion

#pragma region GET

            ////////////////////////////////////////////////////////////////
            // Get
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Accesses the current result.
             * @return Reference to the current result.
             */
            CPP_TS_NODISCARD reference operator*() const noexcept
            {
                return value_;
            }

            /**
             * @brief Accesses the current result.
             * @return Pointer to the current result.
             */
            CPP_TS_NODISCARD pointer operator->() const noexcept
            {
                return &value_;
            }

#pragma endregion

#pragma region ADVANCE

            ////////////////////////////////////////////////////////////////
            // Advance
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Fetches the next result.
             * @return Reference to this iterator.
             * @throws Any exception thrown by the progress callback of the execution.
             */
            QueryResultIterator &operator++()
            {
                advance();
                return *this;
            }

            /**
             * @brief Fetches the next result (single-pass iterators return nothing from postfix increment).
             * @throws Any exception thrown by the progress callback of the execution.
             */
            void operator++(int)
            {
                advance();
            }

#pragma endregion

#pragma region COMPARISON

            ////////////////////////////////////////////////////////////////
            // Comparison
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Compares two iterators (single-pass: only "both at the end" is meaningful).
             * @param lhs First iterator.
             * @param rhs Second iterator.
             * @return Returns true if both iterators are at the end, or both are not.
             */
            CPP_TS_NODISCARD friend bool operator==(const QueryResultIterator &lhs,
                                                    const QueryResultIterator &rhs) noexcept
            {
                return lhs.at_end_ == rhs.at_end_;
            }

#if !CPP_TS_HAS_CXX20
            /**
             * @brief Compares two iterators.
             * @param lhs First iterator.
             * @param rhs Second iterator.
             * @return Returns true if exactly one of the iterators is at the end.
             */
            CPP_TS_NODISCARD friend bool operator!=(const QueryResultIterator &lhs,
                                                    const QueryResultIterator &rhs) noexcept
            {
                return !(lhs == rhs);
            }
#endif // !CPP_TS_HAS_CXX20

#if CPP_TS_HAS_RANGES
            /**
             * @brief Compares an iterator with the end sentinel.
             * @param it Iterator.
             * @param sentinel The end sentinel.
             * @return Returns true if the iterator is at the end.
             */
            CPP_TS_NODISCARD friend bool operator==(const QueryResultIterator &it,
                                                    CPP_TS_MAYBE_UNUSED std::default_sentinel_t sentinel) noexcept
            {
                return it.at_end_;
            }
#endif // CPP_TS_HAS_RANGES

#pragma endregion

        private:
#pragma region PRIVATE_METHODS

            ////////////////////////////////////////////////////////////////
            // Private Methods
            ////////////////////////////////////////////////////////////////

            /// @brief Fetches the next result, or moves to the end.
            void advance()
            {
                if (!at_end_)
                {
                    at_end_ = cursor_ == nullptr || !T_Step::next(*cursor_, value_);
                }
            }

#pragma endregion

#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Iterated cursor, or null for an end iterator.
            QueryCursor *cursor_;

            /// @brief Current result (its storage is reused by every step).
            T_Value value_;

            /// @brief True once the execution is finished.
            bool at_end_;

#pragma endregion
        };

        /// @brief Step policy of `QueryMatches`: fetches the next match.
        struct MatchStep
        {
#pragma region ADVANCE

            ////////////////////////////////////////////////////////////////
            // Advance
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Fetches the next match.
             * @param cursor Iterated cursor.
             * @param value Receives the match.
             * @return Returns true if a match was found.
             */
            static bool next(QueryCursor &cursor, QueryMatch &value)
            {
                return cursor.nextMatch(value);
            }

#pragma endregion
        };

        /// @brief Step policy of `QueryCaptures`: fetches the next capture in source order.
        struct CaptureStep
        {
#pragma region ADVANCE

            ////////////////////////////////////////////////////////////////
            // Advance
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Fetches the next capture.
             * @param cursor Iterated cursor.
             * @param value Receives the match and the index of the capture.
             * @return Returns true if a capture was found.
             */
            static bool next(QueryCursor &cursor, QueryCaptureEntry &value)
            {
                return cursor.nextCapture(value.match, value.capture_index);
            }

#pragma endregion
        };

        /**
         * @brief Single-pass range over the results of a QueryCursor execution.
         * @tparam T_Iterator Iterator type of the range.
         */
        template <typename T_Iterator>
        class QueryResultRange
#if CPP_TS_HAS_RANGES
            : public std::ranges::view_interface<QueryResultRange<T_Iterator>>
#endif // CPP_TS_HAS_RANGES
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            /// @brief Iterator type of the range.
            using iterator = T_Iterator;

#if CPP_TS_HAS_RANGES
            /// @brief End sentinel type of the range.
            using sentinel = std::default_sentinel_t;
#else
            /// @brief End sentinel type of the range (an end iterator).
            using sentinel = T_Iterator;
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Creates an empty range.
            QueryResultRange() noexcept : cursor_(nullptr)
            {}

            /**
             * @brief Creates a range over the current execution of a cursor.
             * @param cursor Iterated cursor. Must outlive the range.
             */
            explicit QueryResultRange(QueryCursor &cursor) noexcept : cursor_(&cursor)
            {}

#pragma endregion

#pragma region ITERATION

            ////////////////////////////////////////////////////////////////
            // Iteration
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Fetches the first remaining result. Calling it again continues from the current position.
             * @return Iterator positioned at the next result.
             */
            CPP_TS_NODISCARD iterator begin() const
            {
                return cursor_ ? iterator(*cursor_) : iterator();
            }

            /**
             * @brief Returns the end of the range.
             * @return End sentinel.
             */
            CPP_TS_NODISCARD sentinel end() const noexcept
            {
                return sentinel();
            }

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Iterated cursor, or null for an empty range.
            QueryCursor *cursor_;

#pragma endregion
        };
    } // namespace details

    /// @brief Single-pass range over the matches of a QueryCursor execution.
    class QueryMatches : public details::QueryResultRange<details::QueryResultIterator<QueryMatch, details::MatchStep>>
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructors of `details::QueryResultRange`.
        using details::QueryResultRange<details::QueryResultIterator<QueryMatch, details::MatchStep>>::QueryResultRange;

#pragma endregion
    };

    /// @brief Single-pass range over the captures (in source order) of a QueryCursor execution.
    class QueryCaptures
        : public details::QueryResultRange<details::QueryResultIterator<QueryCaptureEntry, details::CaptureStep>>
    {
    public:
#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructors of `details::QueryResultRange`.
        using details::QueryResultRange<
                details::QueryResultIterator<QueryCaptureEntry, details::CaptureStep>>::QueryResultRange;

#pragma endregion
    };

    inline QueryMatches QueryCursor::matches() noexcept
    {
        return QueryMatches(*this);
    }

    inline QueryCaptures QueryCursor::captures() noexcept
    {
        return QueryCaptures(*this);
    }

#if CPP_TS_HAS_RANGES
    static_assert(std::input_iterator<QueryMatches::iterator>, "QueryMatches iterator must be an input iterator");
    static_assert(std::ranges::input_range<QueryMatches>, "QueryMatches must be an input range");
    static_assert(std::ranges::input_range<QueryCaptures>, "QueryCaptures must be an input range");
#endif // CPP_TS_HAS_RANGES

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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Creates a lookahead iterator without throwing.
         * @param language The language to be used by the iterator.
         * @param state The parse state to iterate over.
         * @return The iterator, or an error: `InvalidLanguage` or `InvalidParseState`.
         */
        CPP_TS_NODISCARD static std::expected<LookaheadIterator, Error> create(LanguageView language, StateID state);
#endif // CPP_TS_HAS_EXPECTED

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
        CPP_TS_NODISCARD bool resetState(StateID state) noexcept
        {
            return ts_lookahead_iterator_reset_state(impl_.get(), state);
        }

        /**
         * @brief Resets the iterator to a new language and parse state.
         * @param language The new language to use.
         * @param state The new parse state to iterate over.
         * @return Returns true if the language and state were successfully reset, false otherwise.
         */
        CPP_TS_NODISCARD bool reset(LanguageView language, StateID state) noexcept
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
        CPP_TS_NODISCARD LanguageView getLanguage() const noexcept
        {
            return LanguageView(ts_lookahead_iterator_language(impl_.get()));
        }

        /**
         * @brief Retrieves the symbol the iterator is currently pointing at.
         * @return Returns the current symbol.
         */
        CPP_TS_NODISCARD Symbol getCurrentSymbol() const noexcept
        {
            return ts_lookahead_iterator_current_symbol(impl_.get());
        }

        /**
         * @brief Retrieves the name of the current symbol.
         * @return Returns a string view of the symbol's name.
         */
        CPP_TS_NODISCARD details::StringViewReturn getCurrentSymbolName() const
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
        CPP_TS_NODISCARD bool next() noexcept
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
        CPP_TS_NODISCARD operator const TSLookaheadIterator *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Private Lifecycle
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Takes ownership of a C API lookahead iterator.
         * @param raw_iterator Iterator created by `ts_lookahead_iterator_new`.
         */
        explicit LookaheadIterator(TSLookaheadIterator *raw_iterator) noexcept : impl_(raw_iterator)
        {}

        /**
         * @brief Shared implementation of the constructor and `create()`.
         * @param language The language to be used by the iterator.
         * @param state The parse state to iterate over.
         * @param result Receives the iterator on success.
         * @return Returns success, or an `InvalidLanguage` or `InvalidParseState` failure.
         */
        static details::Status make(LanguageView                         language,
                                    StateID                              state,
                                    details::Owned<TSLookaheadIterator> &result) noexcept;

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned C API lookahead iterator, or null for a moved-from iterator.
        details::Owned<TSLookaheadIterator> impl_{};

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
        explicit WasmStore(TSWasmStore *store) noexcept : impl_{ store }
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

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Creates a WebAssembly store without throwing exceptions.
         * @param engine A pointer to the WebAssembly engine.
         * @return Returns a valid `WasmStore`, or an error: one of the `Wasm*` kinds or `Unknown`.
         */
        CPP_TS_NODISCARD static std::expected<WasmStore, Error> create(WasmEngine *engine);
#endif // CPP_TS_HAS_EXPECTED

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
        CPP_TS_NODISCARD Language loadLanguage(details::StringViewParameter name,
                                               details::StringViewParameter wasm_buffer);

        /**
         * @brief Loads a language from a WebAssembly file on disk.
         * @param file_path The path to the WebAssembly file.
         * @return Returns a loaded `Language` object.
         * @throws std::logic_error If the store is currently invalid.
         * @throws std::length_error If the file path exceeds 4GB.
         * @throws std::runtime_error If the file cannot be read or the language fails to load.
         */
        CPP_TS_NODISCARD Language loadLanguage(details::StringViewParameter file_path);

        /**
         * @brief Retrieves the number of languages currently loaded into this store.
         * @return Returns the total count of loaded languages.
         * @throws std::logic_error If the store is currently invalid.
         */
        CPP_TS_NODISCARD std::size_t getLanguageCount() const;

#if CPP_TS_HAS_EXPECTED
        /**
         * @brief Loads a language from a WebAssembly buffer without throwing exceptions.
         * @param name The name of the language.
         * @param wasm_buffer The buffer containing the compiled WebAssembly grammar.
         * @return Returns the loaded `Language`, or an error: `InvalidObject` (invalid store), `InputTooLarge` or one
         * of the `Wasm*` kinds.
         */
        CPP_TS_NODISCARD std::expected<Language, Error> tryLoadLanguage(details::StringViewParameter name,
                                                                        details::StringViewParameter wasm_buffer);

        /**
         * @brief Loads a language from a WebAssembly file on disk without throwing exceptions.
         * @param file_path The path to the WebAssembly file.
         * @return Returns the loaded `Language`, or an error: `InvalidObject` (invalid store), `InputTooLarge`,
         * `FileError` or one of the `Wasm*` kinds.
         */
        CPP_TS_NODISCARD std::expected<Language, Error> tryLoadLanguage(details::StringViewParameter file_path);

        /**
         * @brief Retrieves the number of languages currently loaded into this store without throwing.
         * @return Returns the total count of loaded languages, or an `InvalidObject` error for an invalid store.
         */
        CPP_TS_NODISCARD std::expected<std::size_t, Error> tryGetLanguageCount() const;
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma region FLAGS

        //////////////////////////////////////////////////////////////
        // Flags
        //////////////////////////////////////////////////////////////

        /**
         * @brief Checks if the WebAssembly store is valid and ready to use.
         * @return Returns true if the store is valid, false otherwise.
         */
        CPP_TS_NODISCARD bool isValid() const noexcept
        {
            return impl_ != nullptr;
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
        CPP_TS_NODISCARD operator TSWasmStore *() const noexcept
        {
            return impl_.get();
        }

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /// @brief Forgets the store without deleting it (ownership was transferred to a parser).
        void setInvalid() noexcept
        {
            // We need to set impl as nullptr but don't call deleter on stored pointer.
            static_cast<void>(impl_.release());
        }

        /**
         * @brief Shared implementation of the constructor and `create()`.
         * @param engine A pointer to the WebAssembly engine.
         * @param result Receives the store on success.
         * @return Returns success, or a `Wasm*` or `Unknown` failure.
         */
        static details::Status make(WasmEngine *engine, details::Owned<TSWasmStore> &result);

        /**
         * @brief Checks that the store can be used.
         * @return Returns success, or an `InvalidObject` failure for an invalid store.
         */
        details::Status checkValid() const noexcept;

        /**
         * @brief Shared implementation of `loadLanguage()` and `tryLoadLanguage()` for a buffer.
         * @param name The name of the language.
         * @param wasm_buffer The buffer containing the compiled WebAssembly grammar.
         * @param result Receives the loaded language.
         * @return Returns success, or an `InvalidObject`, `InputTooLarge` or `Wasm*` failure.
         */
        details::Status loadImpl(details::StringViewParameter name,
                                 details::StringViewParameter wasm_buffer,
                                 Language                    &result);

        /**
         * @brief Shared implementation of `loadLanguage()` and `tryLoadLanguage()` for a file.
         * @param file_path The path to the WebAssembly file.
         * @param result Receives the loaded language.
         * @return Returns success, or an `InvalidObject`, `InputTooLarge`, `FileError` or `Wasm*` failure.
         */
        details::Status loadFileImpl(details::StringViewParameter file_path, Language &result);

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Owned C API store, or null once a parser took it over.
        details::Owned<TSWasmStore> impl_{};

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

    /// @brief Selects which children are produced by a children range.
    enum class ChildScope : std::uint8_t
    {
        All,      ///< Every child, named and anonymous.
        NamedOnly ///< Only named children.
    };

#if CPP_TS_HAS_CXX17
    /// @brief A sentinel type used to indicate the end of a node iteration.
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

        /// @brief Type of the produced values.
        using value_type = Node;

        /// @brief Difference type (required by the iterator concepts).
        using difference_type = std::ptrdiff_t;

        /// @brief Nodes are produced by value.
        using reference = Node;

        /// @brief Iterator category.
        using iterator_category = std::input_iterator_tag;

#if CPP_TS_HAS_RANGES
        /// @brief Iterator concept.
        using iterator_concept = std::input_iterator_tag;
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region INNER_CLASSES

        ////////////////////////////////////////////////////////////////
        // Inner Classes
        ////////////////////////////////////////////////////////////////

        /// @brief A proxy object representing a child node during iteration.
        class Child
        {
        public:
#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Constructs a child proxy object.
             * @param v The syntax node to wrap.
             */
            explicit Child(value_type v) noexcept : value_(v)
            {}

#pragma endregion

#pragma region OPERATORS

            ////////////////////////////////////////////////////////////////
            // Operators
            ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_DEDUCING_THIS
            /**
             * @brief Accesses the underlying syntax node.
             * @tparam Self The deduced type of the calling object.
             * @param self The calling object instance.
             * @return Returns a pointer to the syntax node.
             */
            template <typename Self>
            CPP_TS_NODISCARD auto *operator->(this Self &&self) noexcept
            {
                return &self.value_;
            }
#else
            /**
             * @brief Accesses the underlying syntax node (const).
             * @return Returns a const pointer to the syntax node.
             */
            CPP_TS_NODISCARD const value_type *operator->() const noexcept
            {
                return &value_;
            }

            /**
             * @brief Accesses the underlying syntax node.
             * @return Returns a pointer to the syntax node.
             */
            CPP_TS_NODISCARD value_type *operator->() noexcept
            {
                return &value_;
            }
#endif // CPP_TS_HAS_DEDUCING_THIS

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Node exposed through `operator->`.
            value_type value_;

#pragma endregion
        };

        /// @brief A proxy object used to support postfix increment and decrement operators.
        class PostfixProxy
        {
        public:
#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Constructs a postfix proxy object.
             * @param v The syntax node to preserve.
             */
            explicit PostfixProxy(value_type v) noexcept : value_(v)
            {}

#pragma endregion

#pragma region OPERATORS

            ////////////////////////////////////////////////////////////////
            // Operators
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Dereferences the proxy to get the underlying node.
             * @return Returns the preserved syntax node.
             */
            CPP_TS_NODISCARD value_type operator*() const noexcept
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

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief Node the iterator pointed to before the increment.
            value_type value_;

#pragma endregion
        };

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty iterator pointing to the end.
        ChildIterator() noexcept : cursor_(), atEnd_(true), scope_(ChildScope::All)
        {}

        /**
         * @brief Constructs an iterator for traversing the children of a specific node.
         * @param node The parent node whose children will be iterated.
         * @param scope Which children are produced.
         */
        explicit ChildIterator(const Node &node, ChildScope scope = ChildScope::All) noexcept
            : cursor_(node.getCursor()), atEnd_(node.isNull() || !cursor_.gotoFirstChild()), scope_(scope)
        {
            skipForward();
        }

        /**
         * @brief Deleted copy constructor.
         * @param other Another child iterator.
         *
         * @note The iterator owns a tree cursor, copy it explicitly through the parent node if needed.
         */
        ChildIterator(const ChildIterator &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another child iterator.
         * @return Returns a reference to this iterator.
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
        CPP_TS_NODISCARD value_type get() const noexcept
        {
            return cursor_.getCurrentNode();
        }

        /**
         * @brief Dereferences the iterator to get the current child node.
         * @return Returns the current syntax node.
         */
        CPP_TS_NODISCARD value_type operator*() const noexcept
        {
            return get();
        }

        /**
         * @brief Accesses the current child node.
         * @return Returns a proxy object to the current syntax node.
         */
        Child operator->() const noexcept
        {
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
                skipForward();
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
         *
         * @note Decrementing an iterator that reached the end moves it back to the last produced child.
         */
        ChildIterator &operator--() noexcept
        {
            if (atEnd_)
            {
                atEnd_ = !cursor_.isValid();
            }
            else
            {
                static_cast<void>(cursor_.gotoPreviousSibling());
            }
            skipBackward();
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

#pragma region SENTINELS_COMPARISON

        ////////////////////////////////////////////////////////////////
        // Sentinels Comparison
        ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CXX17
        /**
         * @brief Checks if the iterator has reached the end sentinel.
         * @param a The child iterator.
         * @param b The end sentinel.
         * @return Returns true if the iterator is at the end.
         */
        CPP_TS_NODISCARD friend bool operator==(const ChildIterator                             &a,
                                                CPP_TS_MAYBE_UNUSED const ChildIteratorSentinel &b) noexcept
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
        CPP_TS_NODISCARD friend bool operator!=(const ChildIterator &a, const ChildIteratorSentinel &b) noexcept
        {
            return !(a == b);
        }

        /**
         * @brief Checks if the sentinel has been reached by the iterator.
         * @param a The end sentinel.
         * @param b The child iterator.
         * @return Returns true if the iterator is at the end.
         */
        CPP_TS_NODISCARD friend bool operator==(const ChildIteratorSentinel &a, const ChildIterator &b) noexcept
        {
            return b == a;
        }

        /**
         * @brief Checks if the sentinel has not been reached by the iterator.
         * @param a The end sentinel.
         * @param b The child iterator.
         * @return Returns true if the iterator is not at the end.
         */
        CPP_TS_NODISCARD friend bool operator!=(const ChildIteratorSentinel &a, const ChildIterator &b) noexcept
        {
            return b != a;
        }
#endif // !CPP_TS_HAS_CXX20
#endif // CPP_TS_HAS_CXX17

#if CPP_TS_HAS_RANGES
        /**
         * @brief Checks if the iterator has reached the end (`std::default_sentinel`).
         * @param a The child iterator.
         * @param b The end sentinel.
         * @return Returns true if the iterator is at the end.
         */
        CPP_TS_NODISCARD friend bool operator==(const ChildIterator &a,
                                                CPP_TS_MAYBE_UNUSED std::default_sentinel_t b) noexcept
        {
            return a.atEnd_;
        }
#endif // CPP_TS_HAS_RANGES

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
        CPP_TS_NODISCARD friend bool operator==(const ChildIterator &a, const ChildIterator &b) noexcept
        {
            if (a.atEnd_ || b.atEnd_)
            {
                return a.atEnd_ == b.atEnd_;
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
        CPP_TS_NODISCARD friend bool operator!=(const ChildIterator &a, const ChildIterator &b) noexcept
        {
            return !(a == b);
        }
#endif // !CPP_TS_HAS_CXX20

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether the current child is skipped by the scope of the iterator.
         * @return Returns true for an anonymous child of a `ChildScope::NamedOnly` iterator.
         */
        CPP_TS_NODISCARD bool skipped() const noexcept
        {
            return scope_ == ChildScope::NamedOnly && !cursor_.getCurrentNode().isNamed();
        }

        /// @brief Moves forward to the next child produced by the scope (or to the end).
        void skipForward() noexcept
        {
            while (!atEnd_ && skipped())
            {
                atEnd_ = !cursor_.gotoNextSibling();
            }
        }

        /// @brief Moves backward to the previous child produced by the scope (stays on the first child).
        void skipBackward() noexcept
        {
            while (!atEnd_ && skipped() && cursor_.gotoPreviousSibling())
            {
            }
        }

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Cursor positioned on the current child.
        TreeCursor cursor_;

        /// @brief True past the last child.
        bool atEnd_ = false;

        /// @brief Which children are produced.
        ChildScope scope_ = ChildScope::All;

#pragma endregion
    };

    /**
     * @brief A lazy view over the children of a node, suitable for range-based for loops.
     * @details The parent node is held by value, so the view is cheap to copy and never dangles while the tree is
     * alive. In C++20 it is a `std::ranges::view` and composes with `<ranges>` adaptors.
     */
    struct Children
#if CPP_TS_HAS_RANGES
        : public std::ranges::view_interface<Children>
#endif // CPP_TS_HAS_RANGES
    {
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        /// @brief Iterator type of the view.
        using iterator = ChildIterator;

#if CPP_TS_HAS_CXX17
        /// @brief End sentinel type of the view.
        using sentinel = ChildIteratorSentinel;
#else
        /// @brief End sentinel type of the view (an end iterator).
        using sentinel = ChildIterator;
#endif // CPP_TS_HAS_CXX17

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief The parent node whose children are being viewed.
        Node node_ref;

        /// @brief Which children are produced.
        ChildScope scope = ChildScope::All;

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty view (required by `std::ranges::view` in some standard libraries).
        Children() noexcept : node_ref(), scope(ChildScope::All)
        {}

        /**
         * @brief Constructs a children view for a given parent node.
         * @param n The parent node.
         * @param child_scope Which children are produced.
         */
        explicit Children(const Node &n, ChildScope child_scope = ChildScope::All) noexcept
            : node_ref(n), scope(child_scope)
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
        CPP_TS_NODISCARD iterator begin() const noexcept
        {
            return iterator(node_ref, scope);
        }

        /**
         * @brief Returns a sentinel indicating the end of the children sequence.
         * @return Returns the end sentinel (or an end iterator for older C++ standards).
         */
        CPP_TS_NODISCARD sentinel end() const noexcept
        {
            return sentinel();
        }

        /**
         * @brief Retrieves the number of children produced by the view.
         * @return Returns the child count (named child count for `ChildScope::NamedOnly`).
         */
        CPP_TS_NODISCARD std::size_t size() const noexcept
        {
            return static_cast<std::size_t>(scope == ChildScope::NamedOnly ? node_ref.getNamedChildCount()
                                                                           : node_ref.getChildCount());
        }

        /**
         * @brief Checks whether the view produces no children.
         * @return Returns true if there is no child to produce.
         */
        CPP_TS_NODISCARD bool empty() const noexcept
        {
            return size() == 0;
        }

#pragma endregion
    };

    inline Children Node::getChildren() const noexcept
    {
        return Children(*this, ChildScope::All);
    }

    inline Children Node::getNamedChildren() const noexcept
    {
        return Children(*this, ChildScope::NamedOnly);
    }

#if CPP_TS_HAS_RANGES
    static_assert(std::input_iterator<ChildIterator>, "ChildIterator must be an input iterator");
    static_assert(std::sentinel_for<ChildIteratorSentinel, ChildIterator>, "invalid ChildIterator sentinel");
    static_assert(std::ranges::input_range<Children>, "Children must be an input range");
    static_assert(std::ranges::view<Children>, "Children must be a view");
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region PREORDER

    /////////////////////////////////////////////////////////////////////////////
    // Preorder
    // Lazy depth-first (pre-order) range over a subtree.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Single-pass iterator walking a subtree in pre-order with a tree cursor (no recursion, no allocation).
    class PreorderIterator
    {
    public:
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        /// @brief Type of the produced values.
        using value_type = Node;

        /// @brief Difference type (required by the iterator concepts).
        using difference_type = std::ptrdiff_t;

        /// @brief Nodes are produced by value.
        using reference = Node;

        /// @brief Iterator category (single-pass).
        using iterator_category = std::input_iterator_tag;

#if CPP_TS_HAS_RANGES
        /// @brief Iterator concept (single-pass).
        using iterator_concept = std::input_iterator_tag;
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an end iterator.
        PreorderIterator() noexcept : cursor_(), atEnd_(true)
        {}

        /**
         * @brief Constructs an iterator positioned at the root of a subtree.
         * @param root Root of the walked subtree.
         */
        explicit PreorderIterator(const Node &root) noexcept : cursor_(root.getCursor()), atEnd_(root.isNull())
        {}

        /**
         * @brief Deleted copy constructor.
         * @param other Another iterator.
         *
         * @note The iterator owns a tree cursor; a copy would not be independent.
         */
        PreorderIterator(const PreorderIterator &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another iterator.
         * @return Returns a reference to this iterator.
         */
        PreorderIterator &operator=(const PreorderIterator &other) = delete;

        /**
         * @brief Move constructor.
         * @param other Another iterator to move from.
         */
        PreorderIterator(PreorderIterator &&other) noexcept = default;

        /**
         * @brief Move assignment operator.
         * @param other Another iterator to move from.
         * @return Returns a reference to this iterator.
         */
        PreorderIterator &operator=(PreorderIterator &&other) noexcept = default;

#pragma endregion

#pragma region GET

        ////////////////////////////////////////////////////////////////
        // Get
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the current node.
         * @return The current node.
         */
        CPP_TS_NODISCARD value_type operator*() const noexcept
        {
            return cursor_.getCurrentNode();
        }

        /**
         * @brief Returns the depth of the current node relative to the root of the walk.
         * @return Depth, `0` for the root.
         */
        CPP_TS_NODISCARD std::uint32_t depth() const noexcept
        {
            return cursor_.getDepthFromOrigin();
        }

#pragma endregion

#pragma region ADVANCE

        ////////////////////////////////////////////////////////////////
        // Advance
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Moves to the next node in pre-order.
         * @return Reference to this iterator.
         */
        PreorderIterator &operator++() noexcept
        {
            advance();
            return *this;
        }

        /// @brief Moves to the next node in pre-order (single-pass iterators return nothing from postfix increment).
        void operator++(int) noexcept
        {
            advance();
        }

        /**
         * @brief Makes the next increment skip the children of the current node (it goes to the next sibling, or
         * upwards when there is none).
         */
        void skipChildren() noexcept
        {
            skipChildren_ = true;
        }

#pragma endregion

#pragma region COMPARISON

        ////////////////////////////////////////////////////////////////
        // Comparison
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Compares two iterators.
         * @param a First iterator.
         * @param b Second iterator.
         * @return Returns true if both are at the end, or both point to the same position.
         */
        CPP_TS_NODISCARD friend bool operator==(const PreorderIterator &a, const PreorderIterator &b) noexcept
        {
            return (a.atEnd_ || b.atEnd_) ? a.atEnd_ == b.atEnd_ : a.cursor_ == b.cursor_;
        }

#if !CPP_TS_HAS_CXX20
        /**
         * @brief Compares two iterators.
         * @param a First iterator.
         * @param b Second iterator.
         * @return Returns true if the iterators differ.
         */
        CPP_TS_NODISCARD friend bool operator!=(const PreorderIterator &a, const PreorderIterator &b) noexcept
        {
            return !(a == b);
        }
#endif // !CPP_TS_HAS_CXX20

#if CPP_TS_HAS_RANGES
        /**
         * @brief Compares an iterator with the end sentinel.
         * @param a Iterator.
         * @param b The end sentinel.
         * @return Returns true if the walk is finished.
         */
        CPP_TS_NODISCARD friend bool operator==(const PreorderIterator &a,
                                                CPP_TS_MAYBE_UNUSED std::default_sentinel_t b) noexcept
        {
            return a.atEnd_;
        }
#endif // CPP_TS_HAS_RANGES

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        ////////////////////////////////////////////////////////////////
        // Private Methods
        ////////////////////////////////////////////////////////////////

        /// @brief Moves to the first child (unless skipped), otherwise to the next node after the current subtree.
        void advance() noexcept
        {
            if (atEnd_)
            {
                return;
            }

            const bool skip = skipChildren_;
            skipChildren_   = false;
            if (!skip && cursor_.gotoFirstChild())
            {
                return;
            }
            ascend();
        }

        /// @brief Moves to the next sibling of the closest ancestor that has one, or to the end.
        void ascend() noexcept
        {
            while (!atEnd_ && !cursor_.gotoNextSibling())
            {
                atEnd_ = !cursor_.gotoParent();
            }
        }

#pragma endregion

#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Cursor positioned on the current node; it never leaves the walked subtree.
        TreeCursor cursor_;

        /// @brief True once the walk is finished.
        bool atEnd_;

        /// @brief True if the next increment skips the children of the current node.
        bool skipChildren_ = false;

#pragma endregion
    };

    /// @brief Lazy pre-order range over a node and all of its descendants.
    class PreorderView
#if CPP_TS_HAS_RANGES
        : public std::ranges::view_interface<PreorderView>
#endif // CPP_TS_HAS_RANGES
    {
    public:
#pragma region TYPE_ALIASES

        ////////////////////////////////////////////////////////////////
        // Type Aliases
        ////////////////////////////////////////////////////////////////

        /// @brief Iterator type of the view.
        using iterator = PreorderIterator;

#if CPP_TS_HAS_RANGES
        /// @brief End sentinel type of the view.
        using sentinel = std::default_sentinel_t;
#else
        /// @brief End sentinel type of the view (an end iterator).
        using sentinel = PreorderIterator;
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region LIFECYCLE

        ////////////////////////////////////////////////////////////////
        // Lifecycle
        ////////////////////////////////////////////////////////////////

        /// @brief Constructs an empty view.
        PreorderView() noexcept : root_()
        {}

        /**
         * @brief Constructs a view over a subtree.
         * @param root Root of the subtree. Held by value.
         */
        explicit PreorderView(Node root) noexcept : root_(root)
        {}

#pragma endregion

#pragma region ITERATION

        ////////////////////////////////////////////////////////////////
        // Iteration
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Starts a walk at the root of the view.
         * @return Iterator positioned at the root (an end iterator for a null root).
         */
        CPP_TS_NODISCARD iterator begin() const noexcept
        {
            return iterator(root_);
        }

        /**
         * @brief Returns the end of the walk.
         * @return End sentinel.
         */
        CPP_TS_NODISCARD sentinel end() const noexcept
        {
            return sentinel();
        }

#pragma endregion

    private:
#pragma region FIELDS

        ////////////////////////////////////////////////////////////////
        // Fields
        ////////////////////////////////////////////////////////////////

        /// @brief Root of the walked subtree.
        Node root_;

#pragma endregion
    };

    /**
     * @brief Returns a lazy pre-order range over `root` and all of its descendants.
     * @param root Root of the walked subtree.
     * @return Range yielding nodes in document order (parents before children).
     *
     * @code
     * for (ts::Node node : ts::preorder(root)) { ... }
     * @endcode
     */
    CPP_TS_NODISCARD inline PreorderView preorder(Node root) noexcept
    {
        return PreorderView(root);
    }

#if CPP_TS_HAS_RANGES
    static_assert(std::input_iterator<PreorderIterator>, "PreorderIterator must be an input iterator");
    static_assert(std::ranges::input_range<PreorderView>, "PreorderView must be an input range");
    static_assert(std::ranges::view<PreorderView>, "PreorderView must be a view");
#endif // CPP_TS_HAS_RANGES

#pragma endregion

#pragma region VISITOR

    /////////////////////////////////////////////////////////////////////////////
    // Visitor
    // High-level utility for depth-first traversal of a syntax tree.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Controls how `ts::visit` continues after a node has been entered.
    enum class VisitAction : std::uint8_t
    {
        Continue,     ///< Descend into the children of the node.
        SkipChildren, ///< Skip the children of the node and continue with its next sibling.
        Stop          ///< End the whole traversal.
    };

#pragma region TRAITS

    ////////////////////////////////////////////////////////////////
    // Traits
    ////////////////////////////////////////////////////////////////

    namespace details
    {
        /**
         * @brief Maps any list of types to `void` (implementation of `void_t`).
         * @tparam T Types that must be well-formed.
         */
        template <typename... T>
        struct make_void
        {
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            /// @brief Always `void`.
            using type = void;

#pragma endregion
        };

        /**
         * @brief `std::void_t` replacement available in C++11.
         * @tparam T Types that must be well-formed.
         */
        template <typename... T>
        using void_t = typename make_void<T...>::type;

        /**
         * @brief `std::remove_cvref_t` replacement available in C++11.
         * @tparam T Inspected type.
         */
        template <typename T>
        using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

        /**
         * @brief Detects visitor objects providing `onEnter(Node)`.
         * @tparam V Visitor type.
         */
        template <typename V, typename = void>
        struct has_on_enter : std::false_type
        {};

        /**
         * @brief Specialization for visitor objects providing `onEnter(Node)`.
         * @tparam V Visitor type.
         */
        template <typename V>
        struct has_on_enter<V, void_t<decltype(std::declval<V &>().onEnter(std::declval<Node>()))>> : std::true_type
        {};

        /**
         * @brief Detects visitor objects providing `onLeave(Node)`.
         * @tparam V Visitor type.
         */
        template <typename V, typename = void>
        struct has_on_leave : std::false_type
        {};

        /**
         * @brief Specialization for visitor objects providing `onLeave(Node)`.
         * @tparam V Visitor type.
         */
        template <typename V>
        struct has_on_leave<V, void_t<decltype(std::declval<V &>().onLeave(std::declval<Node>()))>> : std::true_type
        {};

        /**
         * @brief Detects callables accepting a `Node`.
         * @tparam F Callable type.
         */
        template <typename F, typename = void>
        struct is_node_callback : std::false_type
        {};

        /**
         * @brief Specialization for callables accepting a `Node`.
         * @tparam F Callable type.
         */
        template <typename F>
        struct is_node_callback<F, void_t<decltype(std::declval<F &>()(std::declval<Node>()))>> : std::true_type
        {};

        /**
         * @brief Result type of a node callback.
         * @tparam F Callable type.
         */
        template <typename F>
        using node_callback_result = decltype(std::declval<F &>()(std::declval<Node>()));

        /**
         * @brief Detects node callbacks returning `void`, `VisitAction` or a value convertible to `bool`.
         * @tparam F Callable type.
         */
        template <typename F, bool = is_node_callback<F>::value>
        struct is_valid_node_callback : std::false_type
        {};

        /**
         * @brief Specialization checking the result type of a node callback.
         * @tparam F Callable type.
         */
        template <typename F>
        struct is_valid_node_callback<F, true>
            : std::integral_constant<
                      bool,
                      std::is_void<node_callback_result<F>>::value
                              || std::is_same<remove_cvref_t<node_callback_result<F>>, VisitAction>::value
                              || std::is_convertible<node_callback_result<F>, bool>::value>
        {};

        /**
         * @brief True for every type accepted by `ts::visit`.
         * @tparam V Visitor type.
         */
        template <typename V>
        struct is_visitor : std::integral_constant<bool,
                                                   has_on_enter<remove_cvref_t<V>>::value
                                                           || is_valid_node_callback<remove_cvref_t<V>>::value>
        {};

        /**
         * @brief Converts the result of a callback to a `VisitAction`.
         * @param action Result of the callback.
         * @return Returns `action`.
         */
        CPP_TS_NODISCARD constexpr VisitAction to_visit_action(VisitAction action) noexcept
        {
            return action;
        }

        /**
         * @brief Converts the result of a boolean callback to a `VisitAction` (backward compatible meaning).
         * @param stop Result of the callback.
         * @return Returns `VisitAction::Stop` for true, `VisitAction::Continue` for false.
         */
        CPP_TS_NODISCARD constexpr VisitAction to_visit_action(bool stop) noexcept
        {
            return stop ? VisitAction::Stop : VisitAction::Continue;
        }

        /**
         * @brief Calls a node callback returning `void`.
         * @tparam F Callable type.
         * @param callback Called callback.
         * @param node Visited node.
         * @return Returns `VisitAction::Continue`.
         */
        template <typename F>
        VisitAction call_node_callback(F &callback, Node node, std::true_type /* returns void */)
        {
            callback(node);
            return VisitAction::Continue;
        }

        /**
         * @brief Calls a node callback returning a value.
         * @tparam F Callable type.
         * @param callback Called callback.
         * @param node Visited node.
         * @return Returns the result of the callback converted to a `VisitAction`.
         */
        template <typename F>
        VisitAction call_node_callback(F &callback, Node node, std::false_type /* returns a value */)
        {
            return to_visit_action(callback(node));
        }

        /**
         * @brief Enters a node with a visitor object.
         * @tparam V Visitor type.
         * @param visitor Visitor object.
         * @param node Entered node.
         * @return Returns the result of `onEnter`.
         */
        template <typename V>
        VisitAction visit_enter(V &visitor, Node node, std::true_type /* visitor object */)
        {
            return to_visit_action(visitor.onEnter(node));
        }

        /**
         * @brief Enters a node with a callable visitor.
         * @tparam V Visitor type.
         * @param visitor Callable visitor.
         * @param node Entered node.
         * @return Returns the result of the callable converted to a `VisitAction`.
         */
        template <typename V>
        VisitAction visit_enter(V &visitor, Node node, std::false_type /* callable */)
        {
            return call_node_callback(visitor, node, std::is_void<node_callback_result<V>>());
        }

        /**
         * @brief Leaves the current node with a visitor providing `onLeave`.
         * @tparam V Visitor type.
         * @param visitor Visitor object.
         * @param cursor Cursor positioned on the left node.
         *
         * @note Only visitors with `onLeave` pay for reading the current node again.
         */
        template <typename V>
        void visit_leave(V &visitor, const TreeCursor &cursor, std::true_type /* has onLeave */)
        {
            visitor.onLeave(cursor.getCurrentNode());
        }

        /**
         * @brief Leaving a node does nothing for visitors without `onLeave`.
         * @tparam V Visitor type.
         */
        template <typename V>
        void visit_leave(V &, const TreeCursor &, std::false_type /* no onLeave */) noexcept
        {}

        /**
         * @brief Detects folders providing `onNode(Node, Span<R>)`.
         * @tparam F Folder type.
         * @tparam R Result type.
         */
        template <typename F, typename R, typename = void>
        struct has_on_node : std::false_type
        {};

        /**
         * @brief Specialization for folders providing `onNode(Node, Span<R>)`.
         * @tparam F Folder type.
         * @tparam R Result type.
         */
        template <typename F, typename R>
        struct has_on_node<F,
                           R,
                           void_t<decltype(std::declval<F &>().onNode(std::declval<Node>(), std::declval<Span<R>>()))>>
            : std::true_type
        {};

        /**
         * @brief Folds a node with a folder object.
         * @tparam R Result type.
         * @tparam F Folder type.
         * @param folder Folder object.
         * @param node Folded node.
         * @param children Results of the children of the node.
         * @return Returns the result of `onNode`.
         */
        template <typename R, typename F>
        R fold_call(F &folder, Node node, Span<R> children, std::true_type /* onNode */)
        {
            return folder.onNode(node, children);
        }

        /**
         * @brief Folds a node with a callable folder.
         * @tparam R Result type.
         * @tparam F Folder type.
         * @param folder Callable folder.
         * @param node Folded node.
         * @param children Results of the children of the node.
         * @return Returns the result of the callable.
         */
        template <typename R, typename F>
        R fold_call(F &folder, Node node, Span<R> children, std::false_type /* callable */)
        {
            return folder(node, children);
        }
    } // namespace details

#pragma endregion

#pragma region CONCEPTS

    ////////////////////////////////////////////////////////////////
    // Concepts
    ////////////////////////////////////////////////////////////////

#if CPP_TS_HAS_CONCEPTS
    /**
     * @brief A visitor object: `onEnter(Node)` returns a `VisitAction` controlling the traversal.
     * @tparam V Visitor type.
     */
    template <typename V>
    concept TreeVisitor = requires(V &visitor, Node node) {
        { visitor.onEnter(node) } -> std::convertible_to<VisitAction>;
    };

    /**
     * @brief A visitor object that is also notified with `onLeave(Node)` after the children of a node.
     * @tparam V Visitor type.
     */
    template <typename V>
    concept LeavingTreeVisitor = TreeVisitor<V> && requires(V &visitor, Node node) { visitor.onLeave(node); };

    /**
     * @brief A callable accepting a `Node` and returning `void`, `VisitAction` or a value convertible to `bool`
     * (`true` stops the traversal).
     * @tparam F Callable type.
     */
    template <typename F>
    concept NodeCallback = details::is_valid_node_callback<F>::value;

    /**
     * @brief Anything accepted by `ts::visit`: a visitor object or a node callback.
     * @tparam T Visitor type.
     */
    template <typename T>
    concept VisitorConcept = TreeVisitor<std::remove_cvref_t<T>> || NodeCallback<std::remove_cvref_t<T>>;

    /**
     * @brief A folder for `ts::fold`: `onNode(Node, std::span<R>)`, or a callable with the same signature,
     * returning a value convertible to `R`.
     * @tparam F Folder type.
     * @tparam R Result type.
     */
    template <typename F, typename R>
    concept TreeFolder = requires(F &folder, Node node, Span<R> children) {
        { folder.onNode(node, children) } -> std::convertible_to<R>;
    } || requires(F &folder, Node node, Span<R> children) {
        { folder(node, children) } -> std::convertible_to<R>;
    };
#elif CPP_TS_HAS_CXX17
    /**
     * @brief A type trait accepting everything `ts::visit` accepts: a visitor object with `onEnter(Node)` or a
     * callable accepting a `Node` and returning `void`, `VisitAction` or a value convertible to `bool`.
     * @tparam T The type of the visitor.
     */
    template <typename T, typename = void>
    struct VisitorConcept : details::is_visitor<T>
    {};
#endif // CPP_TS_HAS_CONCEPTS / CPP_TS_HAS_CXX17

#pragma endregion

#pragma region FUNCTION

    ////////////////////////////////////////////////////////////////
    // Function
    ////////////////////////////////////////////////////////////////

    /**
     * @brief Performs a depth-first traversal of the subtree rooted at `root`.
     * @tparam F The type of the visitor.
     * @param root The starting node for the traversal.
     * @param visitor Either:
     * - a callable `bool(Node)`: returning true halts the traversal (false continues),
     * - a callable `VisitAction(Node)` or `void(Node)`,
     * - a visitor object with `VisitAction onEnter(Node)` and optionally `onLeave(Node)`, called after the children
     *   of a node (in the same LIFO order a recursive walk would produce).
     *
     * @note The walk is cursor based: no recursion and no allocation beyond the tree cursor.
     */
#if CPP_TS_HAS_CONCEPTS
    template <VisitorConcept F>
#else
    template <typename F, typename std::enable_if<details::is_visitor<F>::value, bool>::type = true>
#endif // CPP_TS_HAS_CONCEPTS
    void visit(Node root, F &&visitor)
    {
        using Visitor  = details::remove_cvref_t<F>;
        using IsObject = std::integral_constant<bool, details::has_on_enter<Visitor>::value>;
        using HasLeave = std::integral_constant<bool, IsObject::value && details::has_on_leave<Visitor>::value>;

        if (root.isNull())
        {
            return;
        }

        TreeCursor cursor = root.getCursor();
        while (true)
        {
            const VisitAction action = details::visit_enter(visitor, cursor.getCurrentNode(), IsObject());
            if (action == VisitAction::Stop)
            {
                return;
            }

            // 1. Go down to the first child.
            if (action != VisitAction::SkipChildren && cursor.gotoFirstChild())
            {
                continue;
            }

            // 2. Leave the node and go sideways, or climb up until a next sibling exists (never above `root`).
            while (true)
            {
                details::visit_leave(visitor, cursor, HasLeave());
                if (cursor.gotoNextSibling())
                {
                    break;
                }
                if (!cursor.gotoParent())
                {
                    return; // Back at (and done with) the root.
                }
            }
        }
    }

    namespace details
    {
        /**
         * @brief Shared implementation of `fold()` and `tryFold()`.
         * @tparam R Result type.
         * @tparam F Folder type.
         * @param root Root of the folded subtree. Must not be null.
         * @param folder Folder object or callable.
         * @return The result computed for `root`.
         */
        template <typename R, typename F>
        R fold_impl(Node root, F &folder)
        {
            using UsesOnNode = std::integral_constant<bool, has_on_node<remove_cvref_t<F>, R>::value>;

            TreeCursor               cursor = root.getCursor();
            std::vector<R>           results; // Finished results of the nodes whose parent is not finished yet.
            std::vector<std::size_t> frames;  // Index in `results` where the children of each open node start.

            while (true)
            {
                if (cursor.gotoFirstChild())
                {
                    frames.push_back(results.size());
                    continue;
                }

                // Leaf.
                results.push_back(fold_call<R>(folder, cursor.getCurrentNode(), Span<R>(), UsesOnNode()));

                while (!cursor.gotoNextSibling())
                {
                    if (!cursor.gotoParent())
                    {
                        return std::move(results.back());
                    }

                    // All children of the current node are folded: fold the node itself.
                    const std::size_t start = frames.back();
                    frames.pop_back();

                    R value = fold_call<R>(folder,
                                           cursor.getCurrentNode(),
                                           Span<R>(results.data() + start, results.size() - start),
                                           UsesOnNode());
                    results.erase(results.begin() + static_cast<std::ptrdiff_t>(start), results.end());
                    results.push_back(std::move(value));
                }
            }
        }
    } // namespace details

    /**
     * @brief Computes a value bottom-up (post-order fold) over the subtree rooted at `root`.
     * @tparam R Result type (must be movable, `bool` is not supported because of `std::vector<bool>`).
     * @tparam F Folder type.
     * @param root Root of the folded subtree. Must not be null.
     * @param folder Object with `R onNode(Node, Span<R>)` or a callable with the same signature. It receives the
     * results already computed for the children of the node (an empty span for leaves).
     * @return The result computed for `root`.
     * @throws std::invalid_argument If `root` is null.
     *
     * @note All intermediate results share a single buffer, so the fold performs O(log n) amortized allocations
     * instead of one allocation per node.
     */
#if CPP_TS_HAS_CONCEPTS
    template <typename R, TreeFolder<R> F>
#else
    template <typename R, typename F>
#endif // CPP_TS_HAS_CONCEPTS
    CPP_TS_NODISCARD R fold(Node root, F &&folder)
    {
        static_assert(!std::is_same<R, bool>::value,
                      "ts::fold<bool> is not supported (std::vector<bool> has no contiguous storage), fold to a "
                      "small integer type instead");

        if (root.isNull())
        {
            details::Status(ErrorKind::InvalidArgument, details::null_fold_root_message).raise();
        }
        return details::fold_impl<R>(root, folder);
    }

#if CPP_TS_HAS_EXPECTED
    /**
     * @brief Computes a value bottom-up over the subtree rooted at `root`, without throwing for a null root.
     * @tparam R Result type (`bool` is not supported).
     * @tparam F Folder type.
     * @param root Root of the folded subtree.
     * @param folder Object with `R onNode(Node, Span<R>)` or a callable with the same signature.
     * @return The result computed for `root`, or an `InvalidArgument` error if `root` is null.
     * @throws Any exception thrown by the folder.
     */
#if CPP_TS_HAS_CONCEPTS
    template <typename R, TreeFolder<R> F>
#else
    template <typename R, typename F>
#endif // CPP_TS_HAS_CONCEPTS
    CPP_TS_NODISCARD std::expected<R, Error> tryFold(Node root, F &&folder)
    {
        static_assert(!std::is_same<R, bool>::value,
                      "ts::tryFold<bool> is not supported (std::vector<bool> has no contiguous storage), fold to a "
                      "small integer type instead");

        if (root.isNull())
        {
            return details::to_unexpected(details::Status(ErrorKind::InvalidArgument, details::null_fold_root_message));
        }
        return details::fold_impl<R>(root, folder);
    }
#endif // CPP_TS_HAS_EXPECTED

#pragma endregion

#pragma endregion

} // namespace ts

#pragma region STD_HASH

/////////////////////////////////////////////////////////////////////////////
// std::hash
// Allows Points and Nodes to be used as keys of unordered containers.
/////////////////////////////////////////////////////////////////////////////

namespace std
{
    /// @brief Hash of a source position.
    template <>
    struct hash<ts::Point>
    {
#pragma region OPERATORS

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Computes the hash of a source position.
         * @param point Hashed position.
         * @return Returns the hash of the row and the column.
         */
        CPP_TS_NODISCARD std::size_t operator()(const ts::Point &point) const noexcept
        {
            const std::uint64_t value = (static_cast<std::uint64_t>(point.row) << 32) | point.column;
            return std::hash<std::uint64_t>()(value);
        }

#pragma endregion
    };

    /**
     * @brief Hash of a node.
     * @details Uses only `Node::getID()`, the identity of a node: equal nodes always have the same id, whatever else
     * `ts_node_eq` compares, so the hash stays consistent with `ts::Node::operator==` (nodes at the same position of
     * different trees merely share a bucket).
     */
    template <>
    struct hash<ts::Node>
    {
#pragma region OPERATORS

        ////////////////////////////////////////////////////////////////
        // Operators
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Computes the hash of a node.
         * @param node Hashed node.
         * @return Returns the hash of the node id.
         */
        CPP_TS_NODISCARD std::size_t operator()(const ts::Node &node) const noexcept
        {
            return std::hash<ts::NodeID>()(node.getID());
        }

#pragma endregion
    };
} // namespace std

#pragma endregion

#if defined(CPP_TS_HEADER_ONLY)
#include "cpp-tree-sitter.inl"
#endif // defined(CPP_TS_HEADER_ONLY)

#endif // CPP_TREE_SITTER_HPP
