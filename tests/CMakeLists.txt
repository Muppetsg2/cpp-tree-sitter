# Test suite.
#
# Every `*-tests.cpp` file in this directory is written once, in C++11, against the subset of Catch2 described in
# framework/framework.hpp, and compiled in the selected standard (CPP_TS_TEST_CXX_STANDARD):
#   - C++14 and newer: Catch2 v3 (every test case is registered in CTest, also visible in IDE test explorers),
#   - C++11, or CPP_TS_TEST_BUILTIN_FRAMEWORK=ON: the built-in framework (framework/minitest.hpp), Catch2 v3 requires
#     C++14. Every test file is registered in CTest as a separate test.
# Two executables are built: one for the header-only target and one for the compiled library.
# Adding a test file only requires creating it (and re-running CMake).

# Change this value to test with different c++ versions
set(CPP_TS_TEST_CXX_STANDARD ${CPP_TS_CXX_STANDARD} CACHE STRING "C++ standard for tests" FORCE)
option(CPP_TS_TEST_BUILTIN_FRAMEWORK "Run the tests with the built-in framework instead of Catch2" OFF)

file(GLOB CPP_TS_TEST_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*-tests.cpp")

if(NOT CPP_TS_FEATURE_WASM)
    list(REMOVE_ITEM CPP_TS_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/wasm-tests.cpp")
endif()

set(JSONGrammarArgs
    NAME tree-sitter-json
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json.git
    VERSION ${CPP_TS_JSON_GRAMMAR}
)

if(CPP_TS_FEATURE_WASM)
    list(APPEND JSONGrammarArgs FIND_ALSO_WASM_FILE)
endif()

CPPTSAddGrammar(${JSONGrammarArgs})

if(CPP_TS_TEST_CXX_STANDARD LESS 14 OR CPP_TS_TEST_BUILTIN_FRAMEWORK)
    set(CPP_TS_TEST_USE_CATCH2 OFF)
    message(STATUS "cpp-tree-sitter: tests use the built-in framework (C++${CPP_TS_TEST_CXX_STANDARD})")
else()
    set(CPP_TS_TEST_USE_CATCH2 ON)
    message(STATUS "cpp-tree-sitter: tests use Catch2 (C++${CPP_TS_TEST_CXX_STANDARD})")

    cpp_ts_add_package(
        URI "gh:catchorg/Catch2@${CPP_TS_CATCH2_VERSION}"
        OPTIONS "CATCH_INSTALL_DOCS OFF"
                "BUILD_TESTING OFF"
    )

    if(Catch2_ADDED)
        set(Catch2_Extras ${Catch2_SOURCE_DIR}/extras CACHE INTERNAL "")

        set_target_properties(Catch2 Catch2WithMain PROPERTIES
            CXX_STANDARD ${CPP_TS_TEST_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )

        if(WIN32)
            set_target_properties(Catch2WithMain Catch2 PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${CPP_TS_MSVC_STATIC_RUNTIME}>>:DLL>"
            )
        endif()

        if(MSVC)
            target_compile_definitions(Catch2 PRIVATE NOMINMAX)
            target_compile_definitions(Catch2WithMain PRIVATE NOMINMAX)
        endif()
    endif()

    list(APPEND CMAKE_MODULE_PATH ${Catch2_Extras})
    include(Catch)
endif()

# Check PIE
message(CHECK_START "Checking for CXX linker PIE support")

include(CheckPIESupported)
check_pie_supported(OUTPUT_VARIABLE pie_output LANGUAGES CXX)

if(CMAKE_CXX_LINK_PIE_SUPPORTED)
    message(CHECK_PASS "Supported")
else()
    message(CHECK_FAIL "NOT Supported")
    message(VERBOSE "PIE is not supported at link time:\n${pie_output} PIE link options will not be passed to linker.")
endif()

# Check IPO
message(CHECK_START "Checking for linker IPO support")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)

if(ipo_supported)
    message(CHECK_PASS "Supported")
else()
    message(CHECK_FAIL "NOT Supported")
    message(VERBOSE "IPO is not supported at link time:\n${ipo_output} IPO link options will not be passed to linker.")
endif()

foreach(CPP_TS_TEST_VARIANT IN ITEMS header-only compiled)
    if(CPP_TS_TEST_VARIANT STREQUAL "header-only")
        set(TARGET_NAME ${PROJECT_NAME}-tests)
        set(CPP_TS_TEST_LIBRARY ${PROJECT_NAME}-header-only)
    else()
        set(TARGET_NAME ${PROJECT_NAME}-tests-compiled)
        set(CPP_TS_TEST_LIBRARY ${PROJECT_NAME})
    endif()

    add_executable(${TARGET_NAME} ${CPP_TS_TEST_FILES})

    if(NOT CPP_TS_TEST_USE_CATCH2)
        target_sources(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/framework/minitest-main.cpp)
    endif()

    target_precompile_headers(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common.hpp)

    set_target_properties(${TARGET_NAME} PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD ${CPP_TS_TEST_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_compile_options(${TARGET_NAME} PRIVATE
        ${TS_ERROR_COMPILE_OPTIONS}
        $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
            $<$<CONFIG:Debug>:/RTC1>
            /Zc:preprocessor
            /Zc:__cplusplus
        >
    )

    if(ipo_supported)
        set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
                $<$<CONFIG:Release>:/O2>
                $<$<CONFIG:Release>:/GL>
                $<$<CONFIG:Release>:/fp:fast>
            >
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
                $<$<CONFIG:Release>:-O3>
                $<$<CONFIG:Release>:-flto=auto>
            >
        )

        target_link_options(${TARGET_NAME} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
                $<$<CONFIG:Release>:/LTCG>
                $<$<CONFIG:Release>:/O2>
            >
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
                $<$<CONFIG:Release>:-flto=auto>
                $<$<CONFIG:Release>:-O3>
                $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:Clang>>:-fuse-ld=lld>
            >
        )
    endif()

    if(WIN32)
        set_target_properties(${TARGET_NAME} PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${CPP_TS_MSVC_STATIC_RUNTIME}>>:DLL>"
        )
    endif()

    if(MSVC)
        target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX)
    endif()

    if(CPP_TS_FEATURE_WASM)
        target_compile_definitions(${TARGET_NAME} PRIVATE CPP_TS_TEST_WASM_FILES_DIR="${CPP_TS_WASM_DIR}")

        string(REPLACE "." ";" VERSION_LIST ${CPP_TS_WASMTIME_VERSION})
        list(GET VERSION_LIST 0 VER_MAJOR)
        list(GET VERSION_LIST 1 VER_MINOR)
        list(GET VERSION_LIST 2 VER_PATCH)

        math(EXPR CPP_TS_WT_VERSION "${VER_MAJOR} * 10000 + ${VER_MINOR} * 100 + ${VER_PATCH}")
        target_compile_definitions(${TARGET_NAME} PRIVATE CPP_TS_TEST_WASMTIME_VERSION=${CPP_TS_WT_VERSION})

        CPPTSCopyWasmtime(TARGET ${TARGET_NAME})
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE ${CPP_TS_TEST_LIBRARY} tree-sitter-json)

    if(CPP_TS_TEST_USE_CATCH2)
        target_compile_definitions(${TARGET_NAME} PRIVATE CPP_TS_TEST_USE_CATCH2)
        target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain)

        if(Catch2_ADDED)
            cpp_ts_find_and_copy_lib(
                TARGET ${TARGET_NAME}
                NAMES "$<$<CONFIG:Debug>:Catch2d.dll>$<$<NOT:$<CONFIG:Debug>>:Catch2.dll>"
                PATHS ${Catch2_BINARY_DIR}/src
                POST_BUILD
            )

            cpp_ts_find_and_copy_lib(
                TARGET ${TARGET_NAME}
                NAMES "$<$<CONFIG:Debug>:Catch2Maind.dll>$<$<NOT:$<CONFIG:Debug>>:Catch2Main.dll>"
                PATHS ${Catch2_BINARY_DIR}/src
                POST_BUILD
            )
        endif()

        # One CTest test per test case.
        catch_discover_tests(${TARGET_NAME}
            TEST_PREFIX "${CPP_TS_TEST_VARIANT}: "
            DISCOVERY_MODE PRE_TEST
        )
    else()
        # One CTest test per test file.
        foreach(CPP_TS_TEST_FILE IN LISTS CPP_TS_TEST_FILES)
            get_filename_component(CPP_TS_TEST_FILE_NAME "${CPP_TS_TEST_FILE}" NAME)
            get_filename_component(CPP_TS_TEST_AREA "${CPP_TS_TEST_FILE}" NAME_WE)
            string(REGEX REPLACE "-tests$" "" CPP_TS_TEST_AREA "${CPP_TS_TEST_AREA}")

            add_test(
                NAME "${CPP_TS_TEST_VARIANT}: ${CPP_TS_TEST_AREA}"
                COMMAND ${TARGET_NAME} --file ${CPP_TS_TEST_FILE_NAME}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
        endforeach()
    endif()

    get_target_property(ACTUAL_STANDARD ${TARGET_NAME} CXX_STANDARD)
    message(STATUS "Actual standard for ${TARGET_NAME}: ${ACTUAL_STANDARD}")
endforeach()

if(MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
    configure_file(
        ${CMAKE_SOURCE_DIR}/.runsettings
        ${CMAKE_BINARY_DIR}/.runsettings
        COPYONLY
    )
endif()
