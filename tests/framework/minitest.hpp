#pragma once
#ifndef CPP_TREE_SITTER_TESTS_MINITEST_HPP
#define CPP_TREE_SITTER_TESTS_MINITEST_HPP

// Minimal, dependency-free unit test framework written in C++11.
//
// Catch2 v3 requires C++14 (and raises the standard of everything linked with it), so the C++11 configuration runs the
// same test files with this framework. It implements the subset of Catch2 used by the tests, with the same names and
// semantics, so every test is written once (see framework.hpp):
//
//   TEST_CASE("Parser parses a string", "[parser]")
//   {
//       ts::Parser parser(language);
//       REQUIRE(parser.hasLanguage());                   // stops the test case on failure
//
//       SECTION("Root node")                             // every SECTION runs in a separate pass of the test case
//       {
//           CHECK(tree.getRootNode().getChildCount() == 1); // reports "0 == 1" on failure and continues
//       }
//
//       SECTION("Rejected old tree")
//       {
//           CHECK_THROWS_AS(parser.parseString(text, other), std::invalid_argument);
//       }
//   }
//
// Limitations (shared with Catch2): `&&` / `||` cannot be decomposed, write CHECK((a && b)) or use two checks.
// Values of types without `operator<<` are printed as "{?}" unless ts_test::StringMaker is specialized.
//
// Test cases register themselves; see minitest-main.cpp for the command line of the runner.

// STL
#include <cstddef>
#include <exception>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ts_test
{
#pragma region REGISTRY

    /////////////////////////////////////////////////////////////////////////////
    // Registry
    // Test cases registered by TEST_CASE.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A registered test case.
    struct TestCase
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Name of the test case.
        std::string name;

        /// @brief Tags in the Catch2 format, e.g. "[parser][regression]".
        std::string tags;

        /// @brief Body of the test case.
        void (*function)();

        /// @brief File defining the test case.
        const char *file;

        /// @brief Line of the TEST_CASE macro.
        int line;

#pragma endregion
    };

    /**
     * @brief Returns all test cases registered by the TEST_CASE macro.
     * @return Test cases in registration order.
     */
    inline std::vector<TestCase> &registry()
    {
        static std::vector<TestCase> test_cases;
        return test_cases;
    }

    /// @brief Registers a test case during static initialization.
    struct Registrar
    {
#pragma region LIFECYCLE

        /////////////////////////////////////////////////////////////////////////////
        // Lifecycle
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Registers a test case.
         * @param name Name of the test case.
         * @param tags Tags in the Catch2 format.
         * @param function Body of the test case.
         * @param file File defining the test case.
         * @param line Line of the TEST_CASE macro.
         */
        Registrar(const char *name, const char *tags, void (*function)(), const char *file, int line)
        {
            TestCase test_case;
            test_case.name     = name;
            test_case.tags     = tags;
            test_case.function = function;
            test_case.file     = file;
            test_case.line     = line;
            registry().push_back(test_case);
        }

#pragma endregion
    };

#pragma endregion

#pragma region RESULTS

    /////////////////////////////////////////////////////////////////////////////
    // Results
    // Assertion counters and failure reporting.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Counters of the assertions of the running test case.
    struct Counters
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Number of evaluated assertions.
        std::size_t assertions;

        /// @brief Number of failed assertions.
        std::size_t failures;

#pragma endregion
    };

    /**
     * @brief Returns the counters of the running test case.
     * @return Reference to the counters.
     */
    inline Counters &counters()
    {
        static Counters value = { 0, 0 };
        return value;
    }

    /// @brief Thrown by a failed REQUIRE to abort the running test case.
    struct RequireFailure
    {};

    /// @brief Location and text of an assertion.
    struct AssertionInfo
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Name of the assertion macro.
        const char *macro;

        /// @brief Text of the checked expression.
        const char *expression;

        /// @brief File of the assertion.
        const char *file;

        /// @brief Line of the assertion.
        int line;

        /// @brief True for REQUIRE assertions, which abort the test case on failure.
        bool require;

