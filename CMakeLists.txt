cmake_minimum_required(VERSION 3.30)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(get_cpm)
include(libs_config)
include(utils)
include(grammar)

project(
    cpp-tree-sitter
    VERSION 0.26.9
    LANGUAGES C CXX
    DESCRIPTION "Simple C++ and CMake wrapper around tree-sitter."
)

# OPTIONS
option(CPP_TS_BUILD_TESTS "Build test for cpp-tree-sitter" ${PROJECT_IS_TOP_LEVEL})
option(CPP_TS_FEATURE_WASM "Enable the Wasm feature" OFF)
option(CPP_TS_AMALGAMATED "Build using an amalgamated source" ON)
option(CPP_TS_MSVC_STATIC_RUNTIME "Use static MSVC runtime (/MT or /MTd)" OFF)

# WASMTIME VERSION
if(DEFINED CPP_TS_WASMTIME_VERSION AND NOT CACHE{CPP_TS_WASMTIME_VERSION})
	set(CPP_TS_WASMTIME_VERSION "${CPP_TS_WASMTIME_VERSION}" CACHE PATH "Version of Wasmtime to be fetched from remote" FORCE)
endif()

if(NOT CPP_TS_WASMTIME_VERSION)
	set(CPP_TS_WASMTIME_VERSION "${CPP_TS_WASMTIME_DEFAULT_VERSION}" CACHE PATH "Version of Wasmtime to be fetched from remote")
endif()

# WASMTIME PATH
if(DEFINED CPP_TS_WASMTIME_PATH AND NOT CACHE{CPP_TS_WASMTIME_PATH})
	set(CPP_TS_WASMTIME_PATH "${CPP_TS_WASMTIME_PATH}" CACHE PATH "Path to locally installed wasmtime" FORCE)
endif()

if(NOT CPP_TS_WASMTIME_PATH)
	set(CPP_TS_WASMTIME_PATH "" CACHE PATH "Path to locally installed wasmtime")
endif()

message(STATUS "========== cpp-tree-sitter configuration ==========")
cpp_ts_print_config_option(CPP_TS_BUILD_TESTS)
cpp_ts_print_config_option(CPP_TS_FEATURE_WASM)
cpp_ts_print_config_option(CPP_TS_AMALGAMATED)
cpp_ts_print_config_option(CPP_TS_MSVC_STATIC_RUNTIME)
cpp_ts_print_config_option(CPP_TS_WASMTIME_VERSION)
cpp_ts_print_config_option(CPP_TS_WASMTIME_PATH)
message(STATUS "===================================================")

# We want to automatically download and provide tree-sitter to users of
# the package, so pull it in and retrofit cmake dependencies on top of it.
CPMAddPackage(
    NAME tree-sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    VERSION ${CPP_TS_TS_VERSION}
    DOWNLOAD_ONLY YES
)

