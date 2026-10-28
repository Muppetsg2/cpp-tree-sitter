// Parser and Input: languages, parse methods, callbacks (read, decode, logger, progress), included ranges,
// incremental parsing, debugging output and lifecycle.

#include "common.hpp"

// STL
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    ts::Range bytes(std::uint32_t start, std::uint32_t end)
    {
        return ts::Range(ts::Extent<ts::Point>(ts::Point(0, start), ts::Point(0, end)),
                         ts::Extent<std::uint32_t>(start, end));
    }

    std::FILE *openForWriting(const char *path)
    {
#if defined(_WIN32)
        std::FILE *file = nullptr;
        fopen_s(&file, path, "w");
        return file;
#else
        return std::fopen(path, "w");
#endif // defined(_WIN32)
    }
} // namespace

TEST_CASE("Parser language management", "[parser]")
{
    const ts::Language language(tree_sitter_json());

    SECTION("Parser without a language")
    {
        ts::Parser empty;
        CHECK_FALSE(empty.hasLanguage());
        CHECK_FALSE(empty.getCurrentLanguage().isValid());
        CHECK(empty.raw() != nullptr);

        CHECK_THROWS_AS(empty.parseString("{}"), std::logic_error);
        CHECK_THROWS_AS(empty.parseStringEncoded("{}", ts::InputEncoding::UTF8), std::logic_error);
        const std::string source = "{}";
        ts::Input         input(ts_test::ChunkedReader{ &source, 64 }, ts::InputEncoding::UTF8);
        CHECK_THROWS_AS(empty.parse(input), std::logic_error);
    }

    SECTION("Assigning and clearing the language")
    {
        ts::Parser parser;
        CHECK(parser.setLanguage(language));
        CHECK(parser.hasLanguage());
        CHECK(static_cast<const TSLanguage *>(parser.getCurrentLanguage())
              == static_cast<const TSLanguage *>(language));
        CHECK(parser.getCurrentLanguage().getSymbolsCount() == language.getView().getSymbolsCount());

        // reset() keeps the language.
        parser.reset();
        CHECK(parser.hasLanguage());

        // Assigning an invalid language clears the language.
        CHECK(parser.setLanguage(ts::LanguageView()));
        CHECK_FALSE(parser.hasLanguage());
    }

    SECTION("Constructor")
    {
        const ts::Parser parser(language);
        CHECK(parser.hasLanguage());
        CHECK_THROWS_AS(ts::Parser(ts::LanguageView()), std::runtime_error);
    }
}

TEST_CASE("Parser parses strings", "[parser]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);

    SECTION("Valid code")
    {
        const ts::Tree tree = parser.parseString("{\"key\": [1, 2, null]}");
        REQUIRE(tree.isValid());
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getType() == "document");
        CHECK(tree.getRootNode().getNamedChild(0).getType() == "object");
    }

    SECTION("Syntax errors")
    {
        const ts::Tree tree = parser.parseString("{ \"key\": }"); // no value
        REQUIRE(tree.isValid());
        CHECK(tree.hasError());
    }

    SECTION("Empty sources")
    {
        CHECK(parser.parseString("").isValid());
        // A default-constructed view has a null data() pointer, which must not reach the C API.
        CHECK(parser.parseString(ts::StringView()).isValid());
    }
}

TEST_CASE("Parser parses encoded strings", "[parser][encoding]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);

    SECTION("UTF-16 little and big endian")
    {
        // "[1]" in UTF-16 little and big endian.
        const std::string utf16le("[\0"
                                  "1\0"
                                  "]\0",
                                  6);
        const std::string utf16be("\0["
                                  "\0"
                                  "1"
                                  "\0]",
                                  6);

        const ts::Tree little = parser.parseStringEncoded(utf16le, ts::InputEncoding::UTF16LE);
        CHECK_FALSE(little.hasError());
        CHECK(little.getRootNode().getNamedChild(0).getType() == "array");

        const ts::Tree big = parser.parseStringEncoded(utf16be, ts::InputEncoding::UTF16BE);
        CHECK_FALSE(big.hasError());
        CHECK(big.getRootNode().getNamedChild(0).getNamedChildCount() == 1u);
    }

    SECTION("UTF-8 with an old tree")
    {
        const std::string code = "[1, 2, 3]";
        const ts::Tree    tree = parser.parseStringEncoded(code, ts::InputEncoding::UTF8);
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getNamedChild(0).getNamedChildCount() == 3u);

        // "[1, 2, 3]" -> "[1, 5, 3]" (same length)
        ts::Tree          old_tree = tree.copy();
        const std::string new_code = "[1, 5, 3]";
        old_tree.edit(ts::InputEdit(4, 5, 5, ts::Point(0, 4), ts::Point(0, 5), ts::Point(0, 5)));

        const ts::Tree new_tree = parser.parseStringEncoded(new_code, ts::InputEncoding::UTF8, old_tree);
        CHECK_FALSE(new_tree.hasError());
        CHECK(new_tree.getRootNode().getNamedChild(0).getNamedChild(1).getSourceText(new_code) == "5");
    }
}

