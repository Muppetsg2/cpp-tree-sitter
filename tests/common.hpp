#pragma once
#ifndef CPP_TREE_SITTER_TESTS_COMMON_HPP
#define CPP_TREE_SITTER_TESTS_COMMON_HPP

// Included first by every test file (and precompiled): the test framework, the library, fixtures, helpers hiding API
// differences between standards and printers making failed assertions on library types readable.
//
// Test files are compiled in every standard from C++11 to C++23, so they are written in C++11. Features of newer
// standards are tested under the feature macros of the library (CPP_TS_HAS_STRING_VIEW, CPP_TS_HAS_SPAN,
// CPP_TS_HAS_EXPECTED, ...), so the tests follow exactly what the wrapper enabled.

#include "framework/framework.hpp"

// cpp-tree-sitter
#include <cpp-tree-sitter.hpp>

// STL
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>

#if CPP_TS_HAS_STRING_VIEW
#include <string_view>
#endif // CPP_TS_HAS_STRING_VIEW

#if CPP_TS_HAS_OPTIONAL
#include <optional>
#endif // CPP_TS_HAS_OPTIONAL

#if CPP_TS_HAS_SPAN
#include <span>
#endif // CPP_TS_HAS_SPAN

#if CPP_TS_HAS_EXPECTED
#include <expected>
#endif // CPP_TS_HAS_EXPECTED

#if defined(CPP_TS_FEATURE_WASM)
// The C++ API of wasmtime needs C++17.
#define CPP_TS_TEST_WASMTIME_CXX_ENABLED (CPP_TS_HAS_CXX17 && (CPP_TS_TEST_WASMTIME_VERSION >= 390000))
#if CPP_TS_TEST_WASMTIME_CXX_ENABLED
#include <wasmtime.hh>
#else
#include <wasm.h>
#endif // CPP_TS_TEST_WASMTIME_CXX_ENABLED
#endif // defined(CPP_TS_FEATURE_WASM)

/**
 * @brief Returns the JSON grammar (tree-sitter-json, linked statically).
 * @return Returns the language. Owned by the grammar library.
 */
extern "C" const TSLanguage *tree_sitter_json();

namespace ts_test
{
#pragma region FIXTURES

    /////////////////////////////////////////////////////////////////////////////
    // Fixtures
    // Parsed documents, input readers and Wasm engines shared by the tests.
    /////////////////////////////////////////////////////////////////////////////

    /// @brief A JSON document parsed with the JSON grammar.
    struct JsonFixture
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief The JSON grammar.
        ts::Language language;

        /// @brief Parser of the document.
        ts::Parser parser;

        /// @brief Source of the document.
        std::string source;

        /// @brief Parsed document.
        ts::Tree tree;

#pragma endregion

#pragma region LIFECYCLE

        /////////////////////////////////////////////////////////////////////////////
        // Lifecycle
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Parses a JSON document.
         * @param code Source of the document. Kept alive by the fixture, so nodes can view it.
         */
        explicit JsonFixture(const std::string &code)
            : language(tree_sitter_json()), parser(language), source(code), tree(parser.parseString(source))
        {}

#pragma endregion

#pragma region GET

        /////////////////////////////////////////////////////////////////////////////
        // Get
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the root (`document`) node.
         * @return Returns the root node of the tree.
         */
        ts::Node root() const
        {
            return tree.getRootNode();
        }

        /**
         * @brief Returns the first named child of the root, i.e. the top-level JSON value.
         * @return Returns the value node.
         */
        ts::Node value() const
        {
            return tree.getRootNode().getNamedChild(0);
        }

