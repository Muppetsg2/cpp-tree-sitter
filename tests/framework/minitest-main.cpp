// Runner of the built-in test framework (minitest.hpp), used when the tests are built without Catch2 (C++11).
//
// Usage: cpp-tree-sitter-tests [options]
//   --list               list the test cases (with their file and tags) and exit
//   --file <name>        run the test cases defined in a file, e.g. --file parser-tests.cpp
//   --tag <tag>          run the test cases with a tag, e.g. --tag [regression]
//   --name <text>        run the test cases whose name contains <text>
//   --verbose            print every test case, not only the failed ones
//
// Filters can be combined (all of them must match). The exit code is 0 when every selected test case passed, 1 when
// any failed or no test case matched the filters, 2 for invalid arguments.

#include "minitest.hpp"

// STL
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#pragma region ARGUMENTS

/////////////////////////////////////////////////////////////////////////////
// Arguments
// Command line options and test case filters.
/////////////////////////////////////////////////////////////////////////////

namespace
{
    /// @brief Command line options of the runner.
    struct Options
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief List the selected test cases instead of running them.
        bool list = false;

        /// @brief Print every test case, not only the failed ones.
        bool verbose = false;

        /// @brief Only run the test cases of this file (base name), if not empty.
        std::string file;

        /// @brief Only run the test cases with this tag, if not empty.
        std::string tag;

        /// @brief Only run the test cases whose name contains this text, if not empty.
        std::string name;

#pragma endregion
    };

    /**
     * @brief Returns the file name of a path.
     * @param path File path.
     * @return Returns the part after the last '/' or '\\'.
     */
    std::string baseName(const std::string &path)
    {
        const std::size_t slash = path.find_last_of("/\\");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    /**
     * @brief Checks whether a test case is selected by the filters.
     * @param test_case Checked test case.
     * @param options Filters.
     * @return Returns true if every filter matches.
     */
    bool matches(const ts_test::TestCase &test_case, const Options &options)
    {
        if (!options.file.empty() && baseName(test_case.file) != options.file)
        {
            return false;
        }
        if (!options.tag.empty() && test_case.tags.find(options.tag) == std::string::npos)
        {
            return false;
        }
        if (!options.name.empty() && test_case.name.find(options.name) == std::string::npos)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Prints the command line of the runner.
     * @param program Name of the executable.
     */
    void printUsage(const char *program)
    {
        std::cout << "Usage: " << program
                  << " [--list] [--file <name.cpp>] [--tag <[tag]>] [--name <text>] [--verbose]\n";
    }

    /**
     * @brief Parses the command line.
     * @param argc Number of arguments.
     * @param argv Arguments.
     * @param options Receives the options.
     * @return Returns false for an unknown or incomplete argument.
     */
    bool parseArguments(int argc, char **argv, Options &options)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument  = argv[i];
            const bool        has_value = i + 1 < argc;

            if (argument == "--list")
            {
                options.list = true;
            }
            else if (argument == "--verbose" || argument == "-v")
            {
                options.verbose = true;
            }
            else if (argument == "--file" && has_value)
            {
                options.file = argv[++i];
            }
            else if (argument == "--tag" && has_value)
            {
                options.tag = argv[++i];
            }
            else if (argument == "--name" && has_value)
            {
                options.name = argv[++i];
            }
            else
            {
                std::cerr << "Unknown or incomplete argument: " << argument << "\n";
                return false;
            }
        }
        return true;
    }
} // namespace

#pragma endregion

#pragma region MAIN

/////////////////////////////////////////////////////////////////////////////
// Main
// Runs the selected test cases (one pass per leaf SECTION) and prints a summary.
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Runs the selected test cases.
 * @param argc Number of arguments.
 * @param argv Arguments (see the top of this file).
 * @return Returns 0 when every selected test case passed, 1 when any failed or none matched, 2 for invalid arguments.
 */
int main(int argc, char **argv)
{
    Options options;
    if (!parseArguments(argc, argv, options))
    {
        printUsage(argv[0]);
        return 2;
    }

    std::size_t selected          = 0;
    std::size_t failed            = 0;
    std::size_t total_assertions  = 0;
    std::size_t failed_assertions = 0;

    for (const ts_test::TestCase &test_case : ts_test::registry())
    {
        if (!matches(test_case, options))
        {
            continue;
        }
        ++selected;

        if (options.list)
        {
            std::cout << test_case.name << "\n    " << baseName(test_case.file) << " " << test_case.tags << "\n";
            continue;
        }

        ts_test::counters()               = ts_test::Counters{ 0, 0 };
        ts_test::SectionTracker &sections = ts_test::sectionTracker();
        sections.reset();
        bool aborted = false;

        // One pass per leaf SECTION (a test case without sections runs once). A pass that completes no section (e.g.
        // a failure before the remaining sections are reached) ends the test case.
        std::size_t completed_before = 0;
        do
        {
            completed_before = sections.completed.size();
            sections.startPass();
            try
            {
                test_case.function();
            }
            catch (const ts_test::RequireFailure &)
            {
                aborted = true;
            }
            catch (...)
            {
                // Counted as a failed assertion, so the summary stays consistent.
                ++ts_test::counters().assertions;
                ++ts_test::counters().failures;
                std::cerr << test_case.file << ":" << test_case.line
                          << ": FAILED:\n  unexpected exception in test case: " << ts_test::currentExceptionMessage()
                          << "\n";
            }
        } while (sections.hasPendingBelow("") && sections.completed.size() > completed_before);

        const ts_test::Counters result = ts_test::counters();
        total_assertions += result.assertions;
        failed_assertions += result.failures;

        if (result.failures > 0)
        {
            ++failed;
            std::cerr << "--> FAILED: " << test_case.name << " (" << result.failures << " of " << result.assertions
                      << " assertions failed" << (aborted ? ", stopped by REQUIRE" : "") << ")\n\n";
        }
        else if (options.verbose)
        {
            std::cout << "passed: " << test_case.name << " (" << result.assertions << " assertions)\n";
        }
    }

    if (options.list)
    {
        std::cout << selected << " test cases\n";
        return 0;
    }

    if (selected == 0)
    {
        std::cerr << "No test cases matched the filters.\n";
        return 1;
    }

    std::cout << (failed == 0 ? "All tests passed" : "Tests FAILED") << " (" << total_assertions << " assertions in "
              << selected << " test cases";
    if (failed > 0)
    {
        std::cout << ", " << failed << " test cases and " << failed_assertions << " assertions failed";
    }
    std::cout << ")\n";

    return failed == 0 ? 0 : 1;
}

#pragma endregion