TEST_CASE("Parser parses an Input", "[parser][input]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    const std::string  source = "[1, 2, 3]";

    SECTION("Read in chunks")
    {
        const ts_test::ChunkedReader reader = { &source, 2 };
        ts::Input                    input(reader, ts::InputEncoding::UTF8);
        CHECK(input.getEncoding() == ts::InputEncoding::UTF8);

        const ts::Tree tree = parser.parse(input);
        REQUIRE(tree.isValid());
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getNamedChild(0).getNamedChildCount() == 3u);
    }

    SECTION("Read callback returning a sized view without writing bytes_read")
    {
        ts::Input input(
                [&source](std::uint32_t byte, ts::Point, std::uint32_t *) -> ts::details::StringViewReturn
                {
                    return byte < source.size()
                                 ? ts::details::StringViewReturn(source.data() + byte, source.size() - byte)
                                 : ts::details::StringViewReturn();
                },
                ts::InputEncoding::UTF8);

        const ts::Tree tree = parser.parse(input);
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getNamedChild(0).getNamedChildCount() == 3u);
    }

    SECTION("Empty parse options behave like a standard parse")
    {
        ts::Input              input(ts_test::ChunkedReader{ &source, 1 }, ts::InputEncoding::UTF8);
        const ts::ParseOptions options;

        const ts::Tree tree = parser.parse(input, nullptr, options);
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getType() == "document");
    }
}

TEST_CASE("Input with a custom decoder", "[parser][input][regression]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    const std::string  source = "[1, 2]";

    std::uint32_t decode_calls = 0;
    ts::Input     input(ts_test::ChunkedReader{ &source, 64 },
                        [&decode_calls](ts::ByteView bytes, std::int32_t *code_point) -> std::uint32_t
                        {
                        ++decode_calls;
                        *code_point = bytes.empty() ? -1 : static_cast<std::int32_t>(bytes[0]);
                        return 1;
                        });

    // Regression: the encoding used to be computed from the moved-from decoder and stayed UTF-8.
    CHECK(input.getEncoding() == ts::InputEncoding::Custom);

    const ts::Tree tree = parser.parse(input);
    REQUIRE(tree.isValid());
    CHECK_FALSE(tree.hasError());
    CHECK(decode_calls > 0u);
    CHECK(tree.getRootNode().getNamedChild(0).getNamedChildCount() == 2u);
}

TEST_CASE("Input accessors", "[parser][input]")
{
    ts::Input input;
    CHECK(input.getEncoding() == ts::InputEncoding::UTF8);
    CHECK_FALSE(static_cast<bool>(input.getRead()));
    CHECK_FALSE(static_cast<bool>(input.getDecode()));

    const std::string source = "[]";
    input.setRead(ts_test::ChunkedReader{ &source, 1 });
    CHECK(static_cast<bool>(input.getRead()));

    // A null callback is ignored.
    input.setRead(ts::Input::ReadFunction());
    CHECK(static_cast<bool>(input.getRead()));

    input.setDecode(
            [](ts::ByteView, std::int32_t *code_point) -> std::uint32_t
            {
                *code_point = 'x';
                return 1;
            });
    CHECK(input.getEncoding() == ts::InputEncoding::Custom);

    // Leaving the custom encoding drops the decoder.
    input.setEncoding(ts::InputEncoding::UTF16LE);
    CHECK_FALSE(static_cast<bool>(input.getDecode()));

    // Moving keeps the configuration.
    ts::Input moved(std::move(input));
    CHECK(moved.getEncoding() == ts::InputEncoding::UTF16LE);
    CHECK(static_cast<bool>(moved.getRead()));

    const TSInput raw = moved;
    CHECK(raw.encoding == TSInputEncodingUTF16LE);
    CHECK(raw.payload == static_cast<void *>(&moved));
}

