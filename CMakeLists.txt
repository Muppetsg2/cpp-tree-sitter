cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(get_cpm)

project(
    cpp-tree-sitter
    VERSION 0.26.8
    LANGUAGES C CXX
    DESCRIPTION "Simple C++ and CMake wrapper around tree-sitter."
)

add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wconversion>")

# We want to automatically download and provide tree-sitter to users of
# the package, so pull it in and retrofit cmake dependencies on top of it.
CPMAddPackage(
    NAME tree-sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    VERSION 0.26.8
    DOWNLOAD_ONLY YES
)

if (tree-sitter_ADDED)
    add_library(tree-sitter STATIC "${tree-sitter_SOURCE_DIR}/lib/src/lib.c")
    target_include_directories(tree-sitter PRIVATE $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/src>)
    target_include_directories(tree-sitter PUBLIC  $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${tree-sitter_SOURCE_DIR}/lib/include>)
    target_compile_options(tree-sitter PRIVATE "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-conversion>")
endif()


function(add_grammar_from_git_repo)

    set(options)                                    # Bool Options
    set(oneValueArgs NAME GIT_REPOSITORY VERSION)   # One element arguments
    set(multiValueArgs)                             # No multi-value arguments

    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if(NOT arg_NAME)
        message(FATAL_ERROR "add_grammar_from_git_repo requires NAME")
    endif()

    if(NOT arg_GIT_REPOSITORY)
        message(FATAL_ERROR "add_grammar_from_git_repo requires GIT_REPOSITORY")
    endif()

    if(NOT arg_VERSION)
        message(FATAL_ERROR "add_grammar_from_git_repo requires VERSION")
    endif()

    set(NAME ${arg_NAME})
    set(GIT_REPOSITORY ${arg_GIT_REPOSITORY})
    set(VERSION ${arg_VERSION})

    CPMAddPackage(
        NAME ${NAME}
        GIT_REPOSITORY ${GIT_REPOSITORY}
        VERSION ${VERSION}
        DOWNLOAD_ONLY YES
    )

    if ("${${NAME}_ADDED}")
        file(GLOB maybe_scanner "${${NAME}_SOURCE_DIR}/src/scanner.c")

        add_library(${NAME} STATIC "${${NAME}_SOURCE_DIR}/src/parser.c" ${maybe_scanner})

        # parser.h is stored within the src directory, so we need to include
        # src in the search paths
        target_include_directories(${NAME} PRIVATE $<BUILD_INTERFACE:${${NAME}_SOURCE_DIR}/src>)
        target_include_directories(${NAME} PUBLIC  $<INSTALL_INTERFACE:include>)

        target_link_libraries(${NAME} PRIVATE tree-sitter)
        target_compile_options(${NAME} PRIVATE "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-but-set-variable>")
    endif()
endfunction()

add_library(cpp-tree-sitter INTERFACE)
target_include_directories(cpp-tree-sitter INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(cpp-tree-sitter INTERFACE tree-sitter)