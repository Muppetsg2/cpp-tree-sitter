cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(get_cpm)
include(utils)

project(
    cpp-tree-sitter
    VERSION 0.26.8
    LANGUAGES C CXX
    DESCRIPTION "Simple C++ and CMake wrapper around tree-sitter."
)

# OPTIONS
set(CPP_TS_BUILD_TESTS_DEFAULT OFF)
if(PROJECT_IS_TOP_LEVEL)
    set(CPP_TS_BUILD_TESTS_DEFAULT ON)
endif()

option(CPP_TS_BUILD_TESTS "Build test for cpp-tree-sitter" ${CPP_TS_BUILD_TESTS_DEFAULT})
option(CPP_TS_FEATURE_WASM "Enable the Wasm feature" ON)
option(CPP_TS_AMALGAMATED "Build using an amalgamated source" ON)

set(CPP_TS_GRAMMAR_PATH "" CACHE PATH "Path to locally installed tree-sitter grammars")

# We want to automatically download and provide tree-sitter to users of
# the package, so pull it in and retrofit cmake dependencies on top of it.
CPMAddPackage(
    NAME tree-sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    VERSION 0.26.8
    DOWNLOAD_ONLY YES
)

if (tree-sitter_ADDED)
	if(CPP_TS_AMALGAMATED)
		set(TREE_SITTER_SOURCE_FILES "${tree-sitter_SOURCE_DIR}/lib/src/lib.c")
	else()
		file(GLOB TREE_SITTER_SOURCE_FILES CONFIGURE_DEPENDS "${tree-sitter_SOURCE_DIR}/lib/src/*.c")
		list(REMOVE_ITEM TREE_SITTER_SOURCE_FILES "${tree-sitter_SOURCE_DIR}/lib/src/lib.c")
	endif()

    add_library(tree-sitter STATIC ${TREE_SITTER_SOURCE_FILES})
    target_include_directories(tree-sitter PRIVATE $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/src> $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/src/wasm>)
    target_include_directories(tree-sitter PUBLIC  $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/include>)
    target_compile_options(tree-sitter PRIVATE 
		$<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
			/W4
			/Zc:__cplusplus
		>
		$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
			-Wno-unused-but-set-variable
			-Wno-conversion
			-Wall
			-Wextra
		>
	)
endif()

function(cpp_ts_add_grammar)
    set(options)                                    	  			 # Bool Options
    set(oneValueArgs NAME GIT_REPOSITORY VERSION GIT_TAG SOURCE_DIR) # One element arguments
    set(multiValueArgs)                             	  			 # No multi-value arguments

    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if(NOT arg_NAME)
        message(FATAL_ERROR "cpp_ts_add_grammar requires NAME")
    endif()
	
	set(NAME ${arg_NAME})
	set(GRAMMAR_SOURCE_DIR "")
	
	if(arg_SOURCE_DIR AND EXISTS "${arg_SOURCE_DIR}/src/parser.c")
        set(GRAMMAR_SOURCE_DIR "${arg_SOURCE_DIR}")
        message(STATUS "Tree-sitter: Using ${NAME} from explicitly provided path: ${GRAMMAR_SOURCE_DIR}")
	elseif(TREE_SITTER_GRAMMAR_PATH AND EXISTS "${TREE_SITTER_GRAMMAR_PATH}/${NAME}/src/parser.c")
        set(GRAMMAR_SOURCE_DIR "${TREE_SITTER_GRAMMAR_PATH}/${NAME}")
        message(STATUS "Tree-sitter: Found ${NAME} in TREE_SITTER_GRAMMAR_PATH: ${GRAMMAR_SOURCE_DIR}")
    endif()

	if(NOT GRAMMAR_SOURCE_DIR)
		message(STATUS "Tree-sitter: ${NAME} not found locally, fetching from ${arg_GIT_REPOSITORY}")
		
		if(NOT arg_GIT_REPOSITORY)
            message(FATAL_ERROR "cpp_ts_add_grammar requires GIT_REPOSITORY OR SOURCE_DIR to fetch ${NAME}")
        endif()
		
		set(CPM_ARGS
            NAME ${NAME}
            GIT_REPOSITORY ${arg_GIT_REPOSITORY}
            DOWNLOAD_ONLY YES
        )
		
		if(arg_VERSION)
            list(APPEND CPM_ARGS VERSION ${arg_VERSION})
        elseif(arg_GIT_TAG)
            list(APPEND CPM_ARGS GIT_TAG ${arg_GIT_TAG})
        else()
            message(STATUS "Tree-sitter: No VERSION or GIT_TAG for ${NAME}, defaulting to master/main branch")
        endif()
		
		CPMAddPackage(${CPM_ARGS})
		
		if ("${${NAME}_ADDED}")
			set(GRAMMAR_SOURCE_DIR "${${NAME}_SOURCE_DIR}")
		endif()
	endif()

    if (GRAMMAR_SOURCE_DIR)
        file(GLOB maybe_scanner 
			"${GRAMMAR_SOURCE_DIR}/src/scanner.c"
			"${GRAMMAR_SOURCE_DIR}/src/scanner.cc"
			"${GRAMMAR_SOURCE_DIR}/src/scanner.cpp"
		)

        add_library(${NAME} STATIC "${GRAMMAR_SOURCE_DIR}/src/parser.c" ${maybe_scanner})

		if(MSVC)
			target_compile_definitions(${NAME} PRIVATE NOMINMAX)
		endif()

        # parser.h is stored within the src directory, so we need to include
        # src in the search paths
        target_include_directories(${NAME} PRIVATE $<BUILD_INTERFACE:${GRAMMAR_SOURCE_DIR}/src>)
        target_include_directories(${NAME} PUBLIC  $<INSTALL_INTERFACE:include>)

        target_link_libraries(${NAME} PRIVATE tree-sitter)
		
		foreach(scanner_file ${maybe_scanner})
            if(scanner_file MATCHES ".*\\.cc$" OR scanner_file MATCHES ".*\\.cpp$")
                set_target_properties(${NAME} PROPERTIES LINKER_LANGUAGE CXX)
                break()
            endif()
        endforeach()
		
        target_compile_options(${NAME} PRIVATE 
			$<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
				/W4
				/Zc:__cplusplus
			>
			$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
				-Wno-unused-but-set-variable
				-Wno-conversion
				-Wall
				-Wextra
			>
		)
    endif()