TEST_CASE("Parser logger", "[parser][logger]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    CHECK_FALSE(parser.hasLogger());

    std::uint32_t parse_messages = 0;
    std::uint32_t lex_messages   = 0;
    parser.setLogger(
            [&parse_messages, &lex_messages](ts::LogType type, const char *message)
            {
                CHECK(message != nullptr);
                ++(type == ts::LogType::Parse ? parse_messages : lex_messages);
            });
    CHECK(parser.hasLogger());

    ts_test::discard(parser.parseString("[1]"));
    CHECK(parse_messages > 0u);
    CHECK(lex_messages > 0u);

    parser.removeLogger();
    CHECK_FALSE(parser.hasLogger());
    const std::uint32_t before = parse_messages;
    ts_test::discard(parser.parseString("[1, 2]"));
    CHECK(parse_messages == before);

    // An empty callable removes the logger too.
    parser.setLogger([](ts::LogType, const char *) {});
    parser.setLogger(ts::Parser::LoggerFunction());
    CHECK_FALSE(parser.hasLogger());
}

TEST_CASE("ParseOptions progress callback accessors", "[parser][options]")
{
    ts::ParseOptions options;
    CHECK_FALSE(static_cast<bool>(options.getProgressCallback()));

    options.setProgressCallback([](ts::ParseState *) { return false; });
    CHECK(static_cast<bool>(options.getProgressCallback()));

    options.setProgressCallback(ts::ParseOptions::ProgressCallbackFunction());
    CHECK_FALSE(static_cast<bool>(options.getProgressCallback()));

    const ts::ParseOptions constructed([](ts::ParseState *) { return false; });
    CHECK(static_cast<bool>(constructed.getProgressCallback()));
}

TEST_CASE("Parser progress callback", "[parser][options]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    // tree-sitter reports progress periodically, so the document has to be large enough.
    const std::string source = ts_test::bigJsonArray(5000);
    ts::Input         input(ts_test::ChunkedReader{ &source, 4096 }, ts::InputEncoding::UTF8);

    SECTION("Called during the parse")
    {
        std::uint32_t          calls = 0;
        const ts::ParseOptions options(
                [&calls, &source](ts::ParseState *state) -> bool
                {
                    ++calls;
                    CHECK(state->current_byte_offset <= source.size());
                    return false;
                });

        const ts::Tree tree = parser.parse(input, nullptr, options);
        CHECK(tree.isValid());
        CHECK(calls > 0u);
        // The options keep their callback.
        CHECK(static_cast<bool>(options.getProgressCallback()));
    }

    SECTION("Returning true cancels the parse")
    {
        // The throwing API reports a missing tree with an invalid Tree, not with an exception.
        const ts::ParseOptions cancel([](ts::ParseState *) { return true; });
        CHECK_FALSE(parser.parse(input, nullptr, cancel).isValid());
        CHECK_FALSE(parser.parse(input, nullptr, &cancel).isValid());

        // The parser stays usable.
        CHECK(parser.parse(input).isValid());
    }
}

TEST_CASE("Parser rethrows exceptions thrown inside callbacks", "[parser][exceptions][regression]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    std::string        source = "[1, 2, 3]";

    SECTION("Read callback")
    {
        ts::Input input([](std::uint32_t, ts::Point, std::uint32_t *) -> ts::details::StringViewReturn
                        { throw std::runtime_error("read failed"); },
                        ts::InputEncoding::UTF8);

        CHECK_THROWS_WITH(parser.parse(input), "read failed");
        // The parser stays usable and the input does not keep the old exception.
        CHECK(parser.parseString(source).isValid());
        CHECK_THROWS_WITH(parser.parse(input), "read failed");
    }

    SECTION("Decode callback")
    {
        ts::Input input(ts_test::ChunkedReader{ &source, 64 },
                        [](ts::ByteView, std::int32_t *) -> std::uint32_t
                        { throw std::invalid_argument("decode failed"); });

        CHECK_THROWS_AS(parser.parse(input), std::invalid_argument);
    }

    SECTION("Logger")
    {
        parser.setLogger([](ts::LogType, const char *) { throw std::runtime_error("logger failed"); });
        CHECK_THROWS_WITH(parser.parseString(source), "logger failed");

        parser.removeLogger();
        CHECK(parser.parseString(source).isValid());
    }

    SECTION("Progress callback")
    {
        // tree-sitter reports progress periodically, so the document has to be large enough.
        source = ts_test::bigJsonArray(5000);
        ts::Input              input(ts_test::ChunkedReader{ &source, 4096 }, ts::InputEncoding::UTF8);
        const ts::ParseOptions options([](ts::ParseState *) -> bool { throw std::runtime_error("progress failed"); });

        CHECK_THROWS_WITH(parser.parse(input, nullptr, options), "progress failed");
    }
}

