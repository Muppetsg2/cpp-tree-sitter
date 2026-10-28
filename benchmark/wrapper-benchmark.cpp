#include "pch.hpp"

// tree-sitter
#include <tree_sitter/api.h>

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// wasmtime
#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
#if CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
#include <wasmtime.hh>
#else
#include <wasm.h>
#endif // CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

// STL
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
#include <fstream>
#include <ios>
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

#if defined(_WIN32)
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif // defined(_WIN32)

/**
 * @brief Returns the JSON grammar (tree-sitter-json, linked statically).
 * @return Returns the language. Owned by the grammar library.
 */
extern "C" const TSLanguage *tree_sitter_json();

#if !defined(CPP_TS_BENCHMARK_WASM_JSON_PATH)
#define CPP_TS_BENCHMARK_WASM_JSON_PATH CPP_TS_BENCHMARK_WASM_FILES_DIR "/tree-sitter-json.wasm"
#endif // !defined(CPP_TS_BENCHMARK_WASM_JSON_PATH)

#pragma region STOPWATCH

/////////////////////////////////////////////////////////////////////////////
// Stopwatch
/////////////////////////////////////////////////////////////////////////////

/// @brief Helper class to measure time in milliseconds. Used to benchmark performance.
class Stopwatch
{
public:
#pragma region LIFECYCLE

    /////////////////////////////////////////////////////////////////////////////
    // Lifecycle
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Creates a Stopwatch object and starts the measurement immediately.
    Stopwatch() : start_time_(std::chrono::high_resolution_clock::now())
    {}

#pragma endregion

#pragma region MEASUREMENT

    /////////////////////////////////////////////////////////////////////////////
    // Measurement
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Resets and restarts the stopwatch (used before each benchmark iteration).
    inline void reset() noexcept
    {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Returns the elapsed time since creation or the last `reset` call.
     * @return Elapsed time in milliseconds as a double.
     */
    inline double read() const noexcept
    {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time_)
                .count();
    }

#pragma endregion

private:
#pragma region FIELDS

    /////////////////////////////////////////////////////////////////////////////
    // Fields
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Time of the creation or of the last `reset` call.
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;

#pragma endregion
};

#pragma endregion

#pragma region INPUT_FUNCTIONS

/////////////////////////////////////////////////////////////////////////////
// Input Functions
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Reads a single character from the keyboard.
 * @return The character read from the input.
 */
static char getChar()
{
#if defined(_WIN32)
    return _getch();
#else
    struct termios oldt, newt;
    char           ch;
    tcgetattr(STDIN_FILENO, &oldt); // Get current terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);               // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);        // Apply new settings
    ssize_t bytesRead = read(STDIN_FILENO, &ch, 1); // Read one character
    if (bytesRead != 1)
    {
        // Handle error or unexpected input length
        std::cout << "\n[Error] Failed to read input stream! Expected 1 character got " << bytesRead << ".\n";
        exit(EXIT_FAILURE);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings
    return ch;
#endif // defined(_WIN32)
}

/**
 * @brief Checks whether the standard input is an interactive terminal.
 * @return True when a user can answer prompts (false in CI, with redirected or piped input).
 */
static bool isInteractive()
{
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif // defined(_WIN32)
}

/**
 * @brief Waits until the user presses Enter.
 * @param prompt Message displayed before waiting for input.
 */
static void waitForEnter(const std::string &prompt)
{
#if defined(_WIN32)
    static constexpr char ENTER = '\r';
#else
    static constexpr char ENTER = '\n';
#endif // defined(_WIN32)

    char ch;

    std::cout << prompt;

#if !defined(_WIN32)
    std::cout << std::flush;
#endif // !defined(_WIN32)

    while (true)
    {
        ch = getChar(); // Get a single character from the user

        // If Enter is pressed, stop input
        if (ch == ENTER)
        {
            std::cout << '\n'; // Add a new line after input is confirmed
            break;             // Valid input, exit the loop
        }
        // Ignore non-digit characters (other than enter)
    }
}

#pragma endregion

#pragma region TEST_DATA

/////////////////////////////////////////////////////////////////////////////
// Test Data
/////////////////////////////////////////////////////////////////////////////

/// @brief Structure holding a single set of test data.
struct Dataset
{
#pragma region FIELDS

    /////////////////////////////////////////////////////////////////////////////
    // Fields
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Name/description of the dataset (e.g., "Flat JSON").
    std::string label;

    /// @brief Generated source code string to be parsed.
    std::string source;

#pragma endregion
};

/**
 * @brief Generates a flat JSON array with `count` small objects. Tests the parsing performance on wide, shallow ASTs.
 * @param count Number of objects in the array.
 * @return Generated source string.
 */
static std::string generateFlatJson(const std::size_t count)
{
    std::string json;
    json.reserve(count * 48 + 2);
    json += "[";
    for (std::size_t i = 0; i < count; ++i)
    {
        json += "{\"id\": " + std::to_string(i) + ", \"name\": \"Benchmark\", \"active\": true}";
        if (i + 1 < count)
        {
            json += ",";
        }
    }
    json += "]";
    return json;
}

/**
 * @brief Generates a deeply nested JSON object of the given depth. Tests memory/stack performance on narrow, deep ASTs.
 * @param depth Depth of nested brackets.
 * @return Generated source string.
 */
static std::string generateNestedJson(const std::size_t depth)
{
    std::string prefix;
    std::string suffix;
    prefix.reserve(depth * 8);
    suffix.reserve(depth);
    for (std::size_t i = 0; i < depth; ++i)
    {
        prefix += "{\"n\": ";
        suffix += "}";
    }
    prefix += "0";
    return prefix + suffix;
}

#pragma endregion

#pragma region STATISTICS

/////////////////////////////////////////////////////////////////////////////
// Statistics
/////////////////////////////////////////////////////////////////////////////

/// @brief Structure storing aggregated timing results for a series of benchmark runs.
struct Stats
{
#pragma region FIELDS

    /////////////////////////////////////////////////////////////////////////////
    // Fields
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Average execution time.
    double avg;

    /// @brief Minimum execution time.
    double min;

    /// @brief Maximum execution time.
    double max;

    /// @brief Median value (resistant to outliers).
    double median;

    /// @brief Standard deviation (indicates result stability/variance).
    double stddev;

#pragma endregion
};

/**
 * @brief Calculates statistics based on raw timing results from multiple iterations.
 * @param times Vector of raw execution times from a benchmark.
 * @return `Stats` object with calculated values.
 */
static Stats computeStats(const std::vector<double> &times)
{
    Stats stats = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    if (times.empty())
    {
        return stats;
    }

    std::vector<double> sorted_times(times);
#if CPP_TS_BENCHMARK_HAS_CXX20
    std::ranges::sort(sorted_times);
#else
    std::sort(sorted_times.begin(), sorted_times.end());
#endif // CPP_TS_BENCHMARK_HAS_CXX20

    const double sum = std::accumulate(times.begin(), times.end(), 0.0);
    stats.avg        = sum / static_cast<double>(times.size());
    stats.min        = sorted_times.front();
    stats.max        = sorted_times.back();

    const std::size_t n = sorted_times.size();
    stats.median        = (n % 2 == 0) ? (sorted_times[n / 2 - 1] + sorted_times[n / 2]) / 2.0 : sorted_times[n / 2];

    double sq_sum = 0.0;
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        const double diff = times[i] - stats.avg;
        sq_sum += diff * diff;
    }
    stats.stddev = std::sqrt(sq_sum / static_cast<double>(times.size()));

    return stats;
}

/**
 * @brief Formats and prints the calculated statistics to the console.
 * @param name Benchmark name.
 * @param times Vector of collected iteration times.
 */
static void printStats(const std::string &name, const std::vector<double> &times)
{
    if (times.empty())
    {
        std::cout << std::left << std::setw(42) << name << " | (no data)\n";
        return;
    }

    const Stats stats = computeStats(times);
    std::cout << std::left << std::setw(42) << name << " | Avg: " << std::fixed << std::setprecision(4) << std::setw(8)
              << stats.avg << " ms"
              << " | Med: " << std::setw(8) << stats.median << " ms"
              << " | Min: " << std::setw(8) << stats.min << " ms"
              << " | Max: " << std::setw(8) << stats.max << " ms"
              << " | Std: " << std::setw(8) << stats.stddev << " ms\n";
}

/**
 * @brief Calculates the percentage performance overhead of the tested implementation (e.g., C++ Wrapper) compared to
 * the baseline (C API). Uses median to avoid anomalies.
 * @param baseline Vector of times for the reference implementation.
 * @param compared Vector of times for the evaluated implementation.
 * @return Percentage difference (positive means a performance drop).
 */
static double overheadPercent(const std::vector<double> &baseline, const std::vector<double> &compared)
{
    Stats base = computeStats(baseline);
    Stats comp = computeStats(compared);
    if (base.median <= 0.0 || baseline.empty() || compared.empty())
    {
        return 0.0;
    }
    return ((comp.median / base.median) - 1.0) * 100.0;
}

/**
 * @brief Prints the calculated percentage overhead in a readable format.
 * @param label Text describing the result.
 * @param percent Overhead percentage value.
 */