endfunction()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(${PROJECT_NAME} INTERFACE tree-sitter)
target_compile_options(${PROJECT_NAME} INTERFACE 
	$<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
		/W4
		/Zc:__cplusplus
	>
	$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
		-Wno-unused-but-set-variable
		-Wno-conversion
		-Wall
		-Wextra
	>
)

if(CPP_TS_FEATURE_WASM)
	set(WASM_VERSION 43.0.1)
	if(WIN32)
		set(WASMTIME_URL "https://github.com/bytecodealliance/wasmtime/releases/download/v${WASM_VERSION}/wasmtime-v${WASM_VERSION}-x86_64-windows-c-api.zip")
	elseif(APPLE)
		set(WASMTIME_URL "https://github.com/bytecodealliance/wasmtime/releases/download/v${WASM_VERSION}/wasmtime-v${WASM_VERSION}-x86_64-macos-c-api.tar.xz")
	elseif(UNIX)
		set(WASMTIME_URL "https://github.com/bytecodealliance/wasmtime/releases/download/v${WASM_VERSION}/wasmtime-v${WASM_VERSION}-x86_64-linux-c-api.tar.xz")
	endif()

	CPMAddPackage(
		NAME wasmtime
		URL ${WASMTIME_URL}
		DOWNLOAD_ONLY YES
	)

	set(WASMTIME_LIB_DIR "")
	if(wasmtime_ADDED)
		set(WASMTIME_INCLUDE_DIR ${wasmtime_SOURCE_DIR}/include)
		set(WASMTIME_LIB_DIR ${wasmtime_SOURCE_DIR}/lib)
    
		set(WASMTIME_LIB_NAME wasmtime)
		if(NOT BUILD_SHARED_LIBS)
			if(WIN32)
				set(WASMTIME_LIB_NAME wasmtime.lib)

				# Project
				target_compile_definitions(${PROJECT_NAME} INTERFACE WASM_API_EXTERN= WASI_API_EXTERN=)
				target_link_libraries(${PROJECT_NAME} INTERFACE ws2_32 advapi32 userenv ntdll shell32 ole32 bcrypt)

				# Tree-Sitter
				target_compile_definitions(tree-sitter PRIVATE WASM_API_EXTERN= WASI_API_EXTERN=)
				target_link_libraries(tree-sitter INTERFACE ws2_32 advapi32 userenv ntdll shell32 ole32 bcrypt)
			elseif(NOT APPLE)
				# Project
				target_link_libraries(${PROJECT_NAME} INTERFACE pthread dl m)

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
		target_compile_definitions(${PROJECT_NAME} INTERFACE CPP_TREE_SITTER_FEATURE_WASM)
		target_link_libraries(${PROJECT_NAME} INTERFACE ${WASMTIME_LIBRARY})

		# Tree-Sitter
		target_include_directories(tree-sitter PRIVATE ${WASMTIME_INCLUDE_DIR})
		target_compile_definitions(tree-sitter PUBLIC TREE_SITTER_FEATURE_WASM)
		target_link_libraries(tree-sitter PUBLIC ${WASMTIME_LIBRARY})
	endif()

	function(cpp_ts_copy_wasmtime)
		set(options)
		set(oneValueArgs TARGET)
		set(multiValueArgs)

		cmake_parse_arguments(PARSE_ARGV 0 arg
			"${options}" "${oneValueArgs}" "${multiValueArgs}"
		)

		if(NOT arg_TARGET)
			message(FATAL_ERROR "cpp_ts_copy_wasmtime requires TARGET")
		endif()

		find_and_copy_lib(
			TARGET ${arg_TARGET}
			NAMES wasmtime.dll
			PATHS ${WASMTIME_LIB_DIR}
		)
	endfunction()
endif()

if(CPP_TS_BUILD_TESTS)
	add_subdirectory(tests)
endif()