#pragma endregion
    };

    /// @brief Records a successful assertion.
    inline void pass()
    {
        ++counters().assertions;
    }

    /**
     * @brief Records a failed assertion, prints it and aborts the test case for REQUIRE assertions.
     * @param info Failed assertion.
     * @param details Additional information (expanded values, exception message). May be empty.
     */
    inline void fail(const AssertionInfo &info, const std::string &details)
    {
        ++counters().assertions;
        ++counters().failures;

        std::cerr << info.file << ":" << info.line << ": FAILED:\n  " << info.macro << "(" << info.expression << ")\n";
        if (!details.empty())
        {
            std::cerr << "  " << details << "\n";
        }
        std::cerr << std::flush;

        if (info.require)
        {
            throw RequireFailure();
        }
    }

    /**
     * @brief Returns the message of the exception being handled.
     * @return Returns `what()` of a `std::exception`, or a generic text for other exceptions.
     */
    inline std::string currentExceptionMessage()
    {
        try
        {
            throw;
        }
        catch (const std::exception &error)
        {
            return error.what();
        }
        catch (...)
        {
            return "unknown exception type";
        }
    }

#pragma endregion

#pragma region SECTIONS

    /////////////////////////////////////////////////////////////////////////////
    // Sections
    // Catch2-like SECTION tracking: one pass of a test case per leaf section.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Tracks the sections of the running test case.
     * @details Like in Catch2, a test case is run once per leaf section: every pass enters the first section of each
     * level that has not been completed yet and skips its siblings, which are run by the next passes. Code outside of
     * sections runs in every pass.
     */
    struct SectionTracker
    {
#pragma region INNER_STRUCT

        /////////////////////////////////////////////////////////////////////////////
        // Inner Struct
        /////////////////////////////////////////////////////////////////////////////

        /// @brief A section entered during the current pass.
        struct Frame
        {
#pragma region FIELDS

            /////////////////////////////////////////////////////////////////////////////
            // Fields
            /////////////////////////////////////////////////////////////////////////////

            /// @brief Names of the enclosing sections, separated by '/'.
            std::string path;

            /// @brief True once a child section was entered during this pass.
            bool entered_child;

#pragma endregion
        };

#pragma endregion

#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Every section seen so far (entered or skipped).
        std::set<std::string> discovered;

        /// @brief Sections whose children all ran.
        std::set<std::string> completed;

        /// @brief Sections entered during the current pass, the root (the test case itself) first.
        std::vector<Frame> stack;

#pragma endregion

#pragma region PASS

        /////////////////////////////////////////////////////////////////////////////
        // Pass
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Forgets the sections of the previous test case.
        void reset()
        {
            discovered.clear();
            completed.clear();
        }

        /// @brief Prepares a new pass of the test case.
        void startPass()
        {
            Frame root = { "", false };
            stack.assign(1, root);
        }

        /**
         * @brief Checks whether a discovered section below `path` did not complete yet.
         * @param path Path of a section, or "" for the whole test case.
         * @return Returns true if another pass is needed for a section below `path`.
         */
        bool hasPendingBelow(const std::string &path) const
        {
            const std::string prefix = path + "/";
            for (std::set<std::string>::const_iterator it = discovered.lower_bound(prefix);
                 it != discovered.end() && it->compare(0, prefix.size(), prefix) == 0;
                 ++it)
            {
                if (completed.count(*it) == 0)
                {
                    return true;
                }
            }
            return false;
        }

#pragma endregion
    };

    /**
     * @brief Returns the section tracker of the running test case.
     * @return Reference to the tracker.
     */
    inline SectionTracker &sectionTracker()
    {
        static SectionTracker tracker;
        return tracker;
    }

    /// @brief Scope of a SECTION. Converts to true if the section runs in this pass.
    class Section
    {
    public:
#pragma region LIFECYCLE

        /////////////////////////////////////////////////////////////////////////////
        // Lifecycle
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Enters the section if it runs in this pass.
         * @param name Name of the section, unique among its siblings.
         */
        explicit Section(const char *name) : entered_(false)
        {
            SectionTracker        &tracker = sectionTracker();
            SectionTracker::Frame &parent  = tracker.stack.back();
            path_                          = parent.path + "/" + name;
            tracker.discovered.insert(path_);

            // Every pass enters the first section of a level that is not completed; its siblings run in later passes.
            if (tracker.completed.count(path_) != 0 || parent.entered_child)
            {
                return;
            }

            parent.entered_child        = true;
            entered_                    = true;
            SectionTracker::Frame frame = { path_, false };
            tracker.stack.push_back(frame);
        }

        /// @brief Leaves the section and marks it as completed when none of its children is pending.
        ~Section()
        {
            if (!entered_)
            {
                return;
            }

            // Also runs when a REQUIRE failure or an exception leaves the section: the section is completed unless one
            // of its child sections still has to run.
            SectionTracker &tracker = sectionTracker();
            tracker.stack.pop_back();
            if (!tracker.hasPendingBelow(path_))
            {
                tracker.completed.insert(path_);
            }
        }

        /**
         * @brief Deleted copy constructor.
         * @param other Another section.
         */
        Section(const Section &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another section.
         * @return Returns a reference to this section.
         */
        Section &operator=(const Section &other) = delete;

#pragma endregion

#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Checks whether the section runs in this pass.
         * @return Returns true if the body of the SECTION is executed.
         */
        explicit operator bool() const
        {
            return entered_;
        }

#pragma endregion

    private:
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Path of the section.
        std::string path_;

        /// @brief True if the section runs in this pass.
        bool entered_;

#pragma endregion
    };

#pragma endregion

#pragma region STRINGIFICATION

    /////////////////////////////////////////////////////////////////////////////
    // Stringification
    // Text shown for the values of a failed assertion.
    /////////////////////////////////////////////////////////////////////////////

    namespace details
    {
        /**
         * @brief Detects types that can be written to a std::ostream.
         * @tparam T Checked type.
         */
        template <typename T>
        class is_streamable
        {
        private:
#pragma region PRIVATE_METHODS

            /////////////////////////////////////////////////////////////////////////////
            // Private Methods
            /////////////////////////////////////////////////////////////////////////////

            /**
             * @brief Selected for streamable types.
             * @tparam U Checked type.
             * @return Returns `std::true_type` (only used in unevaluated context).
             */
            template <typename U>
            static auto test(int)
                    -> decltype(std::declval<std::ostream &>() << std::declval<const U &>(), std::true_type());

            /**
             * @brief Selected for every other type.
             * @return Returns `std::false_type` (only used in unevaluated context).
             */
            template <typename>
            static std::false_type test(...);

#pragma endregion

        public:
#pragma region CONSTANTS

            /////////////////////////////////////////////////////////////////////////////
            // Constants
            /////////////////////////////////////////////////////////////////////////////

            /// @brief True if `T` can be written to a std::ostream.
            static const bool value = decltype(test<T>(0))::value;

#pragma endregion
        };

        /**
         * @brief Writes a value to a string with `operator<<`.
         * @tparam T Type of the value.
         * @param value Written value.
         * @return Returns the text.
         */
        template <typename T>
        std::string stream(const T &value)
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }

        /**
         * @brief Converts an enum to text (its underlying value).
         * @tparam T Enum type.
         * @param value Converted value.
         * @return Returns the number.
         */
        template <typename T>
        std::string fallback(const T &value, std::true_type /* enum */, std::false_type)
        {
            return stream(static_cast<long long>(value));
        }

        /**
         * @brief Converts a streamable value to text.
         * @tparam T Type of the value.
         * @param value Converted value.
         * @return Returns the text written by `operator<<`.
         */
        template <typename T>
        std::string fallback(const T &value, std::false_type, std::true_type /* streamable */)
        {
            return stream(value);
        }

        /**
         * @brief Converts a value that cannot be printed.
         * @tparam T Type of the value.
         * @param value Converted value. Unused.
         * @return Returns "{?}".
         */
        template <typename T>
        std::string fallback(const T &value, std::false_type, std::false_type)
        {
            static_cast<void>(value);
            return "{?}";
        }
    } // namespace details

    /**
     * @brief Converts a value to the text shown when an assertion fails.
     * @details Specialize it for project types (see common.hpp). Enums are printed as numbers, streamable types with
     * operator<<, everything else as "{?}".
     * @tparam T Type of the converted values.
     */
    template <typename T>
    struct StringMaker
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(const T &value)
        {
            return details::fallback(
                    value,
                    std::integral_constant<bool, std::is_enum<T>::value>(),
                    std::integral_constant<bool, !std::is_enum<T>::value && details::is_streamable<T>::value>());
        }

