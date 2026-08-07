# Change this value to test with different c++ versions
set(CPP_TS_BENCHMARK_CXX_STANDARD ${CPP_TS_CXX_STANDARD} CACHE STRING "C++ standard for benchmark" FORCE)

set(PCH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp)
set(TARGET_NAME ${PROJECT_NAME}-benchmark)

file(GLOB_RECURSE BENCHMARK_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${TARGET_NAME} ${BENCHMARK_FILES})

target_precompile_headers(${TARGET_NAME} PUBLIC ${PCH_FILE})

set_target_properties(${TARGET_NAME} PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${CPP_TS_BENCHMARK_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)

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

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

get_target_property(ACTUAL_STANDARD ${TARGET_NAME} CXX_STANDARD)
message(STATUS "Actual standard for ${TARGET_NAME}: ${ACTUAL_STANDARD}")

if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE _WIN32)
elseif(APPLE)
    target_compile_definitions(${TARGET_NAME} PRIVATE __APPLE__)
elseif(UNIX)
    target_compile_definitions(${TARGET_NAME} PRIVATE __linux__)
endif()

set(JSONGrammarArgs
    NAME tree-sitter-json
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json.git
    VERSION ${CPP_TS_JSON_GRAMMAR}
)

if(CPP_TS_FEATURE_WASM)
    CPPTSCopyWasmtime(TARGET ${TARGET_NAME})
    list(APPEND JSONGrammarArgs FIND_ALSO_WASM_FILE)
endif()

CPPTSAddGrammar(${JSONGrammarArgs})

target_compile_options(${TARGET_NAME} PRIVATE
    ${TS_ERROR_COMPILE_OPTIONS}
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
        $<$<CONFIG:Debug>:/RTC1>
        /Zc:preprocessor
        /Zc:__cplusplus
    >
)

# Check IPO
message(CHECK_START "Checking for linker IPO support")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)

if(ipo_supported)
	message(CHECK_PASS "Supported")
	set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
	message(CHECK_FAIL "NOT Supported")
	message(VERBOSE "IPO is not supported at link time:\n${ipo_output} IPO link options will not be passed to linker.")
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
    target_compile_definitions(${TARGET_NAME} PRIVATE CPP_TS_BENCHMARK_FEATURE_WASM)
    target_compile_definitions(${TARGET_NAME} PRIVATE CPP_TS_BENCHMARK_WASM_FILES_DIR="${CPP_TS_WASM_DIR}")

    string(REPLACE "." ";" VERSION_LIST ${CPP_TS_WASMTIME_VERSION})
    list(GET VERSION_LIST 0 VER_MAJOR)
    list(GET VERSION_LIST 1 VER_MINOR)
    list(GET VERSION_LIST 2 VER_PATCH)

    math(EXPR CPP_TS_WT_VERSION "${VER_MAJOR} * 10000 + ${VER_MINOR} * 100 + ${VER_PATCH}")
    target_compile_definitions(${TARGET_NAME} PRIVATE CPP_TS_BENCHMARK_WASMTIME_VERSION=${CPP_TS_WT_VERSION})
endif()

target_link_libraries(${TARGET_NAME} PRIVATE 
	${PROJECT_NAME}-header-only
    tree-sitter-json
)