static void printPercent(const std::string &label, const double percent)
{
    std::cout << "  " << label << (percent >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << percent << " %\n";
}

#pragma endregion

#pragma region STANDARD_AWARE_FACTORY_HELPERS

/////////////////////////////////////////////////////////////////////////////
// Standard-Aware Factory Helpers
//
// Under C++23 these route through the non-throwing `T::create(...)` factories
// (returning `std::expected<T, ts::Error>`), which the library only
// declares when `CPP_TS_HAS_EXPECTED` is set. Under earlier standards those
// factories are not declared at all, so we fall back to the throwing
// constructors, which have existed since the library's C++11 baseline. A
// failure is normalized to `std::runtime_error` either way, so every call
// site can use the same try/catch style regardless of which branch compiled.
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Safely creates a parser instance for the given language.
 * @param language View to the tree-sitter language.
 * @return Created `ts::Parser`.
 */
static ts::Parser makeParser(ts::LanguageView language)
{
#if CPP_TS_BENCHMARK_HAS_CXX23
    auto result = ts::Parser::create(language);
    if (!result)
    {
        throw std::runtime_error("Failed to create parser: " + result.error().message);
    }
    return std::move(*result);
#else
    return ts::Parser(language);
#endif // CPP_TS_BENCHMARK_HAS_CXX23
}

/**
 * @brief Compiles and creates a Query object from the pattern source string.
 * @param language The language to query against.
 * @param source Query source code (in Scheme syntax used by tree-sitter).
 * @return Compiled `ts::Query`.
 */
static ts::Query makeQuery(ts::LanguageView language, const std::string &source)
{
#if CPP_TS_BENCHMARK_HAS_CXX23
    auto result = ts::Query::create(language, source);
    if (!result)
    {
        throw std::runtime_error("Failed to compile query: " + result.error().message);
    }
    return std::move(*result);
#else
    return ts::Query(language, source);
#endif // CPP_TS_BENCHMARK_HAS_CXX23
}

#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
/**
 * @brief Creates a module Store for the WASM engine.
 * @param engine Pointer to a running WASM engine (e.g., wasmtime).
 * @return Configured `ts::WasmStore`.
 */
static ts::WasmStore makeWasmStore(ts::WasmEngine *engine)
{
#if CPP_TS_BENCHMARK_HAS_CXX23
    auto result = ts::WasmStore::create(engine);
    if (!result)
    {
        throw std::runtime_error("Failed to create WASM store: " + result.error().message);
    }
    return std::move(*result);
#else
    return ts::WasmStore(engine);
#endif // CPP_TS_BENCHMARK_HAS_CXX23
}

/**
 * @brief Creates a parser utilizing WASM-backed language implementation.
 * @param language The WASM language view.
 * @param store Configured store with loaded WASM modules.
 * @return Parser instance.
 */
static ts::Parser makeWasmParser(ts::LanguageView language, ts::WasmStore &store)
{
#if CPP_TS_BENCHMARK_HAS_CXX23
    auto result = ts::Parser::create(language, store);
    if (!result)
    {
        throw std::runtime_error("Failed to create WASM parser: " + result.error().message);
    }
    return std::move(*result);
#else
    return ts::Parser(language, store);
#endif // CPP_TS_BENCHMARK_HAS_CXX23
}
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

#pragma endregion

#pragma region 1_PARSING_BENCHMARKS

/////////////////////////////////////////////////////////////////////////////
// 1. Parsing Benchmarks
/////////////////////////////////////////////////////////////////////////////

// NOTE: both the C API and the wrapper reuse a single parser across all
// iterations (as most real applications do), so the measured time reflects
// only the cost of parsing, not repeated parser allocation/teardown.

/**
 * @brief Measures repetitive AST generation time using the raw C API.
 * @param source Source code (JSON) to parse.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of iterations.
 * @param times Output container collecting timing samples for each iteration.
 */
static void benchmarkCApiParsing(const std::string   &source,
                                 const std::size_t    warmups,
                                 const std::size_t    iterations,
                                 std::vector<double> &times)
{
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;
        if (isTesting)
        {
            watch.reset();
        }

        TSTree *tree = ts_parser_parse_string(parser,
                                              nullptr,
                                              source.c_str(),
                                              static_cast<std::uint32_t>(source.length()));

        if (isTesting)
        {
            times.push_back(watch.read());
        }
        ts_tree_delete(tree);
    }

    ts_parser_delete(parser);
}

/**
 * @brief Measures repetitive AST generation time using the C++ Wrapper to evaluate abstraction overhead.
 * @param source Source code (JSON).
 * @param warmups Number of warm up iterations.
 * @param iterations Number of iterations.
 * @param times Output container for results.
 */
static void benchmarkCPPWrapperParsing(const std::string   &source,
                                       const std::size_t    warmups,
                                       const std::size_t    iterations,
                                       std::vector<double> &times)
{
    ts::Language cpp_lang = ts::Language(tree_sitter_json());
    ts::Parser   parser   = makeParser(cpp_lang.getView());

    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;
        if (isTesting)
        {
            watch.reset();
        }

        ts::Tree tree = parser.parseString(source);

        if (isTesting)
        {
            times.push_back(watch.read());
        }
    }
}

/**
 * @brief Benchmarks parsing using the C and C++ APIs.
 * @param source Source code to parse.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param c_times Measured C API execution times.
 * @param cpp_times Measured C++ API execution times.
 */
static void benchmarkInterleavedParsing(const std::string   &source,
                                        const std::size_t    warmups,
                                        const std::size_t    iterations,
                                        std::vector<double> &c_times,
                                        std::vector<double> &cpp_times)
{
    TSParser *c_parser = ts_parser_new();
    ts_parser_set_language(c_parser, tree_sitter_json());

    ts::Language cpp_lang   = ts::Language(tree_sitter_json());
    ts::Parser   cpp_parser = makeParser(cpp_lang.getView());

    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        // 1. C API
        if (isTesting)
        {
            watch.reset();
        }

        TSTree *tree1 = ts_parser_parse_string(c_parser,
                                               nullptr,
                                               source.c_str(),
                                               static_cast<std::uint32_t>(source.length()));

        if (isTesting)
        {
            c_times.push_back(watch.read());
        }
        ts_tree_delete(tree1);

        // 2. C++ Wrapper
        if (isTesting)
        {
            watch.reset();
        }

        ts::Tree tree2 = cpp_parser.parseString(source);

        if (isTesting)
        {
            cpp_times.push_back(watch.read());
        }
    }
    ts_parser_delete(c_parser);
}

#pragma endregion

#pragma region 2_TRAVERSAL_BENCHMARKS

/////////////////////////////////////////////////////////////////////////////
// 2. Traversal Benchmarks
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Measures manual full tree traversal (DFS) using `TSTreeCursor` from the raw C API.
 * @param tree AST generated by tree-sitter.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of traversal iterations.
 * @param times Output container for results.
 */
static void benchmarkCApiTraversal(TSTree              *tree,
                                   const std::size_t    warmups,
                                   const std::size_t    iterations,
                                   std::vector<double> &times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        TSNode       root   = ts_tree_root_node(tree);
        TSTreeCursor cursor = ts_tree_cursor_new(root);

        const bool isTesting = i >= warmups;

        if (isTesting)
        {
            watch.reset();
        }
        bool reached_root = false;
        while (!reached_root)
        {
            TSNode node = ts_tree_cursor_current_node(&cursor);
            // Protection against loop removal by the optimizer.
            volatile const void *id = node.id;
            (void)id;

            if (ts_tree_cursor_goto_first_child(&cursor))
            {
                continue;
            }

            if (ts_tree_cursor_goto_next_sibling(&cursor))
            {
                continue;
            }

            while (true)
            {
                if (!ts_tree_cursor_goto_parent(&cursor))
                {
                    reached_root = true;
                    break;
                }
                else if (ts_tree_cursor_goto_next_sibling(&cursor))
                {
                    break;
                }
            }
        }
        if (isTesting)
        {
            times.push_back(watch.read());
        }
        ts_tree_cursor_delete(&cursor);
    }
}

/**
 * @brief Analogous DFS traversal benchmark, but operating on `ts::TreeCursor` and `ts::Node` objects in C++.
 * @param tree C++ wrapper tree object.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of iterations.
 * @param times Output container.
 */
static void benchmarkCPPWrapperTraversal(const ts::Tree      &tree,
                                         const std::size_t    warmups,
                                         const std::size_t    iterations,
                                         std::vector<double> &times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        ts::TreeCursor cursor = tree.getRootNode().getCursor();

        const bool isTesting = i >= warmups;

        if (isTesting)
        {
            watch.reset();
        }
        bool reached_root = false;
        while (!reached_root)
        {
            ts::Node node = cursor.getCurrentNode();
            // Protection against loop removal by the optimizer.
            volatile const void *id = reinterpret_cast<const void *>(node.getID());
            (void)id;

            if (cursor.gotoFirstChild())
            {
                continue;
            }

            if (cursor.gotoNextSibling())
            {
                continue;
            }

            while (true)
            {
                if (!cursor.gotoParent())
                {
                    reached_root = true;
                    break;
                }
                else if (cursor.gotoNextSibling())
                {
                    break;
                }
            }
        }
        if (isTesting)
        {
            times.push_back(watch.read());
        }
    }
}