#pragma endregion
    };

    /// @brief Prints `true` / `false`.
    template <>
    struct StringMaker<bool>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(bool value)
        {
            return value ? "true" : "false";
        }

#pragma endregion
    };

    /// @brief Prints `nullptr`.
    template <>
    struct StringMaker<std::nullptr_t>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(std::nullptr_t value)
        {
            static_cast<void>(value);
            return "nullptr";
        }

#pragma endregion
    };

    /// @brief Prints strings in quotes.
    template <>
    struct StringMaker<std::string>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(const std::string &value)
        {
            return "\"" + value + "\"";
        }

#pragma endregion
    };

    /// @brief Prints C strings in quotes (`nullptr` for a null pointer).
    template <>
    struct StringMaker<const char *>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(const char *value)
        {
            return value ? "\"" + std::string(value) + "\"" : "nullptr";
        }

#pragma endregion
    };

    /// @brief Prints C strings in quotes.
    template <>
    struct StringMaker<char *> : StringMaker<const char *>
    {};

    /// @brief Prints character arrays (string literals) in quotes.
    template <std::size_t N>
    struct StringMaker<char[N]> : StringMaker<const char *>
    {};

    /// @brief Prints character arrays (string literals) in quotes.
    template <std::size_t N>
    struct StringMaker<const char[N]> : StringMaker<const char *>
    {};

    /// @brief Prints characters in single quotes.
    template <>
    struct StringMaker<char>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(char value)
        {
            return "'" + std::string(1, value) + "'";
        }