        /**
         * @brief Returns the source text covered by a node.
         * @param node Node of this document.
         * @return Returns a copy of the text.
         */
        std::string text(ts::Node node) const
        {
            return node.getSourceText(source);
        }

#pragma endregion
    };

    /// @brief Read callback serving `source` in chunks of at most `chunk_size` bytes.
    struct ChunkedReader
    {
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Served source. Must outlive the reader.
        const std::string *source;

        /// @brief Maximum size of a chunk.
        std::uint32_t chunk_size;

#pragma endregion

#pragma region OPERATORS

        /////////////////////////////////////////////////////////////////////////////
        // Operators
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the chunk of the source starting at a byte.
         * @param byte Byte offset of the requested chunk.
         * @param position Position of the requested chunk. Unused.
         * @param bytes_read Receives the length of the chunk.
         * @return Returns the chunk, or an empty chunk past the end.
         */
        ts::details::StringViewReturn operator()(std::uint32_t  byte,
                                                 ts::Point      position,
                                                 std::uint32_t *bytes_read) const
        {
            static_cast<void>(position);
            if (byte >= source->size())
            {
                *bytes_read = 0;
                return ts::details::StringViewReturn();
            }
            const std::size_t left = source->size() - byte;
            *bytes_read            = static_cast<std::uint32_t>(left < chunk_size ? left : chunk_size);
            return ts::details::StringViewReturn(source->data() + byte, *bytes_read);
        }

#pragma endregion
    };

    /**
     * @brief Builds a JSON array of `elements` small objects.
     * @details tree-sitter only reports parse/query progress periodically, so tests of progress callbacks need a
     * document large enough for the callback to be invoked.
     * @param elements Number of `{"key": 1}` objects in the array (an empty object is appended at the end).
     * @return JSON source code.
     */
    inline std::string bigJsonArray(int elements)
    {
        std::string code = "[";
        for (int i = 0; i < elements; ++i)
        {
            code += "{\"key\": 1},";
        }
        code += "{}]";
        return code;
    }

#if defined(CPP_TS_FEATURE_WASM)
    /**
     * @brief Returns the path of the JSON grammar compiled to WebAssembly.
     * @return Returns the path of `tree-sitter-json.wasm`.
     */
    inline const char *jsonWasmPath()
    {
        return CPP_TS_TEST_WASM_FILES_DIR "/tree-sitter-json.wasm";
    }

    /// @brief Owns a wasm engine. Stores clone the engine, so it is released by its creator, after the stores.
    class Engine
    {
    public:
#if !CPP_TS_TEST_WASMTIME_CXX_ENABLED
#pragma region LIFECYCLE

        /////////////////////////////////////////////////////////////////////////////
        // Lifecycle
        /////////////////////////////////////////////////////////////////////////////

        /// @brief Creates an engine.
        Engine() : engine_(wasm_engine_new())
        {}

        /// @brief Deletes the engine.
        ~Engine()
        {
            wasm_engine_delete(engine_);
        }

        /**
         * @brief Deleted copy constructor.
         * @param other Another engine.
         */
        Engine(const Engine &other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * @param other Another engine.
         * @return Returns a reference to this engine.
         */
        Engine &operator=(const Engine &other) = delete;

#pragma endregion

#endif // !CPP_TS_TEST_WASMTIME_CXX_ENABLED

#pragma region GET

        /////////////////////////////////////////////////////////////////////////////
        // Get
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Returns the engine.
         * @return Pointer to the C API engine.
         */
        wasm_engine_t *get()
        {
#if CPP_TS_TEST_WASMTIME_CXX_ENABLED
            return engine_.capi();
#else
            return engine_;
#endif // CPP_TS_TEST_WASMTIME_CXX_ENABLED
        }

#pragma endregion

    private:
#pragma region FIELDS

        /////////////////////////////////////////////////////////////////////////////
        // Fields
        /////////////////////////////////////////////////////////////////////////////

#if CPP_TS_TEST_WASMTIME_CXX_ENABLED
        /// @brief Owned engine (wasmtime C++ API).
        wasmtime::Engine engine_;
#else
        /// @brief Owned engine (wasmtime C API).
        wasm_engine_t *engine_;
#endif // CPP_TS_TEST_WASMTIME_CXX_ENABLED

#pragma endregion
    };
#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

#pragma region HELPERS

    /////////////////////////////////////////////////////////////////////////////
    // Helpers
    // Helpers hiding API differences between standards.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Returns the version metadata of a language, or zeros, in every standard.
     * @param language Inspected language.
     * @return Returns the metadata, or zeros if the language has none.
     */
    inline ts::LanguageMetadata metadata(ts::LanguageView language)
    {
#if CPP_TS_HAS_OPTIONAL
        const std::optional<ts::LanguageMetadata> result = language.getMetadata();
        return result ? *result : ts::LanguageMetadata{ 0, 0, 0 };
#else
        return language.getMetadata();
#endif // CPP_TS_HAS_OPTIONAL
    }

    /**
     * @brief Explicitly discards a value (results of the wrapper are [[nodiscard]] since C++17).
     * @tparam T Type of the value.
     * @param value Discarded value.
     */
    template <typename T>
    void discard(const T &value)
    {
        static_cast<void>(value);
    }

    /**
     * @brief Checks whether a text contains a substring.
     * @tparam Text Type of the text (any type with `data()` and `size()`).
     * @param text Searched text.
     * @param part Searched substring.
     * @return Returns true if `text` contains `part`.
     */
    template <typename Text>
    bool contains(const Text &text, const std::string &part)
    {
        return std::string(text.data(), text.size()).find(part) != std::string::npos;
    }

    /**
     * @brief Calls a function and returns the message of the exception it threw.
     * @tparam Function Type of the function.
     * @param function Called function.
     * @return Returns the `what()` of the thrown exception, or a marker if nothing was thrown.
     */
    template <typename Function>
    std::string thrownMessage(Function function)
    {
        try
        {
            function();
        }
        catch (const std::exception &error)
        {
            return error.what();
        }
        return "<nothing thrown>";
    }

    /**
     * @brief Reads a whole file (binary).
     * @param path Path of the file.
     * @return Returns the content, or an empty string if the file cannot be opened.
     */
    inline std::string readFile(const char *path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

#pragma endregion
} // namespace ts_test

#pragma region PRINTERS

/////////////////////////////////////////////////////////////////////////////
// Printers
// StringMaker specializations making failed assertions on library types readable.
/////////////////////////////////////////////////////////////////////////////

namespace TS_TEST_STRING_MAKER_NAMESPACE
{
    /// @brief Prints a point as "(row, column)".
    template <>
    struct StringMaker<ts::Point>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param point Printed point.
         * @return Returns the text.
         */
        static std::string convert(const ts::Point &point)
        {
            return "(" + std::to_string(point.row) + ", " + std::to_string(point.column) + ")";
        }

#pragma endregion
    };

    /// @brief Prints an extent as "[start, end)".
    template <typename T>
    struct StringMaker<ts::Extent<T>>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param extent Printed extent.
         * @return Returns the text.
         */
        static std::string convert(const ts::Extent<T> &extent)
        {
            return "[" + StringMaker<T>::convert(extent.start) + ", " + StringMaker<T>::convert(extent.end) + ")";
        }

#pragma endregion
    };

    /// @brief Prints the byte and point extents of a range.
    template <>
    struct StringMaker<ts::Range>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param range Printed range.
         * @return Returns the text.
         */
        static std::string convert(const ts::Range &range)
        {
            return "bytes " + StringMaker<ts::Extent<std::uint32_t>>::convert(range.byte) + " points "
                 + StringMaker<ts::Extent<ts::Point>>::convert(range.point);
        }