/**
 * @brief Benchmarks the high-level `ts::visit` convenience helper (visitor with lambda). Puts extra load on the
 * callback mechanism.
 * @param tree C++ wrapper tree object.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of iterations.
 * @param times Output container.
 */
static void benchmarkCPPWrapperVisit(const ts::Tree      &tree,
                                     const std::size_t    warmups,
                                     const std::size_t    iterations,
                                     std::vector<double> &times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;
        if (isTesting)
        {
            watch.reset();
        }
        ts::visit(tree.getRootNode(),
                  [](ts::Node node) -> bool
                  {
                      // Protection against loop removal by the optimizer.
                      volatile const void *id = reinterpret_cast<const void *>(node.getID());
                      (void)id;
                      return false; // Never stop early - visit every node, like the manual traversals above.
                  });
        if (isTesting)
        {
            times.push_back(watch.read());
        }
    }
}

/**
 * @brief Benchmarks tree traversal using the C and C++ APIs.
 * @param c_tree Tree traversed using the C API.
 * @param cpp_tree Tree traversed using the C++ API.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param c_times Measured C API execution times.
 * @param cpp_times Measured C++ API execution times.
 */
static void benchmarkInterleavedTraversal(TSTree              *c_tree,
                                          const ts::Tree      &cpp_tree,
                                          const std::size_t    warmups,
                                          const std::size_t    iterations,
                                          std::vector<double> &c_times,
                                          std::vector<double> &cpp_times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        // 1. C API
        TSNode       root1     = ts_tree_root_node(c_tree);
        TSTreeCursor c_cursor1 = ts_tree_cursor_new(root1);
        if (isTesting)
        {
            watch.reset();
        }
        bool reached_root = false;
        while (!reached_root)
        {
            TSNode node = ts_tree_cursor_current_node(&c_cursor1);
            // Protection against loop removal by the optimizer.
            volatile const void *id = node.id;
            (void)id;

            if (ts_tree_cursor_goto_first_child(&c_cursor1))
            {
                continue;
            }

            if (ts_tree_cursor_goto_next_sibling(&c_cursor1))
            {
                continue;
            }

            while (true)
            {
                if (!ts_tree_cursor_goto_parent(&c_cursor1))
                {
                    reached_root = true;
                    break;
                }
                else if (ts_tree_cursor_goto_next_sibling(&c_cursor1))
                {
                    break;
                }
            }
        }

        if (isTesting)
        {
            c_times.push_back(watch.read());
        }
        ts_tree_cursor_delete(&c_cursor1);

        // 2. C++ Wrapper
        ts::TreeCursor cpp_cursor = cpp_tree.getRootNode().getCursor();
        if (isTesting)
        {
            watch.reset();
        }
        reached_root = false;
        while (!reached_root)
        {
            ts::Node node = cpp_cursor.getCurrentNode();
            // Protection against loop removal by the optimizer.
            volatile const void *id = reinterpret_cast<const void *>(node.getID());
            (void)id;
            if (cpp_cursor.gotoFirstChild())
            {
                continue;
            }

            if (cpp_cursor.gotoNextSibling())
            {
                continue;
            }

            while (true)
            {
                if (!cpp_cursor.gotoParent())
                {
                    reached_root = true;
                    break;
                }
                else if (cpp_cursor.gotoNextSibling())
                {
                    break;
                }
            }
        }

        if (isTesting)
        {
            cpp_times.push_back(watch.read());
        }
    }
}

#pragma endregion

#pragma region 3_QUERY_MATCHING_BENCHMARKS

/////////////////////////////////////////////////////////////////////////////
// 3. Query Matching Benchmarks
/////////////////////////////////////////////////////////////////////////////

/// @brief Pattern capturing all 'string' nodes and assigning them to the '@value' tag.
static constexpr const char *const QUERY_SOURCE = "(string) @value";

/**
 * @brief Executes a structural query against the tree and counts matches using the raw C API.
 * @param tree AST being queried.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of iterations.
 * @param times Execution times (excludes query compilation time).
 * @return Total number of matches found in a single pass.
 */
static std::size_t benchmarkCApiQuery(TSTree              *tree,
                                      const std::size_t    warmups,
                                      const std::size_t    iterations,
                                      std::vector<double> &times)
{
    std::uint32_t error_offset = 0;
    TSQueryError  error_type   = TSQueryErrorNone;
    TSQuery      *query        = ts_query_new(tree_sitter_json(),
                                              QUERY_SOURCE,
                                              static_cast<std::uint32_t>(std::strlen(QUERY_SOURCE)),
                                              &error_offset,
                                              &error_type);
    if (query == nullptr)
    {
        std::cout << "  [C API] Failed to compile query (error offset " << error_offset << ")\n";
        return 0;
    }

    Stopwatch   watch;
    std::size_t last_match_count = 0;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        TSQueryCursor *cursor = ts_query_cursor_new();
        ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

        if (isTesting)
        {
            watch.reset();
        }

        std::size_t  match_count = 0;
        TSQueryMatch match;
        while (ts_query_cursor_next_match(cursor, &match))
        {
            ++match_count;
        }

        if (isTesting)
        {
            times.push_back(watch.read());
        }

        last_match_count = match_count;
        ts_query_cursor_delete(cursor);
    }

    ts_query_delete(query);
    return last_match_count;
}

/**
 * @brief Tests query execution performance using the `ts::Query` abstraction in the C++ wrapper.
 * @param tree C++ wrapper tree.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of iterations.
 * @param times Output container for timing samples.
 * @return Total number of matches found.
 */
static std::size_t benchmarkCPPWrapperQuery(const ts::Tree      &tree,
                                            const std::size_t    warmups,
                                            const std::size_t    iterations,
                                            std::vector<double> &times)
{
    std::size_t last_match_count = 0;
    try
    {
        ts::Query query = makeQuery(tree.getLanguage(), QUERY_SOURCE);

        Stopwatch watch;
        for (std::size_t i = 0; i < warmups + iterations; ++i)
        {
            const bool isTesting = i >= warmups;

            ts::QueryCursor cursor;
            cursor.exec(query, tree.getRootNode());

            if (isTesting)
            {
                watch.reset();
            }

            std::size_t    match_count = 0;
            ts::QueryMatch match;
            while (cursor.nextMatch(match))
            {
                ++match_count;
            }

            if (isTesting)
            {
                times.push_back(watch.read());
            }

            last_match_count = match_count;
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << "  [C++ Wrapper] Failed to compile query: " << ex.what() << "\n";
    }

    return last_match_count;
}

/**
 * @brief Benchmarks query execution using the C and C++ APIs.
 * @param c_tree Tree queried using the C API.
 * @param cpp_tree Tree queried using the C++ API.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param c_times Measured C API execution times.
 * @param cpp_times Measured C++ API execution times.
 * @return Number of matches found by the query.
 */
static std::size_t benchmarkInterleavedQuery(TSTree              *c_tree,
                                             const ts::Tree      &cpp_tree,
                                             const std::size_t    warmups,
                                             const std::size_t    iterations,
                                             std::vector<double> &c_times,
                                             std::vector<double> &cpp_times)
{
    std::uint32_t error_offset = 0;
    TSQueryError  error_type   = TSQueryErrorNone;
    TSQuery      *c_query      = ts_query_new(tree_sitter_json(),
                                              QUERY_SOURCE,
                                              static_cast<std::uint32_t>(std::strlen(QUERY_SOURCE)),
                                              &error_offset,
                                              &error_type);

    ts::Query cpp_query = makeQuery(cpp_tree.getLanguage(), QUERY_SOURCE);

    std::size_t last_match_count = 0;
    Stopwatch   watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        // 1. C API
        TSQueryCursor *c_cursor = ts_query_cursor_new();
        ts_query_cursor_exec(c_cursor, c_query, ts_tree_root_node(c_tree));

        if (isTesting)
        {
            watch.reset();
        }

        CPP_TS_MAYBE_UNUSED std::size_t match_c = 0;
        TSQueryMatch                    m;
        while (ts_query_cursor_next_match(c_cursor, &m))
        {
            ++match_c;
        }

        if (isTesting)
        {
            c_times.push_back(watch.read());
        }
        ts_query_cursor_delete(c_cursor);

        // 2. C++ API
        ts::QueryCursor cpp_cursor;
        cpp_cursor.exec(cpp_query, cpp_tree.getRootNode());

        if (isTesting)
        {
            watch.reset();
        }

        std::size_t    match_cpp = 0;
        ts::QueryMatch m_cpp;
        while (cpp_cursor.nextMatch(m_cpp))
        {
            ++match_cpp;
        }

        if (isTesting)
        {
            cpp_times.push_back(watch.read());
        }

        last_match_count = match_cpp;
    }

    ts_query_delete(c_query);
    return last_match_count;
}

#pragma endregion

#pragma region 4_INCREMENTAL_BENCHMARKS

/////////////////////////////////////////////////////////////////////////////
// 4. Incremental (Edit + Reparse) Benchmarks
/////////////////////////////////////////////////////////////////////////////

/// @brief Data required to simulate text editing and trigger incremental parsing (partial AST update).
struct EditPlan
{
#pragma region FIELDS