#pragma endregion
    };

    /// @brief Prints bytes as numbers.
    template <>
    struct StringMaker<unsigned char>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param value Converted value.
         * @return Returns the text.
         */
        static std::string convert(unsigned char value)
        {
            return details::stream(static_cast<unsigned>(value));
        }

#pragma endregion
    };

    /**
     * @brief Converts a value to the text shown when an assertion fails.
     * @tparam T Type of the value.
     * @param value Converted value.
     * @return Returns the text produced by `StringMaker`.
     */
    template <typename T>
    std::string toString(const T &value)
    {
        return StringMaker<typename std::remove_cv<T>::type>::convert(value);
    }

#pragma endregion

#pragma region EXPRESSION_DECOMPOSITION

    /////////////////////////////////////////////////////////////////////////////
    // Expression Decomposition
    // Captures both operands of `CHECK(a == b)`.
    /////////////////////////////////////////////////////////////////////////////

// Comparisons are performed on the operand types chosen by the test author, so signed/unsigned mismatches are
// intended (e.g. CHECK(vector.size() == 2)).
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4018 4389)
#endif

    /// @brief Result of a decomposed assertion expression.
    struct ExpressionResult
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Truth value of the expression.
        bool passed;

        /// @brief Text of the evaluated operands, e.g. "1 == 2".
        std::string expansion;