TEST_CASE("Parser incremental parsing and optional arguments", "[parser][edit]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    ts::Tree           tree = parser.parseString("[1]");

    SECTION("The old tree is accepted in the same ways in every standard")
    {
        CHECK(parser.parseString("[1]", tree).isValid());
        CHECK(parser.parseString("[1]", &tree).isValid());
        CHECK(parser.parseString("[1]", nullptr).isValid());
        CHECK(parser.parseString("[1]", {}).isValid());
        CHECK(parser.parseString("[1]", std::ref(tree)).isValid());
#if CPP_TS_HAS_OPTIONAL
        CHECK(parser.parseString("[1]", std::nullopt).isValid());
#endif // CPP_TS_HAS_OPTIONAL
        CHECK(parser.parseStringEncoded("[1]", ts::InputEncoding::UTF8, tree).isValid());
    }

    SECTION("Reparse after an edit")
    {
        tree.edit(ts::InputEdit(2, 2, 5, ts::Point(0, 2), ts::Point(0, 2), ts::Point(0, 5)));
        const ts::Tree updated = parser.parseString("[1, 2]", tree);
        CHECK(updated.getRootNode().getNamedChild(0).getNamedChildCount() == 2u);
    }
}

TEST_CASE("Parser included ranges", "[parser][range]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);

    SECTION("Only the included ranges are parsed")
    {
        // Only "[2]" (bytes 7..10) of the source is parsed.
        std::vector<ts::Range> ranges;
        ranges.push_back(bytes(7, 10));
        CHECK(parser.setIncludedRanges(ranges));
        REQUIRE(parser.getIncludedRanges().size() == 1u);
        CHECK(parser.getIncludedRanges()[0].byte == ts::Extent<std::uint32_t>(7, 10));

        const std::string source = "ignore [2] ignore";
        const ts::Tree    tree   = parser.parseString(source);
        CHECK(tree.getRootNode().getNamedChild(0).getSourceText(source) == "[2]");
    }

    SECTION("An empty list includes the whole document again")
    {
        const ts::Range narrow = bytes(4, 5);
#if CPP_TS_HAS_SPAN
        REQUIRE(parser.setIncludedRanges(std::span<const ts::Range>(&narrow, 1)));
#else
        REQUIRE(parser.setIncludedRanges(std::vector<ts::Range>(1, narrow)));
#endif // CPP_TS_HAS_SPAN

        CHECK(parser.setIncludedRanges(std::vector<ts::Range>()));
        REQUIRE(parser.getIncludedRanges().size() == 1u);
        CHECK(parser.getIncludedRanges()[0].byte.start == 0u);
        CHECK(parser.getIncludedRanges()[0].byte.end == 0xFFFFFFFFu);

        const std::string code = "[1, 2, 3]";
        const ts::Tree    tree = parser.parseString(code);
        CHECK_FALSE(tree.hasError());
        CHECK(tree.getRootNode().getByteRange().end == code.size());
    }

    SECTION("Unordered and overlapping ranges are sorted and merged")
    {
        // tree-sitter alone would reject them.
        std::vector<ts::Range> ranges;
        ranges.push_back(bytes(20, 30));
        ranges.push_back(bytes(0, 10));
        ranges.push_back(bytes(5, 15));
        CHECK(parser.setIncludedRanges(ranges));

        const std::vector<ts::Range> result = parser.getIncludedRanges();
        REQUIRE(result.size() == 2u);
        CHECK(result[0].byte == ts::Extent<std::uint32_t>(0, 15));
        CHECK(result[1].byte == ts::Extent<std::uint32_t>(20, 30));
    }

    SECTION("Enveloped ranges are merged")
    {
        std::vector<ts::Range> ranges;
        ranges.push_back(bytes(5, 10));
        ranges.push_back(bytes(0, 20));
        CHECK(parser.setIncludedRanges(ranges));

        const std::vector<ts::Range> result = parser.getIncludedRanges();
        REQUIRE(result.size() == 1u);
        CHECK(result[0].byte == ts::Extent<std::uint32_t>(0, 20));
    }
}

