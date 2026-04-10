#include "pch.hpp"

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// Catch2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// STL
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

extern "C" const TSLanguage *tree_sitter_json();

TEST_CASE("Parser and Tree lifecycle", "[parser]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    std::string code = R"({"key": [1, 2, null]})";

    SECTION("Parsing valid code")
    {
        ts::Tree tree = parser.parseString(code);
        ts::Node root = tree.getRootNode();

        CHECK_FALSE(root.isNull());
        CHECK(root.getType().compare("document") == 0);
        CHECK_FALSE(tree.hasError());
    }

    SECTION("Handling syntax errors")
    {
        ts::Tree tree = parser.parseString("{ \"key\": }"); // Error: No value
        CHECK(tree.hasError());
    }
}

TEST_CASE("Parser Logging", "[parser][debug]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    std::vector<std::string> log_messages;

    parser.setLogger([&](ts::LogType type, const char *message) { log_messages.push_back(message); });

    ts::Tree tree = parser.parseString("[1]");

    CHECK_FALSE(log_messages.empty());

    parser.removeLogger();
    size_t count_after_removal = log_messages.size();
    tree                       = parser.parseString("[1, 2]");

    CHECK(log_messages.size() == count_after_removal);
}

TEST_CASE("Parser Progress Callback", "[parser]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    int              calls = 0;
    ts::ParseOptions options;
    options.progress_callback = [&](ts::ParseState *state)
    {
        ++calls;
        return false;
    };

    // We are generating large, deeply nested JSON that will force the parser to do a lot of work.
    std::string code = "[";
    for (int i = 0; i < 1000; ++i)
    {
        code += "[";
    }
    for (int i = 0; i < 1000; ++i)
    {
        code += "1,";
    }
    code += "1";
    for (int i = 0; i < 1000; ++i)
    {
        code += "]";
    }
    code += "]";

    ts::Input input;
    input.encoding = ts::InputEncoding::UTF8;
    input.read     = [&](uint32_t byte, ts::Point p, uint32_t *read)
    {
        *read = (byte < code.size()) ? 1 : 0;
        return ts::details::make_view(code.c_str() + byte, *read);
    };
#if TEST_HAS_CXX17
    ts::Tree tree = parser.parse(input, {}, options);
#else
    ts::Tree tree = parser.parse(input, nullptr, &options);
#endif
    CHECK(calls > 0);
}

TEST_CASE("Parser Included Ranges", "[parser][range]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    ts::Range r1{ ts::Extent<ts::Point>{ { 0, 0 }, { 0, 5 } }, ts::Extent<uint32_t>{ 0, 5 } };
    ts::Range r2{ ts::Extent<ts::Point>{ { 0, 10 }, { 0, 15 } }, ts::Extent<uint32_t>{ 10, 15 } };

    std::vector<ts::Range> ranges = { r2, r1 };

    CHECK(parser.setIncludedRanges(ranges));
    auto back = parser.getIncludedRanges();

    REQUIRE(back.size() >= 1);
    CHECK(back[0].byte.start == 0);
}

TEST_CASE("Parser Configuration and Language", "[parser]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    SECTION("Language management")
    {
        CHECK(parser.getCurrentLanguage().operator const TSLanguage *() == lang.operator const TSLanguage *());

        CHECK(parser.setLanguage(lang));

        parser.reset();
        CHECK(parser.getCurrentLanguage().operator const TSLanguage *() == lang.operator const TSLanguage *());
    }
}

TEST_CASE("Parser Encoded Strings", "[parser]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    // JSON in UTF-8 format
    std::string utf8_code = "[1, 2, 3]";

    SECTION("Parse UTF-8 explicitly")
    {
        ts::Tree tree = parser.parseStringEncoded(utf8_code, ts::InputEncoding::UTF8);
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getNamedChild(0).getNamedChildCount() == 3);
    }

    SECTION("Parse with Old Tree (Incremental Encoded)")
    {
        ts::Tree old_tree = parser.parseString(utf8_code);

        // Simulate small change in text
        std::string   new_code = "[1, 5, 3]";
        ts::InputEdit edit{ 4,        5,        5, // '2' to '5' (same length)
                            { 0, 4 }, { 0, 5 }, { 0, 5 } };
        old_tree.edit(edit);

#if TEST_HAS_CXX17
        ts::Tree new_tree = parser.parseStringEncoded(new_code, ts::InputEncoding::UTF8, old_tree);
#else
        ts::Tree new_tree = parser.parseStringEncoded(new_code, ts::InputEncoding::UTF8, &old_tree);
#endif
        CHECK_FALSE(new_tree.hasError());
        CHECK(new_tree.getRootNode().getNamedChild(0).getNamedChild(1).getSourceText(new_code) == "5");
    }
}

TEST_CASE("Parser Debugging (DOT Graphs)", "[parser][debug]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);

    SECTION("Enable and Disable DOT graphs")
    {
        const char *temp_file = "parser_graph.dot";
#if defined(_WIN32)
        FILE *f = nullptr;
        fopen_s(&f, temp_file, "w");
#else
        FILE *f = fopen(temp_file, "w");
#endif
        REQUIRE(f != nullptr);

        parser.enableDotGraphs(f);
        ts::Tree tree = parser.parseString("[true]");
        parser.disableDotGraphs();
        fclose(f);

#if defined(_WIN32)
        FILE *f_check = nullptr;
        fopen_s(&f_check, temp_file, "r");
#else
        FILE *f_check = fopen(temp_file, "r");
#endif
        REQUIRE(f_check != nullptr);

        fseek(f_check, 0, SEEK_END);
        long size = ftell(f_check);
        fclose(f_check);

        CHECK(size > 0);
        std::remove(temp_file);
    }
}

TEST_CASE("Parser DOT graphs by filename", "[parser][debug]")
{
    ts::Language lang = tree_sitter_json();
    ts::Parser   parser(lang);
    std::string  filename = "auto_graph.dot";

    SECTION("Enable by path")
    {
        CHECK_NOTHROW(parser.enableDotGraphs(filename));

        ts::Tree tree = parser.parseString("[1, 2, 3]");
        parser.disableDotGraphs();

        std::ifstream f(filename);
        CHECK(f.good());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        CHECK(content.find("digraph") != std::string::npos);
        f.close();

        CHECK(std::remove(filename.c_str()) == 0);
    }
}