#pragma endregion
    };

    /**
     * @brief Left operand of a decomposed expression (`Decomposer() <= lhs`).
     * @tparam L Type of the left operand.
     */
    template <typename L>
    class ExpressionLhs
    {
    public:
#pragma region LIFECYCLE

        /////////////////////////////////////////////////////////////////////////////
        // Lifecycle
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Captures the left operand.
         * @param lhs Left operand.
         */
        explicit ExpressionLhs(const L &lhs) : lhs_(lhs)
        {}

#pragma endregion

#pragma region COMPARISON

        /////////////////////////////////////////////////////////////////////////////
        // Comparison
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Evaluates `lhs == rhs`.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Returns the result and the expansion of the comparison.
         */
        template <typename R>
        ExpressionResult operator==(const R &rhs) const
        {
            return make(static_cast<bool>(lhs_ == rhs), "==", rhs);
        }

        /**
         * @brief Evaluates `lhs != rhs`.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Returns the result and the expansion of the comparison.
         */
        template <typename R>
        ExpressionResult operator!=(const R &rhs) const
        {
            return make(static_cast<bool>(lhs_ != rhs), "!=", rhs);
        }

        /**
         * @brief Evaluates `lhs < rhs`.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Returns the result and the expansion of the comparison.
         */
        template <typename R>
        ExpressionResult operator<(const R &rhs) const
        {
            return make(static_cast<bool>(lhs_ < rhs), "<", rhs);
        }

        /**
         * @brief Evaluates `lhs <= rhs`.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Returns the result and the expansion of the comparison.
         */
        template <typename R>
        ExpressionResult operator<=(const R &rhs) const
        {
            return make(static_cast<bool>(lhs_ <= rhs), "<=", rhs);
        }

        /**
         * @brief Evaluates `lhs > rhs`.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Returns the result and the expansion of the comparison.
         */
        template <typename R>
        ExpressionResult operator>(const R &rhs) const
        {
            return make(static_cast<bool>(lhs_ > rhs), ">", rhs);
        }

        /**
         * @brief Evaluates `lhs >= rhs`.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Returns the result and the expansion of the comparison.
         */
        template <typename R>
        ExpressionResult operator>=(const R &rhs) const
        {
            return make(static_cast<bool>(lhs_ >= rhs), ">=", rhs);
        }

#pragma endregion

#pragma region REJECTED_OPERATORS

        /////////////////////////////////////////////////////////////////////////////
        // Rejected Operators
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Rejected at compile time: `&&` cannot be decomposed.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Never returns.
         */
        template <typename R>
        ExpressionResult operator&&(const R &rhs) const
        {
            static_assert(sizeof(R) == 0, "&& cannot be decomposed: write CHECK((a && b)) or use two checks");
            return ExpressionResult();
        }

        /**
         * @brief Rejected at compile time: `||` cannot be decomposed.
         * @tparam R Type of the right operand.
         * @param rhs Right operand.
         * @return Never returns.
         */
        template <typename R>
        ExpressionResult operator||(const R &rhs) const
        {
            static_assert(sizeof(R) == 0, "|| cannot be decomposed: write CHECK((a || b)) or use two checks");
            return ExpressionResult();
        }

#pragma endregion

#pragma region EVALUATION

        /////////////////////////////////////////////////////////////////////////////
        // Evaluation
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Evaluates an expression without a comparison operator.
         * @return Returns the truth value and the expansion of the operand.
         */
        ExpressionResult evaluate() const
        {
            ExpressionResult result;
            result.passed    = static_cast<bool>(lhs_);
            result.expansion = toString(lhs_);
            return result;
        }

#pragma endregion

    private:
#pragma region PRIVATE_METHODS

        /////////////////////////////////////////////////////////////////////////////
        // Private Methods
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Builds the result of a comparison.
         * @tparam R Type of the right operand.
         * @param passed Result of the comparison.
         * @param op Comparison operator.
         * @param rhs Right operand.
         * @return Returns the result with the expansion of both operands.
         */
        template <typename R>
        ExpressionResult make(bool passed, const char *op, const R &rhs) const
        {
            // Always expanded: CHECK_FALSE needs the expansion when the comparison is true.
            ExpressionResult result;
            result.passed    = passed;
            result.expansion = toString(lhs_) + " " + op + " " + toString(rhs);
            return result;
        }

#pragma endregion

#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Left operand.
        const L &lhs_;

#pragma endregion
    };

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

    /// @brief Captures the left operand of an assertion expression.
    struct Decomposer
    {
#pragma region OPERATORS

        /////////////////////////////////////////////////////////////////////////////
        // Operators
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Captures the left operand (`<=` binds tighter than the comparison operators).
         * @tparam L Type of the left operand.
         * @param lhs Left operand.
         * @return Returns the captured operand.
         */
        template <typename L>
        ExpressionLhs<L> operator<=(const L &lhs) const
        {
            return ExpressionLhs<L>(lhs);
        }

#pragma endregion
    };

    /**
     * @brief Returns the result of a decomposed comparison.
     * @param result Result of the comparison.
     * @return Returns `result`.
     */
    inline ExpressionResult toResult(const ExpressionResult &result)
    {
        return result;
    }

    /**
     * @brief Evaluates a decomposed expression without a comparison operator.
     * @tparam L Type of the operand.
     * @param lhs Captured operand.
     * @return Returns the result of `lhs.evaluate()`.
     */
    template <typename L>
    ExpressionResult toResult(const ExpressionLhs<L> &lhs)
    {
        return lhs.evaluate();
    }

    /**
     * @brief Records the outcome of a decomposed assertion.
     * @param info Assertion being evaluated.
     * @param expected Expected truth value (false for CHECK_FALSE / REQUIRE_FALSE).
     * @param result Decomposed expression.
     */
    inline void handle(const AssertionInfo &info, bool expected, const ExpressionResult &result)
    {
        if (result.passed == expected)
        {
            pass();
            return;
        }

        std::string expansion = result.expansion;
        if (expansion.empty())
        {
            expansion = toString(result.passed);
        }
        fail(info, "with expansion: " + std::string(expected ? "" : "!(") + expansion + (expected ? "" : ")"));
    }

