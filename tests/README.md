# Tests

Every test is written once and runs in every supported standard (C++11 to C++23):

* **C++14 and newer:** the tests are built with [Catch2 v3](https://github.com/catchorg/Catch2). Every test case is
  registered in CTest (and is visible in IDE test explorers).
* **C++11:** Catch2 v3 requires C++14 (and propagates `cxx_std_14` to every target linked with it, which would silently
  turn a "C++11" build into C++14), so the same test files are built with the small built-in framework in
  `framework/minitest.hpp`. It implements the subset of Catch2 used by the tests with the same names and semantics.
  Every test file is registered in CTest as a separate test.

The built-in framework can also be used in newer standards with `-DCPP_TS_TEST_BUILTIN_FRAMEWORK=ON`.

```
tests/
├── framework/
│   ├── framework.hpp        # selects Catch2 or the built-in framework
│   ├── minitest.hpp         # built-in framework (no dependencies, C++11)
│   └── minitest-main.cpp    # runner of the built-in framework: filters, summary, exit code
├── common.hpp               # included first by every test: library, fixtures, helpers, value printers
└── <area>-tests.cpp         # tests, one file per area of the library
```

Two executables are built from the same files: `cpp-tree-sitter-tests` (header-only target) and
`cpp-tree-sitter-tests-compiled` (compiled library).

## Writing tests

Test files include `common.hpp` and use only these macros (available with both frameworks):

`TEST_CASE`, `SECTION`, `CHECK`, `CHECK_FALSE`, `REQUIRE`, `REQUIRE_FALSE`, `CHECK_THROWS`, `REQUIRE_THROWS`,
`CHECK_THROWS_AS`, `REQUIRE_THROWS_AS`, `CHECK_THROWS_WITH`, `REQUIRE_THROWS_WITH`, `CHECK_NOTHROW`, `REQUIRE_NOTHROW`,
`FAIL`, `SUCCEED`.

```cpp
#include "common.hpp"

TEST_CASE("Node navigation", "[node][navigation]")
{
    ts_test::JsonFixture json("[1, true]");          // parsed document, see common.hpp

    REQUIRE(json.value().getType() == "array");       // stops the test case on failure

    SECTION("Children")                               // each SECTION runs in its own pass of the test case
    {
        CHECK(json.value().getNamedChildCount() == 2u); // reports "3 == 2" on failure and continues
        CHECK_FALSE(json.root().hasError());
    }

    SECTION("Errors")
    {
        CHECK_THROWS_AS(json.parser.parseString("[1]", other_tree), std::invalid_argument);
        CHECK_THROWS_WITH(ts::Parser(ts::LanguageView()), "Tree-sitter: Passed Language for Parser was invalid");
    }
}
```

Rules:

* The code must stay C++11: no generic lambdas, `auto` return types, `std::make_unique`, `std::optional`, ... Test
  features of newer standards under the feature macros of the library (`#if CPP_TS_HAS_EXPECTED`,
  `CPP_TS_HAS_SPAN`, `CPP_TS_HAS_RANGES`, ...), so the tests follow exactly what the wrapper enabled.
* Use only the macros above (no matchers, generators or `INFO`). For substrings use `CHECK(ts_test::contains(text,
  "part"))`, for exception messages `CHECK_THROWS_WITH` or `ts_test::thrownMessage`.
* `&&` and `||` cannot be decomposed: write `CHECK((a && b))` or two checks.
* Failed assertions print library types through the printers in `common.hpp` (`StringMaker` specializations in the
  namespace of the selected framework). Add one there for a new type.
* Test case names must be unique in the whole suite.
* Tests of the `std::expected` API (`create` / `try*`) go to `errors-tests.cpp`; the exceptions of the throwing API
  are tested in the file of each class.

## Adding a test file

Create `tests/<area>-tests.cpp` and re-run CMake. Files that need an optional feature guard their content with its
macro (see `wasm-tests.cpp`) and are excluded in `CMakeLists.txt` when the feature is off.

## Running

```bash
ctest --test-dir build                                        # everything
ctest --test-dir build -R "header-only"                       # one executable

# Catch2 (C++14 and newer)
./build/tests/cpp-tree-sitter-tests "[parser]"
./build/tests/cpp-tree-sitter-tests -# "[#parser-tests]"      # the test cases of one file

# Built-in framework (C++11)
./build/tests/cpp-tree-sitter-tests --list
./build/tests/cpp-tree-sitter-tests --file parser-tests.cpp
./build/tests/cpp-tree-sitter-tests --tag [regression] --verbose
./build/tests/cpp-tree-sitter-tests --name "Query copy"
```
