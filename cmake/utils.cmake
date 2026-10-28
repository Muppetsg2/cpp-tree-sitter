set(CPP_TS_NECESSARY_COMPILE_OPTIONS
	$<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
		/Zc:__cplusplus
	>
)

set(TS_ERROR_COMPILE_OPTIONS
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
		$<$<CONFIG:Debug>:/RTC1>
        /wd4018 # disable 'signed/unsigned mismatch'
        /wd4232 # disable 'nonstandard extension used'
        /wd4244 # disable 'possible loss of data'
        /wd4267 # disable 'possible loss of data (size_t)'
        /wd4701 # disable 'potentially uninitialized local variable'
        /we4022 # treat 'incompatible types' as an error
        /W4
    >
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
        -Wno-unused-but-set-variable
		-Wno-conversion
		-Wall
		-Wextra
        -Wshadow
        -Wpedantic
    >
)

set(TS_NO_IPO_COMPILE_OPTIONS
	$<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
		$<$<CONFIG:Release>:/O2>
		$<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/fp:fast>
        $<$<CONFIG:Debug>:/RTC1>
        /Zc:preprocessor
	>
	$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
		$<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-flto>
	>
)

set(TS_NO_IPO_LINK_OPTIONS
	$<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:
		$<$<CONFIG:Release>:/LTCG>
	>
	$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:GNU>>>:
		$<$<CONFIG:Release>:-flto>
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:Clang>>:-fuse-ld=lld>
	>
)

# Directory of the cpp-tree-sitter CMake modules. CMAKE_SOURCE_DIR must not be used for this, because it points to
# the consumer's project when cpp-tree-sitter is added as a dependency.
set(CPP_TS_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "Directory of the cpp-tree-sitter CMake modules")

function(cpp_ts_print_config_option NAME)
	message(STATUS "${NAME} = ${${NAME}}")
endfunction()

function(cpp_ts_find_and_copy_lib)
	set(options POST_BUILD)
	set(oneValueArgs TARGET)
	set(multiValueArgs NAMES PATHS)

	cmake_parse_arguments(PARSE_ARGV 0 arg
		"${options}" "${oneValueArgs}" "${multiValueArgs}"
	)

	if(NOT arg_TARGET)
		message(FATAL_ERROR "cpp_ts_find_and_copy_lib requires TARGET")
	endif()

	if(NOT arg_NAMES)
		message(FATAL_ERROR "cpp_ts_find_and_copy_lib requires NAMES")
	endif()

	if(NOT arg_PATHS)
		message(FATAL_ERROR "cpp_ts_find_and_copy_lib requires PATHS")
	endif()

	if(arg_NAMES)
        list(GET arg_NAMES 0 FIRST_NAME)
    else()
        set(FIRST_NAME "unknown")
    endif()

	if(BUILD_SHARED_LIBS AND WIN32)
		if(arg_POST_BUILD)
			message(STATUS "Postponing search for ${FIRST_NAME} until build time")

            add_custom_command(TARGET ${arg_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} 
                    -D NAMES="${arg_NAMES}" 
                    -D PATHS="${arg_PATHS}" 
                    -D DEST="$<TARGET_FILE_DIR:${arg_TARGET}>"
                    -P "${CPP_TS_CMAKE_DIR}/copy_helper.cmake"
                COMMENT "Searching and copying ${FIRST_NAME} at build time"
            )
		else()
			find_file(DLL_PATH_FOR_${FIRST_NAME}
				NAMES ${arg_NAMES}
				PATHS ${arg_PATHS}
				NO_DEFAULT_PATH
			)

			if(DLL_PATH_FOR_${FIRST_NAME})
				set(CURRENT_DLL ${DLL_PATH_FOR_${FIRST_NAME}})
				message(STATUS "Found ${FIRST_NAME} file: ${CURRENT_DLL}")
        
				add_custom_command(TARGET ${arg_TARGET} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
						"${CURRENT_DLL}"
						"$<TARGET_FILE_DIR:${arg_TARGET}>"
					COMMENT "Copying ${FIRST_NAME} to target directory"
				)
			else()
				message(WARNING "Couldn't find ${FIRST_NAME} file")
			endif()
		endif()
    endif()
endfunction()