    /////////////////////////////////////////////////////////////////////////////
    // Fields
    /////////////////////////////////////////////////////////////////////////////

    /// @brief Final source string after insertion.
    std::string modified_source;

    /// @brief Edit instructions and offsets for the C API.
    TSInputEdit c_edit;

    /// @brief Edit instruction object for the C++ wrapper.
    ts::InputEdit cpp_edit;

#pragma endregion
};

/**
 * @brief Prepares a text edit—appending a new element to the end of a non-empty, single-line flat JSON array. Computes
 * offsets for the parser.
 * @param original_source Initial source string.
 * @return Populated AST update plan (`EditPlan`).
 */
static EditPlan buildAppendEdit(const std::string &original_source)
{
    static const std::string insertion = ",{\"id\": 999999, \"name\": \"Incremental\", \"active\": false}";

    // The generated datasets are single-line, so byte offsets and column offsets coincide (row is always 0).
    const std::uint32_t insert_at = static_cast<std::uint32_t>(original_source.length()
                                                               - 1); // just before the final ']'

    EditPlan plan;
    plan.modified_source = original_source;
    plan.modified_source.insert(insert_at, insertion);

    const std::uint32_t new_end = static_cast<std::uint32_t>(insert_at + insertion.length());

    TSPoint start_point{ 0u, insert_at };
    TSPoint old_end_point{ 0u, insert_at };
    TSPoint new_end_point{ 0u, new_end };

    plan.c_edit   = TSInputEdit{ insert_at, insert_at, new_end, start_point, old_end_point, new_end_point };
    plan.cpp_edit = ts::InputEdit(insert_at,
                                  insert_at,
                                  new_end,
                                  ts::Point(0u, insert_at),
                                  ts::Point(0u, insert_at),
                                  ts::Point(0u, new_end));

    return plan;
}

/**
 * @brief Measures the C API time to incrementally reparse an AST. Applies an edit to the old tree and passes it as a
 * reference to parse the new source.
 * @param original_source Original source code.
 * @param modified_source Updated source code.
 * @param edit Edit configurations (byte/point offsets).
 * @param warmups Number of warm up iterations.
 * @param iterations Number of reparse cycles.
 * @param times Output container for times.
 */
static void benchmarkCApiIncrementalReparse(const std::string   &original_source,
                                            const std::string   &modified_source,
                                            const TSInputEdit   &edit,
                                            const std::size_t    warmups,
                                            const std::size_t    iterations,
                                            std::vector<double> &times)
{
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        TSTree *base_tree = ts_parser_parse_string(parser,
                                                   nullptr,
                                                   original_source.c_str(),
                                                   static_cast<std::uint32_t>(original_source.length()));
        ts_tree_edit(base_tree, &edit);

        if (isTesting)
        {
            watch.reset();
        }

        TSTree *new_tree = ts_parser_parse_string(parser,
                                                  base_tree,
                                                  modified_source.c_str(),
                                                  static_cast<std::uint32_t>(modified_source.length()));

        if (isTesting)
        {
            times.push_back(watch.read());
        }

        ts_tree_delete(new_tree);
        ts_tree_delete(base_tree);
    }

    ts_parser_delete(parser);
}

/**
 * @brief Analogous benchmark for incremental tree reparsing using the C++ wrapper.
 * @param original_source Old source code.
 * @param modified_source Updated source code.
 * @param edit Edit configuration.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of execution cycles.
 * @param times Times.
 */
static void benchmarkCPPWrapperIncrementalReparse(const std::string   &original_source,
                                                  const std::string   &modified_source,
                                                  const ts::InputEdit &edit,
                                                  const std::size_t    warmups,
                                                  const std::size_t    iterations,
                                                  std::vector<double> &times)
{
    ts::Language cpp_lang = ts::Language(tree_sitter_json());
    ts::Parser   parser   = makeParser(cpp_lang.getView());

    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        ts::Tree base_tree = parser.parseString(original_source);
        base_tree.edit(edit);

        if (isTesting)
        {
            watch.reset();
        }

        ts::Tree new_tree = parser.parseString(modified_source, std::ref(base_tree));

        if (isTesting)
        {
            times.push_back(watch.read());
        }
    }
}

/**
 * @brief Benchmarks incremental reparsing using the C and C++ APIs.
 * @param original_source Original source code.
 * @param modified_source Modified source code.
 * @param c_edit Edit applied using the C API.
 * @param cpp_edit Edit applied using the C++ API.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param c_times Measured C API execution times.
 * @param cpp_times Measured C++ API execution times.
 */
static void benchmarkInterleavedIncrementalReparse(const std::string   &original_source,
                                                   const std::string   &modified_source,
                                                   const TSInputEdit   &c_edit,
                                                   const ts::InputEdit &cpp_edit,
                                                   const std::size_t    warmups,
                                                   const std::size_t    iterations,
                                                   std::vector<double> &c_times,
                                                   std::vector<double> &cpp_times)
{
    TSParser *c_parser = ts_parser_new();
    ts_parser_set_language(c_parser, tree_sitter_json());

    ts::Language cpp_lang   = ts::Language(tree_sitter_json());
    ts::Parser   cpp_parser = makeParser(cpp_lang.getView());
    Stopwatch    watch;

    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        // 1. C API
        TSTree *c_base = ts_parser_parse_string(c_parser,
                                                nullptr,
                                                original_source.c_str(),
                                                static_cast<std::uint32_t>(original_source.length()));
        ts_tree_edit(c_base, &c_edit);
        if (isTesting)
        {
            watch.reset();
        }

        TSTree *c_new = ts_parser_parse_string(c_parser,
                                               c_base,
                                               modified_source.c_str(),
                                               static_cast<std::uint32_t>(modified_source.length()));

        if (isTesting)
        {
            c_times.push_back(watch.read());
        }
        ts_tree_delete(c_new);
        ts_tree_delete(c_base);

        // 2. C++ API
        ts::Tree cpp_base = cpp_parser.parseString(original_source);
        cpp_base.edit(cpp_edit);

        if (isTesting)
        {
            watch.reset();
        }

        ts::Tree cpp_new = cpp_parser.parseString(modified_source, std::ref(cpp_base));

        if (isTesting)
        {
            cpp_times.push_back(watch.read());
        }
    }
    ts_parser_delete(c_parser);
}

#pragma endregion

#pragma region 5_TREE_COPY_BENCHMARKS

/////////////////////////////////////////////////////////////////////////////
// 5. Tree Copy Benchmarks
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Measures the cost of copying a tree (deep cloning of the pointer handle) using the raw C API.
 * @param tree The AST instance.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of copies.
 * @param times Times.
 */
static void benchmarkCApiTreeCopy(TSTree              *tree,
                                  const std::size_t    warmups,
                                  const std::size_t    iterations,
                                  std::vector<double> &times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        if (isTesting)
        {
            watch.reset();
        }

        TSTree *copy = ts_tree_copy(tree);

        if (isTesting)
        {
            times.push_back(watch.read());
        }
        ts_tree_delete(copy);
    }
}

/**
 * @brief Measures the cost of calling the `.copy()` method provided by the C++ wrapper.
 * @param tree C++ wrapper AST instance.
 * @param warmups Number of warm up iterations.
 * @param iterations Number of copies.
 * @param times Times.
 */
static void benchmarkCPPWrapperTreeCopy(const ts::Tree      &tree,
                                        const std::size_t    warmups,
                                        const std::size_t    iterations,
                                        std::vector<double> &times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        if (isTesting)
        {
            watch.reset();
        }

        ts::Tree copy = tree.copy();

        if (isTesting)
        {
            times.push_back(watch.read());
        }
    }
}

/**
 * @brief Benchmarks tree copying using the C and C++ APIs.
 * @param c_tree Tree to copy using the C API.
 * @param cpp_tree Tree to copy using the C++ API.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param c_times Measured C API execution times.
 * @param cpp_times Measured C++ API execution times.
 */
static void benchmarkInterleavedTreeCopy(TSTree              *c_tree,
                                         const ts::Tree      &cpp_tree,
                                         const std::size_t    warmups,
                                         const std::size_t    iterations,
                                         std::vector<double> &c_times,
                                         std::vector<double> &cpp_times)
{
    Stopwatch watch;
    for (std::size_t i = 0; i < warmups + iterations; ++i)
    {
        const bool isTesting = i >= warmups;

        // 1. C API
        if (isTesting)
        {
            watch.reset();
        }

        TSTree *copy = ts_tree_copy(c_tree);

        if (isTesting)
        {
            c_times.push_back(watch.read());
        }
        ts_tree_delete(copy);

        // 2. C++ API
        if (isTesting)
        {
            watch.reset();
        }

        ts::Tree copy_cpp = cpp_tree.copy();

        if (isTesting)
        {
            cpp_times.push_back(watch.read());
        }
    }
}

#pragma endregion

#pragma region 6_WASM_BENCHMARKS

/////////////////////////////////////////////////////////////////////////////
// 6. WASM Benchmarks
/////////////////////////////////////////////////////////////////////////////

#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)

/**
 * @brief Benchmarks parsing using the C API with a WASM parser.
 * @param source Source code to parse.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param times Measured execution times.
 */