#pragma endregion
    };

    /// @brief Prints the type and the byte range of a node.
    template <>
    struct StringMaker<ts::Node>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param node Printed node.
         * @return Returns the text.
         */
        static std::string convert(const ts::Node &node)
        {
            if (node.isNull())
            {
                return "null node";
            }
            return std::string(node.getType()) + " "
                 + StringMaker<ts::Extent<std::uint32_t>>::convert(node.getByteRange());
        }

#pragma endregion
    };

    /// @brief Prints the name of an error kind.
    template <>
    struct StringMaker<ts::ErrorKind>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param kind Printed error kind.
         * @return Returns the text.
         */
        static std::string convert(const ts::ErrorKind &kind)
        {
            return ts::getErrorKindName(kind);
        }

#pragma endregion
    };

#if !CPP_TS_HAS_STRING_VIEW
    /// @brief Prints a string view in quotes.
    template <>
    struct StringMaker<ts::details::StringView>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param view Printed view.
         * @return Returns the text.
         */
        static std::string convert(const ts::details::StringView &view)
        {
            return "\"" + std::string(view) + "\"";
        }

#pragma endregion
    };
#elif !defined(CPP_TS_TEST_USE_CATCH2)
    /// @brief Prints a string view in quotes.
    template <>
    struct StringMaker<std::string_view>
    {
#pragma region CONVERSION

        /////////////////////////////////////////////////////////////////////////////
        // Conversion
        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Converts a value to text.
         * @param view Printed view.
         * @return Returns the text.
         */
        static std::string convert(const std::string_view &view)
        {
            return "\"" + std::string(view) + "\"";
        }

#pragma endregion
    };
#endif // !CPP_TS_HAS_STRING_VIEW
} // namespace TS_TEST_STRING_MAKER_NAMESPACE

#pragma endregion

#endif // CPP_TREE_SITTER_TESTS_COMMON_HPP