#pragma endregion
} // namespace ts_test

#pragma region MACROS

/////////////////////////////////////////////////////////////////////////////
// Macros
// Catch2 compatible test macros.
/////////////////////////////////////////////////////////////////////////////

/// @brief Concatenates two tokens (implementation of TS_TEST_CONCAT).
#define TS_TEST_CONCAT_IMPL(a, b) a##b

/// @brief Concatenates two tokens after expanding them.
#define TS_TEST_CONCAT(a, b) TS_TEST_CONCAT_IMPL(a, b)

/// @brief Creates an identifier unique in the file (suffixed with the line number).
#define TS_TEST_UNIQUE(name) TS_TEST_CONCAT(name, __LINE__)

// `Decomposer() <= a == b` relies on operator precedence on purpose: silence the "suggest parentheses" warning.
#if defined(__GNUC__) || defined(__clang__)
/// @brief Starts suppressing the "suggest parentheses" warning.
#define TS_TEST_SUPPRESS_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wparentheses\"")

/// @brief Stops suppressing the "suggest parentheses" warning.
#define TS_TEST_SUPPRESS_END _Pragma("GCC diagnostic pop")
#else
#define TS_TEST_SUPPRESS_BEGIN
#define TS_TEST_SUPPRESS_END
#endif

/// @brief Declares, registers and starts the definition of a test case function.
#define TS_TEST_CASE_IMPL(function, registrar, name, tags)                            \
    static void                 function();                                           \
    static ::ts_test::Registrar registrar(name, tags, &function, __FILE__, __LINE__); \
    static void                 function()