static void benchmarkCApiWasmParsing(const std::string   &source,
                                     const std::size_t    warmups,
                                     const std::size_t    iterations,
                                     std::vector<double> &times)
{
#if CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    // C++ Api Wasm Engine
    wasmtime::Engine engineCpp;
    wasm_engine_t   *engine = engineCpp.capi();
#else
    // C Api Wasm Engine
    wasm_engine_t *engine = wasm_engine_new();
#endif // CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED

    if (engine == nullptr)
    {
        std::cout << "  Could not create a WASM engine - skipping WASM benchmarks.\n";
        return;
    }

    // Reading a WASM file for the C API
    std::vector<char> wasm_buffer;
    {
        std::ifstream wasm_file(CPP_TS_BENCHMARK_WASM_JSON_PATH, std::ios::binary);
        if (!wasm_file.is_open())
        {
            std::cout << "  Skipped: Failed to open WASM file.\n";
            wasm_engine_delete(engine);
            return;
        }
        wasm_file.seekg(0, std::ios::end);
        wasm_buffer.resize(static_cast<std::size_t>(wasm_file.tellg()));
        wasm_file.seekg(0, std::ios::beg);
        wasm_file.read(wasm_buffer.data(), static_cast<std::streamsize>(wasm_buffer.size()));
    }

    try
    {
        // C Api Initialization
        TSWasmError  wasm_error{ TSWasmErrorKindNone, nullptr };
        TSWasmStore *c_store = ts_wasm_store_new(engine, &wasm_error);
        if (c_store == nullptr)
        {
            throw std::runtime_error("C API: ts_wasm_store_new failed.");
        }

        const TSLanguage *c_wasm_lang = ts_wasm_store_load_language(c_store,
                                                                    "json",
                                                                    wasm_buffer.data(),
                                                                    static_cast<std::uint32_t>(wasm_buffer.size()),
                                                                    &wasm_error);

        if (c_wasm_lang == nullptr)
        {
            ts_wasm_store_delete(c_store);
            throw std::runtime_error("C API: ts_wasm_store_load_language failed.");
        }

        TSParser *c_parser = ts_parser_new();
        ts_parser_set_wasm_store(c_parser, c_store);
        ts_parser_set_language(c_parser, c_wasm_lang);

        Stopwatch watch;

        for (std::size_t i = 0; i < warmups + iterations; ++i)
        {
            const bool isTesting = i >= warmups;

            if (isTesting)
            {
                watch.reset();
            }

            TSTree *tree = ts_parser_parse_string(c_parser,
                                                  nullptr,
                                                  source.c_str(),
                                                  static_cast<std::uint32_t>(source.length()));

            if (isTesting)
            {
                times.push_back(watch.read());
            }
            ts_tree_delete(tree);
        }

        ts_parser_delete(c_parser);
    }
    catch (const std::exception &ex)
    {
        std::cout << "  Skipped: " << ex.what() << "\n";
        std::cout << "  (Provide a compiled '" CPP_TS_BENCHMARK_WASM_JSON_PATH
                     "' file, e.g. via `tree-sitter build --wasm`, to enable this benchmark;\n"
                     "   override the expected path with -DCPP_TS_BENCHMARK_WASM_JSON_PATH='\"...\"'.)\n";
    }

#if !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    wasm_engine_delete(engine);
#endif // !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
}

/**
 * @brief Benchmarks parsing using the C++ wrapper with a WASM parser.
 * @param source Source code to parse.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param times Measured execution times.
 */