#if SIZE_MAX > UINT32_MAX
TEST_CASE("Parser rejects inputs longer than 4GB", "[parser][exceptions]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);

    // A view with an artificial size, nothing of that size is allocated.
    const char           *fake_str      = "fake";
    constexpr std::size_t overflow_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1;
    const ts::StringView  huge_view(fake_str, overflow_size);

    CHECK_THROWS_AS(parser.parseString(huge_view), std::length_error);
    CHECK_THROWS_AS(parser.parseStringEncoded(huge_view, ts::InputEncoding::UTF8), std::length_error);
}
#endif // SIZE_MAX > UINT32_MAX

TEST_CASE("Parser DOT graphs", "[parser][debug]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);

    SECTION("To a FILE")
    {
        const char *path = "cpp-tree-sitter-parser-file.dot";
        std::FILE  *file = openForWriting(path);
        REQUIRE(file != nullptr);
        parser.enableDotGraphs(file); // the parser writes to a duplicate of the descriptor
        ts_test::discard(parser.parseString("[true]"));
        parser.disableDotGraphs();
        std::fclose(file);

        const std::string content = ts_test::readFile(path);
        std::remove(path);
        CHECK_FALSE(content.empty());
    }

    SECTION("To a path")
    {
        const std::string path = "cpp-tree-sitter-parser-path.dot";
        CHECK_NOTHROW(parser.enableDotGraphs(path));
        ts_test::discard(parser.parseString("[1, 2, 3]"));
        parser.disableDotGraphs();

        const std::string content = ts_test::readFile(path.c_str());
        CHECK(std::remove(path.c_str()) == 0);
        CHECK(ts_test::contains(content, "digraph"));
    }

    SECTION("A null FILE is ignored")
    {
        CHECK_NOTHROW(parser.enableDotGraphs(static_cast<std::FILE *>(nullptr)));
        CHECK_NOTHROW(parser.disableDotGraphs());
    }

#if !defined(_WIN32)
    // The MSVC runtime treats _dup of a closed descriptor as an invalid parameter (debug assertion), so POSIX only.
    SECTION("An invalid file descriptor")
    {
        CHECK_THROWS_AS(parser.enableDotGraphs(987654), std::runtime_error);
        CHECK_NOTHROW(parser.disableDotGraphs());
    }
#endif // !defined(_WIN32)
}

TEST_CASE("Parser move and moved-from state", "[parser][lifecycle][regression]")
{
    const ts::Language language(tree_sitter_json());
    ts::Parser         parser(language);
    std::uint32_t      logs = 0;
    parser.setLogger([&logs](ts::LogType, const char *) { ++logs; });

    // The logger keeps working after a move (its payload does not point to the old object).
    ts::Parser moved(std::move(parser));
    CHECK(moved.hasLanguage());
    CHECK(moved.parseString("[1]").isValid());
    CHECK(logs > 0u);

    // Using or assigning from a moved-from parser used to dereference a null TSParser.
    ts::Parser target;
    target = std::move(parser);
    CHECK_FALSE(target.hasLanguage());
    CHECK_NOTHROW(target.reset());
    CHECK_NOTHROW(target.removeLogger());
    CHECK(target.getIncludedRanges().empty());
    CHECK_THROWS_AS(target.parseString("[1]"), std::logic_error);

    // A moved-from parser can be revived by assigning a language.
    CHECK_NOTHROW(parser.reset());
    CHECK(parser.setLanguage(language));
    CHECK(parser.parseString("[2]").isValid());
    CHECK(target.setLanguage(language));
    CHECK(target.parseString("[2]").isValid());
}