if(tree-sitter_ADDED)
	if(CPP_TS_AMALGAMATED)
		set(TREE_SITTER_SOURCE_FILES "${tree-sitter_SOURCE_DIR}/lib/src/lib.c")
	else()
		file(GLOB TREE_SITTER_SOURCE_FILES CONFIGURE_DEPENDS "${tree-sitter_SOURCE_DIR}/lib/src/*.c")
		list(REMOVE_ITEM TREE_SITTER_SOURCE_FILES "${tree-sitter_SOURCE_DIR}/lib/src/lib.c")
	endif()

	add_library(tree-sitter STATIC ${TREE_SITTER_SOURCE_FILES})
    target_include_directories(tree-sitter PRIVATE $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/src> $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/src/wasm>)
    target_include_directories(tree-sitter PUBLIC  $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/include>)
	set_target_properties(tree-sitter PROPERTIES 
		LINKER_LANGUAGE C
		C_STANDARD_REQUIRED ON
		POSITION_INDEPENDENT_CODE ON
	)
	target_compile_features(tree-sitter PRIVATE c_std_11)
	target_compile_options(tree-sitter PRIVATE 
		${TS_ERROR_COMPILE_OPTIONS}
		$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
			-Werror=incompatible-pointer-types
		>
	)

	# Check IPO
	message(CHECK_START "Checking for linker IPO support")

	include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)

	if(ipo_supported)
		message(CHECK_PASS "Supported")
		set_target_properties(tree-sitter PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
	else()
		message(CHECK_FAIL "NOT Supported")
		message(VERBOSE "IPO is not supported at link time:\n${ipo_output} IPO link options will not be passed to linker.")
		target_compile_options(tree-sitter PRIVATE ${TS_NO_IPO_COMPILE_OPTIONS})
		target_link_options(tree-sitter PRIVATE ${TS_NO_IPO_LINK_OPTIONS})
	endif()

	if(WIN32)
		set_target_properties(tree-sitter PROPERTIES 
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${CPP_TS_MSVC_STATIC_RUNTIME}>>:DLL>"
		)
	endif()
	
	if(MSVC)
		target_compile_definitions(tree-sitter PRIVATE NOMINMAX)
	endif()

	target_compile_definitions(tree-sitter PRIVATE _POSIX_C_SOURCE=200112L _DEFAULT_SOURCE _BSD_SOURCE _DARWIN_C_SOURCE)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(${PROJECT_NAME} INTERFACE tree-sitter)
target_compile_options(${PROJECT_NAME} INTERFACE ${CPP_TS_NECESSARY_COMPILE_OPTIONS})

if(MSVC)
	target_compile_definitions(${PROJECT_NAME} INTERFACE NOMINMAX)
endif()

if(CPP_TS_FEATURE_WASM)
	set(WASMTIME_INCLUDE_DIR "")
	set(WASMTIME_LIB_DIR "")
	
	if(NOT CPP_TS_WASMTIME_PATH)
		set(WASM_VERSION "${CPP_TS_WASMTIME_VERSION}")

		if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
			set(WASM_ARCH "aarch64")
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(WASM_ARCH "x86_64")
		else()
			set(WASM_ARCH "i686")
		endif()

		if(WIN32)
			set(WASM_PLATFORM "windows")
			set(WASM_ARCHIVE_EXT "zip")
		elseif(APPLE)
			set(WASM_PLATFORM "macos")
			set(WASM_ARCHIVE_EXT "tar.xz")
		elseif(UNIX)
			set(WASM_PLATFORM "linux")
			set(WASM_ARCHIVE_EXT "tar.xz")
		endif()

		set(WASMTIME_URL "https://github.com/bytecodealliance/wasmtime/releases/download/v${WASM_VERSION}/wasmtime-v${WASM_VERSION}-${WASM_ARCH}-${WASM_PLATFORM}-c-api.${WASM_ARCHIVE_EXT}")

		CPMAddPackage(
			NAME wasmtime
			URL ${WASMTIME_URL}
			DOWNLOAD_ONLY YES
		)

		if(wasmtime_ADDED)
			set(WASMTIME_INCLUDE_DIR ${wasmtime_SOURCE_DIR}/include)
			set(WASMTIME_LIB_DIR ${wasmtime_SOURCE_DIR}/lib)
		endif()
	else()
		if(EXISTS "${CPP_TS_WASMTIME_PATH}")
			set(WASMTIME_INCLUDE_DIR ${CPP_TS_WASMTIME_PATH}/include)
			set(WASMTIME_LIB_DIR ${CPP_TS_WASMTIME_PATH}/lib)
		endif()
	endif()

	if(WASMTIME_INCLUDE_DIR AND WASMTIME_LIB_DIR)
		set(WASMTIME_LIB_NAME wasmtime)
		if(NOT BUILD_SHARED_LIBS)
			if(WIN32)
				set(WASMTIME_LIB_NAME wasmtime.lib)
				# Tree-Sitter
				target_compile_definitions(tree-sitter PRIVATE WASM_API_EXTERN= WASI_API_EXTERN=)
				target_link_libraries(tree-sitter INTERFACE ws2_32 advapi32 userenv ntdll shell32 ole32 bcrypt)
			elseif(NOT APPLE)
				# Tree-Sitter
				target_link_libraries(tree-sitter INTERFACE pthread dl m)
			endif()
		endif()

		unset(WASMTIME_LIBRARY CACHE)
		find_library(
			WASMTIME_LIBRARY 
			NAMES ${WASMTIME_LIB_NAME}
			PATHS ${WASMTIME_LIB_DIR}
			NO_DEFAULT_PATH
		)

		# Project
		target_include_directories(${PROJECT_NAME} INTERFACE ${WASMTIME_INCLUDE_DIR})
		target_compile_definitions(${PROJECT_NAME} INTERFACE CPP_TS_FEATURE_WASM)

		# Tree-Sitter
		target_include_directories(tree-sitter PRIVATE ${WASMTIME_INCLUDE_DIR})
		target_compile_definitions(tree-sitter PUBLIC TREE_SITTER_FEATURE_WASM)
		target_link_libraries(tree-sitter PUBLIC ${WASMTIME_LIBRARY})
	endif()

	function(CPPTSCopyWasmtime)
		set(options)
		set(oneValueArgs TARGET)
		set(multiValueArgs)

		cmake_parse_arguments(PARSE_ARGV 0 arg
			"${options}" "${oneValueArgs}" "${multiValueArgs}"
		)

		if(NOT arg_TARGET)
			message(FATAL_ERROR "CPPTSCopyWasmtime requires TARGET")
		endif()

		if(EXISTS "${WASMTIME_LIB_DIR}")
			cpp_ts_find_and_copy_lib(
				TARGET ${arg_TARGET}
				NAMES wasmtime.dll
				PATHS ${WASMTIME_LIB_DIR}
			)
		endif()
	endfunction()
endif()

if(CPP_TS_BUILD_TESTS)
	include(CTest)
	enable_testing()
	add_subdirectory(tests)
endif()