static void benchmarkCPPWrapperWasmParsing(const std::string   &source,
                                           const std::size_t    warmups,
                                           const std::size_t    iterations,
                                           std::vector<double> &times)
{
#if CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    // C++ Api Wasm Engine
    wasmtime::Engine engineCpp;
    wasm_engine_t   *engine = engineCpp.capi();
#else
    // C Api Wasm Engine
    wasm_engine_t *engine = wasm_engine_new();
#endif // CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED

    if (engine == nullptr)
    {
        std::cout << "  Could not create a WASM engine - skipping WASM benchmarks.\n";
        return;
    }

    try
    {
        // C++ Wrapper Initialization
        ts::WasmStore store         = makeWasmStore(engine);
        ts::Language  wasm_language = store.loadLanguage(CPP_TS_BENCHMARK_WASM_JSON_PATH);
        ts::Parser    wasm_parser   = makeWasmParser(wasm_language.getView(), store);

        Stopwatch watch;

        for (std::size_t i = 0; i < warmups + iterations; ++i)
        {
            const bool isTesting = i >= warmups;

            if (isTesting)
            {
                watch.reset();
            }

            ts::Tree tree = wasm_parser.parseString(source);

            if (isTesting)
            {
                times.push_back(watch.read());
            }
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << "  Skipped: " << ex.what() << "\n";
        std::cout << "  (Provide a compiled '" CPP_TS_BENCHMARK_WASM_JSON_PATH
                     "' file, e.g. via `tree-sitter build --wasm`, to enable this benchmark;\n"
                     "   override the expected path with -DCPP_TS_BENCHMARK_WASM_JSON_PATH='\"...\"'.)\n";
    }

#if !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    wasm_engine_delete(engine);
#endif // !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
}

/**
 * @brief Benchmarks WASM parsing using the C and C++ APIs.
 * @param source Source code to parse.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param c_times Measured C API execution times.
 * @param cpp_times Measured C++ API execution times.
 */
static void benchmarkInterleavedWasmParsing(const std::string   &source,
                                            const std::size_t    warmups,
                                            const std::size_t    iterations,
                                            std::vector<double> &c_times,
                                            std::vector<double> &cpp_times)
{
#if CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    // C++ Api Wasm Engine
    wasmtime::Engine engineCpp;
    wasm_engine_t   *engine = engineCpp.capi();
#else
    // C Api Wasm Engine
    wasm_engine_t *engine = wasm_engine_new();
#endif // CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED

    if (engine == nullptr)
    {
        std::cout << "  Could not create a WASM engine - skipping WASM benchmarks.\n";
        return;
    }

    // Reading a WASM file for the C API
    std::vector<char> wasm_buffer;
    {
        std::ifstream wasm_file(CPP_TS_BENCHMARK_WASM_JSON_PATH, std::ios::binary);
        if (!wasm_file.is_open())
        {
            std::cout << "  Skipped: Failed to open WASM file.\n";
            wasm_engine_delete(engine);
            return;
        }
        wasm_file.seekg(0, std::ios::end);
        wasm_buffer.resize(static_cast<std::size_t>(wasm_file.tellg()));
        wasm_file.seekg(0, std::ios::beg);
        wasm_file.read(wasm_buffer.data(), static_cast<std::streamsize>(wasm_buffer.size()));
    }

    try
    {
        // C Api Initialization
        TSWasmError  wasm_error{ TSWasmErrorKindNone, nullptr };
        TSWasmStore *c_store = ts_wasm_store_new(engine, &wasm_error);
        if (c_store == nullptr)
        {
            throw std::runtime_error("C API: ts_wasm_store_new failed.");
        }

        const TSLanguage *c_wasm_lang = ts_wasm_store_load_language(c_store,
                                                                    "json",
                                                                    wasm_buffer.data(),
                                                                    static_cast<std::uint32_t>(wasm_buffer.size()),
                                                                    &wasm_error);

        if (c_wasm_lang == nullptr)
        {
            ts_wasm_store_delete(c_store);
            throw std::runtime_error("C API: ts_wasm_store_load_language failed.");
        }

        TSParser *c_parser = ts_parser_new();
        ts_parser_set_wasm_store(c_parser, c_store);
        ts_parser_set_language(c_parser, c_wasm_lang);

        // C++ Wrapper Initialization
        ts::WasmStore cpp_store    = makeWasmStore(engine);
        ts::Language  cpp_language = cpp_store.loadLanguage(CPP_TS_BENCHMARK_WASM_JSON_PATH);
        ts::Parser    cpp_parser   = makeWasmParser(cpp_language.getView(), cpp_store);

        Stopwatch watch;

        for (std::size_t i = 0; i < warmups + iterations; ++i)
        {
            const bool isTesting = i >= warmups;

            // 1. C Api
            if (isTesting)
            {
                watch.reset();
            }

            TSTree *c_tree = ts_parser_parse_string(c_parser,
                                                    nullptr,
                                                    source.c_str(),
                                                    static_cast<std::uint32_t>(source.length()));

            if (isTesting)
            {
                c_times.push_back(watch.read());
            }
            ts_tree_delete(c_tree);

            // 2. C++ Wrapper
            if (isTesting)
            {
                watch.reset();
            }

            ts::Tree cpp_tree = cpp_parser.parseString(source);

            if (isTesting)
            {
                cpp_times.push_back(watch.read());
            }
        }

        ts_parser_delete(c_parser);
    }
    catch (const std::exception &ex)
    {
        std::cout << "  Skipped: " << ex.what() << "\n";
        std::cout << "  (Provide a compiled '" CPP_TS_BENCHMARK_WASM_JSON_PATH
                     "' file, e.g. via `tree-sitter build --wasm`, to enable this benchmark;\n"
                     "   override the expected path with -DCPP_TS_BENCHMARK_WASM_JSON_PATH='\"...\"'.)\n";
    }

#if !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    wasm_engine_delete(engine);
#endif // !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
}

/**
 * @brief Benchmarks tree traversal using WASM parsers and compares API overhead.
 * @param label Label used to identify the benchmark results.
 * @param source Source code to parse and traverse.
 * @param warmups Number of warm-up iterations.
 * @param iterations Number of measured iterations.
 * @param overhead_summary Summary of measured wrapper overheads.
 */
static void benchmarkWasmTraversal(const std::string                           &label,
                                   const std::string                           &source,
                                   const std::size_t                            warmups,
                                   const std::size_t                            iterations,
                                   std::vector<std::pair<std::string, double>> &overhead_summary)
{
#if CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    // C++ Api Wasm Engine
    wasmtime::Engine engineCpp;
    wasm_engine_t   *engine = engineCpp.capi();
#else
    // C Api Wasm Engine
    wasm_engine_t *engine = wasm_engine_new();
#endif // CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED

    if (engine == nullptr)
    {
        std::cout << "  Could not create a WASM engine - skipping WASM benchmarks.\n";
        return;
    }

    // Reading a WASM file for the C API
    std::vector<char> wasm_buffer;
    {
        std::ifstream wasm_file(CPP_TS_BENCHMARK_WASM_JSON_PATH, std::ios::binary);
        if (!wasm_file.is_open())
        {
            std::cout << "  Skipped: Failed to open WASM file.\n";
            wasm_engine_delete(engine);
            return;
        }
        wasm_file.seekg(0, std::ios::end);
        wasm_buffer.resize(static_cast<std::size_t>(wasm_file.tellg()));
        wasm_file.seekg(0, std::ios::beg);
        wasm_file.read(wasm_buffer.data(), static_cast<std::streamsize>(wasm_buffer.size()));
    }

    try
    {
        // C Api Initialization
        TSWasmError  wasm_error{ TSWasmErrorKindNone, nullptr };
        TSWasmStore *c_store = ts_wasm_store_new(engine, &wasm_error);
        if (c_store == nullptr)
        {
            throw std::runtime_error("C API: ts_wasm_store_new failed.");
        }

        const TSLanguage *c_wasm_lang = ts_wasm_store_load_language(c_store,
                                                                    "json",
                                                                    wasm_buffer.data(),
                                                                    static_cast<std::uint32_t>(wasm_buffer.size()),
                                                                    &wasm_error);

        if (c_wasm_lang == nullptr)
        {
            ts_wasm_store_delete(c_store);
            throw std::runtime_error("C API: ts_wasm_store_load_language failed.");
        }

        TSParser *c_parser = ts_parser_new();
        ts_parser_set_wasm_store(c_parser, c_store);
        ts_parser_set_language(c_parser, c_wasm_lang);

        TSTree *c_wasm_tree = ts_parser_parse_string(c_parser,
                                                     nullptr,
                                                     source.c_str(),
                                                     static_cast<std::uint32_t>(source.length()));

        // C++ Wrapper Initialization
        ts::WasmStore store         = makeWasmStore(engine);
        ts::Language  wasm_language = store.loadLanguage(CPP_TS_BENCHMARK_WASM_JSON_PATH);
        ts::Parser    wasm_parser   = makeWasmParser(wasm_language.getView(), store);

        ts::Tree cpp_wasm_tree = wasm_parser.parseString(source);

        std::vector<double> c_trav, cpp_trav_cursor, cpp_trav_visit;
        benchmarkCApiTraversal(c_wasm_tree, warmups, iterations, c_trav);
        benchmarkCPPWrapperTraversal(cpp_wasm_tree, warmups, iterations, cpp_trav_cursor);
        benchmarkCPPWrapperVisit(cpp_wasm_tree, warmups, iterations, cpp_trav_visit);

        std::vector<double> c_trav_int, cpp_trav_int;
        benchmarkInterleavedTraversal(c_wasm_tree, cpp_wasm_tree, warmups, iterations, c_trav_int, cpp_trav_int);

        printStats("  WASM C API (cursor)", c_trav);
        printStats("  WASM C++ Wrapper (cursor)", cpp_trav_cursor);
        printStats("  WASM C++ Wrapper (ts::visit)", cpp_trav_visit);
        printStats("  [Interleaved] WASM C API (cursor)", c_trav_int);
        printStats("  [Interleaved] WASM C++ Wrapper (cursor)", cpp_trav_int);

        std::cout << "\n";

        const double overhead_cursor     = overheadPercent(c_trav, cpp_trav_cursor);
        const double overhead_visit      = overheadPercent(c_trav, cpp_trav_visit);
        const double overhead_cursor_int = overheadPercent(c_trav_int, cpp_trav_int);

        printPercent("Cursor wrapper overhead (median): ", overhead_cursor);
        printPercent("ts::visit overhead (median):      ", overhead_visit);
        printPercent("[Interleaved] Cursor wrapper overhead (median): ", overhead_cursor_int);
        std::cout << "\n";

        overhead_summary.push_back(std::make_pair(std::string("WASM Traversal (cursor) - ") + label, overhead_cursor));
        overhead_summary.push_back(std::make_pair(std::string("WASM Traversal (visit) - ") + label, overhead_visit));
        overhead_summary.push_back(
                std::make_pair(std::string("[Interleaved] WASM Traversal (cursor) - ") + label, overhead_cursor_int));

        ts_tree_delete(c_wasm_tree);
        ts_parser_delete(c_parser);
    }
    catch (const std::exception &ex)
    {
        std::cout << "  Skipped: " << ex.what() << "\n";
        std::cout << "  (Provide a compiled '" CPP_TS_BENCHMARK_WASM_JSON_PATH
                     "' file, e.g. via `tree-sitter build --wasm`, to enable this benchmark;\n"
                     "   override the expected path with -DCPP_TS_BENCHMARK_WASM_JSON_PATH='\"...\"'.)\n";
    }

#if !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
    wasm_engine_delete(engine);
#endif // !CPP_TS_BENCHMARK_WASMTIME_CXX_ENABLED
}

#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

#pragma endregion

/////////////////////////////////////////////////////////////////////////////
// Main
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Returns the name and version of the compiler used to build the benchmark.
 * @return Compiler description.
 */
static std::string compilerDescription()
{
#if defined(__clang__) && defined(__apple_build_version__)
    return "AppleClang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "."
         + std::to_string(__clang_patchlevel__);
#elif defined(__clang__) && defined(_MSC_VER)
    return "Clang-cl " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "."
         + std::to_string(__clang_patchlevel__);
#elif defined(__clang__)
    return "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "."
         + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
    return "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "."
         + std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_FULL_VER);
#else
    return "Unknown compiler";
#endif
}

/**
 * @brief Returns the name of the operating system the benchmark was built for.
 * @return Operating system name.
 */
static const char *systemDescription()
{
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "MacOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown system";
#endif
}

/**
 * @brief Runs the benchmark suite and prints the results.
 * @param argc Number of arguments.
 * @param argv Arguments: `--no-wait` skips the final "Press Enter" prompt.
 * @return Returns 0.
 */
int main(int argc, char **argv)
{
#pragma region HEADER
    // ---------------------------------------------------------------------
    // Header
    // ---------------------------------------------------------------------

    // `--no-wait` (or a non-interactive standard input, e.g. in CI) skips the final "Press Enter" prompt.
    bool wait_at_exit = isInteractive();
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--no-wait")
        {
            wait_at_exit = false;
        }
    }

    std::cout << "cpp-tree-sitter Benchmark Suite\n";
    std::cout << "System: " << systemDescription() << "\n";
    std::cout << "Compiler: " << compilerDescription() << "\n";
    std::cout << "C++ language level (__cplusplus): " << static_cast<long>(CPP_TS_CXX_LEVEL) << "\n";
    std::cout << "C++ standard detected: "
#if CPP_TS_BENCHMARK_HAS_CXX23
              << "C++23 or newer"
#elif CPP_TS_BENCHMARK_HAS_CXX20
              << "C++20"
#elif CPP_TS_BENCHMARK_HAS_CXX17
              << "C++17"
#else
              << "C++11/C++14"
#endif // CPP_TS_BENCHMARK_HAS_CXX23 / CPP_TS_BENCHMARK_HAS_CXX20 / CPP_TS_BENCHMARK_HAS_CXX17
              << "\n";
#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
    std::cout << "WASM benchmarks: enabled" << "\n";
#else
    std::cout << "WASM benchmarks: disabled" << "\n";
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)
    std::cout << std::string(100, '=') << "\n";

#pragma endregion

#pragma region CONFIGURATION

    // ---------------------------------------------------------------------
    // Configuration
    // ---------------------------------------------------------------------

    // Configuration of the number of test iterations
    static constexpr const std::size_t WARMUP_ITERATIONS      = 20;
    static constexpr const std::size_t PARSE_ITERATIONS       = 200;
    static constexpr const std::size_t TRAVERSAL_ITERATIONS   = 100;
    static constexpr const std::size_t QUERY_ITERATIONS       = 100;
    static constexpr const std::size_t INCREMENTAL_ITERATIONS = 100;
    static constexpr const std::size_t COPY_ITERATIONS        = 100;

    std::vector<std::pair<std::string, double>> overhead_summary;

    std::vector<Dataset> datasets;
    datasets.push_back(Dataset{ "Flat array (100 items)", generateFlatJson(100) });
    datasets.push_back(Dataset{ "Flat array (1,000 items)", generateFlatJson(1000) });
    datasets.push_back(Dataset{ "Flat array (10,000 items)", generateFlatJson(10000) });
    datasets.push_back(Dataset{ "Nested object (depth 500)", generateNestedJson(500) });

