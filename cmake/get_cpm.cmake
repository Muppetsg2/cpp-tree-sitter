# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: Copyright (c) 2019-2023 Lars Melchior and contributors

# Adds a package with CPMAddPackage (same arguments). Every package of cpp-tree-sitter goes through it.
macro(cpp_ts_add_package)
  set(_cpp_ts_saved_disable_source_changes "${CMAKE_DISABLE_SOURCE_CHANGES}")
  set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
  CPMAddPackage(${ARGN})
  set(CMAKE_DISABLE_SOURCE_CHANGES "${_cpp_ts_saved_disable_source_changes}")
  unset(_cpp_ts_saved_disable_source_changes)
endmacro()

# A parent project (or the user) may already provide CPM: reuse it instead of downloading another copy.
if(COMMAND CPMAddPackage)
  return()
endif()

set(CPM_DOWNLOAD_VERSION 0.43.2)
set(CPM_HASH_SUM "49a3bef91ceb65bb66d57255e12d1ffd22abc2f6408fa9fe4534c544a2f232aa")

# Default source cache: the build directory, so nothing is written outside of it and deleting the build directory removes
# everything. Only applied when cpp-tree-sitter is the top-level project and nobody configured a cache, so consumers keep
# full control over CPM_SOURCE_CACHE (variable or environment variable). CMakePresets.json sets a cache shared by all
# presets (<source>/out/cpm_cache), so e.g. wasmtime is downloaded once.
if(NOT CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE} AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm_cache")
endif()

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

# Expand relative path. This is important if the provided path contains a tilde (~)
get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)

unset(CPM_DIRECTORY CACHE)

message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
file(DOWNLOAD
     https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
     ${CPM_DOWNLOAD_LOCATION} EXPECTED_HASH SHA256=${CPM_HASH_SUM}
)

include(${CPM_DOWNLOAD_LOCATION})
