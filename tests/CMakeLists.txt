set(PCH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp)
set(TARGET_NAME ${PROJECT_NAME}-tests)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

enable_testing()

add_executable(${TARGET_NAME} "tests.cpp")

target_precompile_headers(${TARGET_NAME} PUBLIC ${PCH_FILE})

set_target_properties(${TARGET_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
    CXX_EXTENSIONS OFF
)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE _WIN32)
elseif(APPLE)
    target_compile_definitions(${TARGET_NAME} PRIVATE __APPLE__)
elseif(UNIX)
    target_compile_definitions(${TARGET_NAME} PRIVATE __linux__)
endif()

add_grammar(
    NAME tree-sitter-json
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-json.git
    VERSION 0.24.8
)

CPMAddPackage(
    URI "gh:catchorg/Catch2@3.14.0"
	OPTIONS "CATCH_INSTALL_DOCS OFF"
			"BUILD_TESTING OFF"
)

if(Catch2_ADDED)
    set(Catch2_Extras ${Catch2_SOURCE_DIR}/extras CACHE INTERNAL "")
endif()

target_compile_options(${TARGET_NAME} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/fp:fast>
        $<$<CONFIG:Debug>:/RTC1>
        /Zc:preprocessor
        /Zc:__cplusplus
    >
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-flto>
    >
)
	
target_link_options(${TARGET_NAME} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
        $<$<CONFIG:Release>:/LTCG>
    >
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
        $<$<CONFIG:Release>:-flto>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:Clang>>:-fuse-ld=lld>
        -static
    >
)

if(MSVC)
    set_property(TARGET ${TARGET_NAME} ${PROJECT_NAME} tree-sitter tree-sitter-json Catch2WithMain Catch2 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE 
	${PROJECT_NAME}
    tree-sitter-json
	Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${Catch2_Extras})
include(CTest)
include(Catch)
catch_discover_tests(
	${TARGET_NAME}
	DISCOVERY_MODE POST_BUILD
)

if(MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
    configure_file(
        ${CMAKE_SOURCE_DIR}/.runsettings
        ${CMAKE_BINARY_DIR}/.runsettings
        COPYONLY
    )
endif()