#pragma endregion

#pragma region TEST_1_PARSING

    // ---------------------------------------------------------------------
    // 1. Parsing
    // ---------------------------------------------------------------------

    std::cout << "\nPARSING RESULTS:\n";
    for (std::size_t d = 0; d < datasets.size(); ++d)
    {
        const Dataset &ds = datasets[d];
        std::cout << "-- " << ds.label << " (" << ds.source.length() / 1024.0 << " KB) --\n";

        std::vector<double> c_times, cpp_times;
        benchmarkCApiParsing(ds.source, WARMUP_ITERATIONS, PARSE_ITERATIONS, c_times);
        benchmarkCPPWrapperParsing(ds.source, WARMUP_ITERATIONS, PARSE_ITERATIONS, cpp_times);

        std::vector<double> c_times_int, cpp_times_int;
        benchmarkInterleavedParsing(ds.source, WARMUP_ITERATIONS, PARSE_ITERATIONS, c_times_int, cpp_times_int);

        printStats("  C API", c_times);
        printStats("  C++ Wrapper", cpp_times);
        printStats("  [Interleaved] C API", c_times_int);
        printStats("  [Interleaved] C++ Wrapper", cpp_times_int);

        std::cout << "\n";

        const double overhead     = overheadPercent(c_times, cpp_times);
        const double overhead_int = overheadPercent(c_times_int, cpp_times_int);

        printPercent("Wrapper overhead (median): ", overhead);
        printPercent("[Interleaved] Wrapper overhead (median): ", overhead_int);

        std::cout << "\n";
        overhead_summary.push_back(std::make_pair(std::string("Parsing - ") + ds.label, overhead));
        overhead_summary.push_back(std::make_pair(std::string("[Interleaved] Parsing - ") + ds.label, overhead_int));
    }

#pragma endregion

#pragma region TEST_2_TRAVERSAL

    // ---------------------------------------------------------------------
    // 2. Traversal
    // ---------------------------------------------------------------------

    std::cout << "TREE TRAVERSAL RESULTS:\n";

    std::vector<std::size_t> traversal_dataset_indices;
    traversal_dataset_indices.push_back(2); // Flat array (10,000 items)
    traversal_dataset_indices.push_back(3); // Nested object (depth 500)

    for (std::size_t k = 0; k < traversal_dataset_indices.size(); ++k)
    {
        const Dataset &ds = datasets[traversal_dataset_indices[k]];
        std::cout << "-- " << ds.label << " --\n";

        TSParser *c_parser = ts_parser_new();
        ts_parser_set_language(c_parser, tree_sitter_json());
        TSTree *c_tree = ts_parser_parse_string(c_parser,
                                                nullptr,
                                                ds.source.c_str(),
                                                static_cast<std::uint32_t>(ds.source.length()));

        ts::Language cpp_lang   = ts::Language(tree_sitter_json());
        ts::Parser   cpp_parser = makeParser(cpp_lang.getView());
        ts::Tree     cpp_tree   = cpp_parser.parseString(ds.source);

        std::vector<double> c_trav, cpp_trav_cursor, cpp_trav_visit;
        benchmarkCApiTraversal(c_tree, WARMUP_ITERATIONS, TRAVERSAL_ITERATIONS, c_trav);
        benchmarkCPPWrapperTraversal(cpp_tree, WARMUP_ITERATIONS, TRAVERSAL_ITERATIONS, cpp_trav_cursor);
        benchmarkCPPWrapperVisit(cpp_tree, WARMUP_ITERATIONS, TRAVERSAL_ITERATIONS, cpp_trav_visit);

        std::vector<double> c_trav_int, cpp_trav_int;
        benchmarkInterleavedTraversal(c_tree,
                                      cpp_tree,
                                      WARMUP_ITERATIONS,
                                      TRAVERSAL_ITERATIONS,
                                      c_trav_int,
                                      cpp_trav_int);

        printStats("  C API (cursor)", c_trav);
        printStats("  C++ Wrapper (cursor)", cpp_trav_cursor);
        printStats("  C++ Wrapper (ts::visit)", cpp_trav_visit);
        printStats("  [Interleaved] C API (cursor)", c_trav_int);
        printStats("  [Interleaved] C++ Wrapper (cursor)", cpp_trav_int);

        std::cout << "\n";

        const double overhead_cursor     = overheadPercent(c_trav, cpp_trav_cursor);
        const double overhead_visit      = overheadPercent(c_trav, cpp_trav_visit);
        const double overhead_cursor_int = overheadPercent(c_trav_int, cpp_trav_int);

        printPercent("Cursor wrapper overhead (median): ", overhead_cursor);
        printPercent("ts::visit overhead (median):      ", overhead_visit);
        printPercent("[Interleaved] Cursor wrapper overhead (median): ", overhead_cursor_int);
        std::cout << "\n";

        overhead_summary.push_back(std::make_pair(std::string("Traversal (cursor) - ") + ds.label, overhead_cursor));
        overhead_summary.push_back(std::make_pair(std::string("Traversal (visit) - ") + ds.label, overhead_visit));
        overhead_summary.push_back(
                std::make_pair(std::string("[Interleaved] Traversal (cursor) - ") + ds.label, overhead_cursor_int));

        ts_tree_delete(c_tree);
        ts_parser_delete(c_parser);
    }

#pragma endregion

#pragma region TEST_3_QUERY_MATCHING

    // ---------------------------------------------------------------------
    // 3. Query Matching
    // ---------------------------------------------------------------------

    std::cout << "QUERY MATCHING RESULTS (pattern: " << QUERY_SOURCE << "):\n";
    {
        const std::string &source = datasets[2].source; // Flat array (10,000 items)

        TSParser *c_parser = ts_parser_new();
        ts_parser_set_language(c_parser, tree_sitter_json());
        TSTree *c_tree = ts_parser_parse_string(c_parser,
                                                nullptr,
                                                source.c_str(),
                                                static_cast<std::uint32_t>(source.length()));

        ts::Language cpp_lang   = ts::Language(tree_sitter_json());
        ts::Parser   cpp_parser = makeParser(cpp_lang.getView());
        ts::Tree     cpp_tree   = cpp_parser.parseString(source);

        std::vector<double> c_query_times, cpp_query_times;
        const std::size_t   c_matches = benchmarkCApiQuery(c_tree, WARMUP_ITERATIONS, QUERY_ITERATIONS, c_query_times);
        const std::size_t   cpp_matches = benchmarkCPPWrapperQuery(cpp_tree,
                                                                   WARMUP_ITERATIONS,
                                                                   QUERY_ITERATIONS,
                                                                   cpp_query_times);

        std::vector<double> c_query_int, cpp_query_int;
        benchmarkInterleavedQuery(c_tree, cpp_tree, WARMUP_ITERATIONS, QUERY_ITERATIONS, c_query_int, cpp_query_int);

        printStats("  C API", c_query_times);
        printStats("  C++ Wrapper", cpp_query_times);
        printStats("  [Interleaved] C API", c_query_int);
        printStats("  [Interleaved] C++ Wrapper", cpp_query_int);

        std::cout << "\n";

        if (c_matches != cpp_matches)
        {
            std::cout << "  WARNING: match count mismatch (C API: " << c_matches << ", Wrapper: " << cpp_matches
                      << ")\n";
        }
        else
        {
            std::cout << "  Matches found: " << c_matches << "\n";
        }

        const double overhead     = overheadPercent(c_query_times, cpp_query_times);
        const double overhead_int = overheadPercent(c_query_int, cpp_query_int);

        printPercent("Wrapper overhead (median): ", overhead);
        printPercent("[Interleaved] Wrapper overhead (median): ", overhead_int);
        std::cout << "\n";
        overhead_summary.push_back(std::make_pair(std::string("Query matching"), overhead));
        overhead_summary.push_back(std::make_pair(std::string("[Interleaved] Query matching"), overhead_int));

        ts_tree_delete(c_tree);
        ts_parser_delete(c_parser);
    }

#pragma endregion