/// @brief Defines and registers a test case. `tags` uses the Catch2 format, e.g. "[parser][regression]".
#define TEST_CASE(name, tags) \
    TS_TEST_CASE_IMPL(TS_TEST_UNIQUE(ts_test_function_), TS_TEST_UNIQUE(ts_test_registrar_), name, tags)

/// @brief Evaluates a decomposed assertion; unexpected exceptions are reported as failures.
#define TS_TEST_ASSERT(macro, require, expected, ...)                                                               \
    do                                                                                                              \
    {                                                                                                               \
        const ::ts_test::AssertionInfo ts_test_info = { macro, #__VA_ARGS__, __FILE__, __LINE__, require };         \
        try                                                                                                         \
        {                                                                                                           \
            TS_TEST_SUPPRESS_BEGIN                                                                                  \
            ::ts_test::handle(ts_test_info, expected, ::ts_test::toResult(::ts_test::Decomposer() <= __VA_ARGS__)); \
            TS_TEST_SUPPRESS_END                                                                                    \
        }                                                                                                           \
        catch (const ::ts_test::RequireFailure &)                                                                   \
        {                                                                                                           \
            throw;                                                                                                  \
        }                                                                                                           \
        catch (...)                                                                                                 \
        {                                                                                                           \
            ::ts_test::fail(ts_test_info, "unexpected exception: " + ::ts_test::currentExceptionMessage());         \
        }                                                                                                           \
    } while (false)

/// @brief Checks that an expression throws an exception of a given type.
#define TS_TEST_THROWS(macro, require, expression, exception_type, exception_name)                               \
    do                                                                                                           \
    {                                                                                                            \
        const ::ts_test::AssertionInfo ts_test_info   = { macro,                                                 \
                                                          #expression ", " exception_name,                       \
                                                          __FILE__,                                              \
                                                          __LINE__,                                              \
                                                          require };                                             \
        bool                           ts_test_thrown = false;                                                   \
        try                                                                                                      \
        {                                                                                                        \
            static_cast<void>(expression);                                                                       \
        }                                                                                                        \
        catch (const exception_type &)                                                                           \
        {                                                                                                        \
            ts_test_thrown = true;                                                                               \
        }                                                                                                        \
        catch (...)                                                                                              \
        {                                                                                                        \
            ::ts_test::fail(ts_test_info, "unexpected exception type: " + ::ts_test::currentExceptionMessage()); \
            break;                                                                                               \
        }                                                                                                        \
        if (ts_test_thrown)                                                                                      \
        {                                                                                                        \
            ::ts_test::pass();                                                                                   \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            ::ts_test::fail(ts_test_info, "no exception was thrown");                                            \
        }                                                                                                        \
    } while (false)

/// @brief Checks that an expression throws an exception whose what() equals a message.
#define TS_TEST_THROWS_WITH(macro, require, expression, message)                                 \
    do                                                                                           \
    {                                                                                            \
        const ::ts_test::AssertionInfo ts_test_info   = { macro,                                 \
                                                          #expression ", " #message,             \
                                                          __FILE__,                              \
                                                          __LINE__,                              \
                                                          require };                             \
        bool                           ts_test_thrown = false;                                   \
        std::string                    ts_test_what;                                             \
        try                                                                                      \
        {                                                                                        \
            static_cast<void>(expression);                                                       \
        }                                                                                        \
        catch (...)                                                                              \
        {                                                                                        \
            ts_test_thrown = true;                                                               \
            ts_test_what   = ::ts_test::currentExceptionMessage();                               \
        }                                                                                        \
        if (!ts_test_thrown)                                                                     \
        {                                                                                        \
            ::ts_test::fail(ts_test_info, "no exception was thrown");                            \
        }                                                                                        \
        else if (ts_test_what == std::string(message))                                           \
        {                                                                                        \
            ::ts_test::pass();                                                                   \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            ::ts_test::fail(ts_test_info, "with message: " + ::ts_test::toString(ts_test_what)); \
        }                                                                                        \
    } while (false)

/// @brief Checks that an expression does not throw.
#define TS_TEST_NOTHROW(macro, require, expression)                                                         \
    do                                                                                                      \
    {                                                                                                       \
        const ::ts_test::AssertionInfo ts_test_info = { macro, #expression, __FILE__, __LINE__, require };  \
        try                                                                                                 \
        {                                                                                                   \
            static_cast<void>(expression);                                                                  \
            ::ts_test::pass();                                                                              \
        }                                                                                                   \
        catch (...)                                                                                         \
        {                                                                                                   \
            ::ts_test::fail(ts_test_info, "unexpected exception: " + ::ts_test::currentExceptionMessage()); \
        }                                                                                                   \
    } while (false)

/// @brief Checks an expression and continues the test case on failure.
#define CHECK(...) TS_TEST_ASSERT("CHECK", false, true, __VA_ARGS__)
/// @brief Checks that an expression is false and continues the test case on failure.
#define CHECK_FALSE(...) TS_TEST_ASSERT("CHECK_FALSE", false, false, __VA_ARGS__)
/// @brief Checks an expression and stops the test case on failure.
#define REQUIRE(...) TS_TEST_ASSERT("REQUIRE", true, true, __VA_ARGS__)
/// @brief Checks that an expression is false and stops the test case on failure.
#define REQUIRE_FALSE(...) TS_TEST_ASSERT("REQUIRE_FALSE", true, false, __VA_ARGS__)

/// @brief Checks that an expression throws an exception of the given type (or a derived one).
#define CHECK_THROWS_AS(expression, exception_type) \
    TS_TEST_THROWS("CHECK_THROWS_AS", false, expression, exception_type, #exception_type)
/// @brief Like CHECK_THROWS_AS, but stops the test case on failure.
#define REQUIRE_THROWS_AS(expression, exception_type) \
    TS_TEST_THROWS("REQUIRE_THROWS_AS", true, expression, exception_type, #exception_type)
/// @brief Checks that an expression throws any exception.
#define CHECK_THROWS(expression) TS_TEST_THROWS("CHECK_THROWS", false, expression, ::std::exception, "std::exception")
/// @brief Like CHECK_THROWS, but stops the test case on failure.
#define REQUIRE_THROWS(expression) \
    TS_TEST_THROWS("REQUIRE_THROWS", true, expression, ::std::exception, "std::exception")
/// @brief Checks that an expression throws an exception whose what() equals `message`.
#define CHECK_THROWS_WITH(expression, message) TS_TEST_THROWS_WITH("CHECK_THROWS_WITH", false, expression, message)
/// @brief Like CHECK_THROWS_WITH, but stops the test case on failure.
#define REQUIRE_THROWS_WITH(expression, message) TS_TEST_THROWS_WITH("REQUIRE_THROWS_WITH", true, expression, message)
/// @brief Checks that an expression does not throw.
#define CHECK_NOTHROW(expression) TS_TEST_NOTHROW("CHECK_NOTHROW", false, expression)
/// @brief Like CHECK_NOTHROW, but stops the test case on failure.
#define REQUIRE_NOTHROW(expression) TS_TEST_NOTHROW("REQUIRE_NOTHROW", true, expression)

/// @brief Runs the enclosed block in its own pass of the test case (see ts_test::SectionTracker).
#define SECTION(name) if (const ::ts_test::Section &TS_TEST_UNIQUE(ts_test_section_) = ::ts_test::Section(name))

/// @brief Fails the test case unconditionally with a message.
#define FAIL(message) \
    ::ts_test::fail(::ts_test::AssertionInfo{ "FAIL", "", __FILE__, __LINE__, true }, std::string(message))

/// @brief Records a successful assertion.
#define SUCCEED(message) ::ts_test::pass()

#pragma endregion

#endif // CPP_TREE_SITTER_TESTS_MINITEST_HPP