#pragma region TEST_4_INCREMENTAL_REPARSE

    // ---------------------------------------------------------------------
    // 4. Incremental Reparse
    // ---------------------------------------------------------------------

    std::cout << "INCREMENTAL REPARSE RESULTS:\n";
    {
        const std::string &source = datasets[1].source; // Flat array (1,000 items)
        EditPlan           plan   = buildAppendEdit(source);

        std::vector<double> c_incr_times, cpp_incr_times;
        benchmarkCApiIncrementalReparse(source,
                                        plan.modified_source,
                                        plan.c_edit,
                                        WARMUP_ITERATIONS,
                                        INCREMENTAL_ITERATIONS,
                                        c_incr_times);
        benchmarkCPPWrapperIncrementalReparse(source,
                                              plan.modified_source,
                                              plan.cpp_edit,
                                              WARMUP_ITERATIONS,
                                              INCREMENTAL_ITERATIONS,
                                              cpp_incr_times);

        std::vector<double> c_incr_int, cpp_incr_int;
        benchmarkInterleavedIncrementalReparse(source,
                                               plan.modified_source,
                                               plan.c_edit,
                                               plan.cpp_edit,
                                               WARMUP_ITERATIONS,
                                               INCREMENTAL_ITERATIONS,
                                               c_incr_int,
                                               cpp_incr_int);

        printStats("  C API (incremental)", c_incr_times);
        printStats("  C++ Wrapper (incremental)", cpp_incr_times);
        printStats("  [Interleaved] C API (incremental)", c_incr_int);
        printStats("  [Interleaved] C++ Wrapper (incremental)", cpp_incr_int);

        std::cout << "\n";

        const double overhead     = overheadPercent(c_incr_times, cpp_incr_times);
        const double overhead_int = overheadPercent(c_incr_int, cpp_incr_int);

        printPercent("Wrapper overhead (median): ", overhead);
        printPercent("[Interleaved] Wrapper overhead (median): ", overhead_int);
        overhead_summary.push_back(std::make_pair(std::string("Incremental reparse"), overhead));
        overhead_summary.push_back(std::make_pair(std::string("[Interleaved] Incremental reparse"), overhead_int));

        // Reference point: cost of a full from-scratch reparse of the modified source.
        std::vector<double> c_full_times, cpp_full_times;
        benchmarkCApiParsing(plan.modified_source, WARMUP_ITERATIONS, INCREMENTAL_ITERATIONS, c_full_times);
        benchmarkCPPWrapperParsing(plan.modified_source, WARMUP_ITERATIONS, INCREMENTAL_ITERATIONS, cpp_full_times);

        std::vector<double> c_full_times_int, cpp_full_times_int;
        benchmarkInterleavedParsing(plan.modified_source,
                                    WARMUP_ITERATIONS,
                                    PARSE_ITERATIONS,
                                    c_full_times_int,
                                    cpp_full_times_int);

        printStats("  C API (full reparse)", c_full_times);
        printStats("  C++ Wrapper (full reparse)", cpp_full_times);
        printStats("  [Interleaved] C API (full reparse)", c_full_times_int);
        printStats("  [Interleaved] C++ Wrapper (full reparse)", cpp_full_times_int);

        std::cout << "\n";

        const double speedup_c       = overheadPercent(c_full_times, c_incr_times);
        const double speedup_cpp     = overheadPercent(cpp_full_times, cpp_incr_times);
        const double speedup_c_int   = overheadPercent(c_full_times_int, c_incr_int);
        const double speedup_cpp_int = overheadPercent(cpp_full_times_int, cpp_incr_int);
        printPercent("Incremental vs full reparse (C API, median):       ", speedup_c);
        printPercent("Incremental vs full reparse (C++ Wrapper, median): ", speedup_cpp);
        printPercent("[Interleaved] Incremental vs full reparse (C API, median):       ", speedup_c_int);
        printPercent("[Interleaved] Incremental vs full reparse (C++ Wrapper, median): ", speedup_cpp_int);
        std::cout << "\n";
    }

#pragma endregion

#pragma region TEST_5_TREE_COPY

    // ---------------------------------------------------------------------
    // 5. Tree Copy
    // ---------------------------------------------------------------------

    std::cout << "TREE COPY RESULTS:\n";
    {
        const std::string &source = datasets[2].source; // Flat array (10,000 items)

        TSParser *c_parser = ts_parser_new();
        ts_parser_set_language(c_parser, tree_sitter_json());
        TSTree *c_tree = ts_parser_parse_string(c_parser,
                                                nullptr,
                                                source.c_str(),
                                                static_cast<std::uint32_t>(source.length()));

        ts::Language cpp_lang   = ts::Language(tree_sitter_json());
        ts::Parser   cpp_parser = makeParser(cpp_lang.getView());
        ts::Tree     cpp_tree   = cpp_parser.parseString(source);

        std::vector<double> c_copy_times, cpp_copy_times;
        benchmarkCApiTreeCopy(c_tree, WARMUP_ITERATIONS, COPY_ITERATIONS, c_copy_times);
        benchmarkCPPWrapperTreeCopy(cpp_tree, WARMUP_ITERATIONS, COPY_ITERATIONS, cpp_copy_times);

        std::vector<double> c_copy_int, cpp_copy_int;
        benchmarkInterleavedTreeCopy(c_tree, cpp_tree, WARMUP_ITERATIONS, COPY_ITERATIONS, c_copy_int, cpp_copy_int);

        printStats("  C API", c_copy_times);
        printStats("  C++ Wrapper", cpp_copy_times);
        printStats("  [Interleaved] C API", c_copy_int);
        printStats("  [Interleaved] C++ Wrapper", cpp_copy_int);

        std::cout << "\n";

        const double overhead     = overheadPercent(c_copy_times, cpp_copy_times);
        const double overhead_int = overheadPercent(c_copy_int, cpp_copy_int);

        printPercent("Wrapper overhead (median): ", overhead);
        printPercent("[Interleaved] Wrapper overhead (median): ", overhead_int);
        std::cout << "\n";
        overhead_summary.push_back(std::make_pair(std::string("Tree copy"), overhead));
        overhead_summary.push_back(std::make_pair(std::string("[Interleaved] Tree copy"), overhead_int));

        ts_tree_delete(c_tree);
        ts_parser_delete(c_parser);
    }

#pragma endregion

#pragma region TEST_6_WASM

    // ---------------------------------------------------------------------
    // 6. WASM
    // ---------------------------------------------------------------------

#if defined(CPP_TS_BENCHMARK_FEATURE_WASM)
    std::cout << "\nWASM RESULTS (grammar: " CPP_TS_BENCHMARK_WASM_JSON_PATH "):\n";

    std::cout << "\nWASM PARSING RESULTS:\n";
    for (std::size_t d = 0; d < datasets.size(); ++d)
    {
        const Dataset &ds = datasets[d];
        std::cout << "-- " << ds.label << " (" << ds.source.length() / 1024.0 << " KB) --\n";

        std::vector<double> c_times, cpp_times;
        benchmarkCApiWasmParsing(ds.source, WARMUP_ITERATIONS, PARSE_ITERATIONS, c_times);
        benchmarkCPPWrapperWasmParsing(ds.source, WARMUP_ITERATIONS, PARSE_ITERATIONS, cpp_times);

        std::vector<double> c_times_int, cpp_times_int;
        benchmarkInterleavedWasmParsing(ds.source, WARMUP_ITERATIONS, PARSE_ITERATIONS, c_times_int, cpp_times_int);

        printStats("  WASM C API", c_times);
        printStats("  WASM C++ Wrapper", cpp_times);
        printStats("  [Interleaved] WASM C API", c_times_int);
        printStats("  [Interleaved] WASM C++ Wrapper", cpp_times_int);

        std::cout << "\n";

        const double overhead     = overheadPercent(c_times, cpp_times);
        const double overhead_int = overheadPercent(c_times_int, cpp_times_int);

        printPercent("Wrapper overhead (median): ", overhead);
        printPercent("[Interleaved] Wrapper overhead (median): ", overhead_int);

        std::cout << "\n";
        overhead_summary.push_back(std::make_pair(std::string("WASM Parsing - ") + ds.label, overhead));
        overhead_summary.push_back(
                std::make_pair(std::string("[Interleaved] WASM Parsing - ") + ds.label, overhead_int));
    }

    std::cout << "WASM TREE TRAVERSAL RESULTS:\n";
    for (std::size_t k = 0; k < traversal_dataset_indices.size(); ++k)
    {
        const Dataset &ds = datasets[traversal_dataset_indices[k]];
        std::cout << "-- " << ds.label << " --\n";

        benchmarkWasmTraversal(ds.label, ds.source, WARMUP_ITERATIONS, TRAVERSAL_ITERATIONS, overhead_summary);
    }

#else
    std::cout << "\nWASM benchmarks skipped (enable CPP_TS_FEATURE_WASM to include them).\n";
#endif // defined(CPP_TS_BENCHMARK_FEATURE_WASM)

#pragma endregion

#pragma region SUMMARY

    // ---------------------------------------------------------------------
    // Summary
    // ---------------------------------------------------------------------

    std::cout << std::string(100, '=') << "\n";
    std::cout << "SUMMARY - C++ Wrapper overhead vs raw C API (median-based):\n";
    for (std::size_t i = 0; i < overhead_summary.size(); ++i)
    {
        std::cout << "  " << std::left << std::setw(66) << overhead_summary[i].first << ": "
                  << (overhead_summary[i].second >= 0 ? "+" : "") << std::fixed << std::setprecision(2)
                  << overhead_summary[i].second << " %\n";
    }

#pragma endregion

#pragma region END

    // ---------------------------------------------------------------------
    // End
    // ---------------------------------------------------------------------

    if (wait_at_exit)
    {
        waitForEnter("\nPress Enter to exit...");
    }
    return EXIT_SUCCESS;

#pragma endregion
}
