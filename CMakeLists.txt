# Copyright (C) 2021 Michael Fabian Dirks
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)

# Detect if we are building by ourselves or as part of something else.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
	set(GROUPED OFF)
	set(PREFIX "")
else()
	set(GROUPED ON)
	set(PREFIX "XUProfiler_")
endif()
set(LOGPREFIX "XUP: ")

################################################################################
# Options
################################################################################
set(${PREFIX}BUILD_EXAMPLES ON CACHE BOOL "Build Examples")
set(${PREFIX}BUILD_TESTS ON CACHE BOOL "Build Tests")


################################################################################
# Versioning
################################################################################

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
set(VERSION_COMMIT "00000000")

# Check if we are in a git repository.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	# Try and figure out where git is.
	find_program(GIT git
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
	)

	if(GIT)
		set(GIT_RESULT)
		set(GIT_OUTPUT)
		set(GIT_ERROR)

		execute_process(
			COMMAND "${GIT}" describe --tags --long --match "[0-9]*.[0-9]*.[0-9]*" --abbrev=8 HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE GIT_OUTPUT
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)

		if(GIT_RESULT EQUAL 0)
			string(REPLACE "-" "." GIT_OUTPUT "${GIT_OUTPUT}")
			string(REPLACE "." ";" GIT_OUTPUT "${GIT_OUTPUT}")

			# Parse Version
			list(GET GIT_OUTPUT 0 VERSION_MAJOR)
			list(GET GIT_OUTPUT 1 VERSION_MINOR)
			list(GET GIT_OUTPUT 2 VERSION_PATCH)
			list(GET GIT_OUTPUT 3 VERSION_TWEAK)
			list(GET GIT_OUTPUT 4 VERSION_COMMIT)

			# Patch needs additional parsing.
			# This may be a [0-9]*[a-z]*[0-9]+ string.
			string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
			set(VERSION_PATCH "${CMAKE_MATCH_1}")
			if(CMAKE_MATCH_2)
				set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
			else()
				set(VERSION_SUFFIX "")
			endif()
		else()
			message(WARNING "${LOGPREFIX}Failed to detect version, using default instead.")
		endif()
	endif()
else()
	message(STATUS "${LOGPREFIX}Not a git repository, automatic version detection disabled.")
endif()

# Allow manual overrides of the detected version.
set(${PREFIX}VERSION "" CACHE STRING "Override StreamFX version with this string. Format: Major.Minor.Patch[Suffix][-Tweak[-Commit8c]]")
if(NOT (${PREFIX}VERSION STREQUAL ""))
	string(REPLACE "-" "." T_VERSION "${${PREFIX}VERSION}")
	string(REPLACE "." ";" T_VERSION "${${PREFIX}VERSION}")

	list(LENGTH T_VERSION T_VERSIONLEN)
	list(GET T_VERSION 0 VERSION_MAJOR)
	list(GET T_VERSION 1 VERSION_MINOR)
	list(GET T_VERSION 2 VERSION_PATCH)
	if (T_VERSIONLEN GREATER_EQUAL 3)
		list(GET T_VERSION 3 VERSION_TWEAK)
	else()
		set(VERSION_BUILD 0)
	endif()
	if (T_VERSIONLEN GREATER_EQUAL 4)
		list(GET T_VERSION 4 VERSION_COMMIT)
	else()
		set(VERSION_COMMIT "")
	endif()

	# Patch needs additional parsing.
	# This may be a [0-9]*[a-z]*[0-9]+ string.
	string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
	set(VERSION_PATCH "${CMAKE_MATCH_1}")
	if(CMAKE_MATCH_2)
		set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
	else()
		set(VERSION_SUFFIX "")
	endif()
endif()

# Generate Version String
if(NOT (VERSION_COMMIT STREQUAL ""))
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${VERSION_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()
message(STATUS "${LOGPREFIX}Version ${VERSION_STRING}")

################################################################################
# Define Library
################################################################################
project(
	profiler
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
	DESCRIPTION "Simple but effective way to profile code."
	HOMEPAGE_URL "https://xaymar.com/"
)

set(PRIVATE_SOURCE
	"source/xmr/utility/profiler/profiler.cpp"
	"source/xmr/utility/profiler/clock/hpc.cpp"
	"source/xmr/utility/profiler/clock/tsc.cpp"
)
set(PUBLIC_INCLUDE
	"include/xmr/utility/profiler/profiler.hpp"
	"include/xmr/utility/profiler/clock/hpc.hpp"
	"include/xmr/utility/profiler/clock/tsc.hpp"
)

add_library(${PROJECT_NAME}
	${PRIVATE_SOURCE}
	${PUBLIC_INCLUDE}
)

source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Sources" FILES ${PRIVATE_SOURCE})
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Headers" FILES ${PUBLIC_INCLUDE})

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/source"
		"${PROJECT_SOURCE_DIR}/include"
	INTERFACE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:include>"
)

target_link_libraries(${PROJECT_NAME}
	INTERFACE
)

################################################################################
# Install Library
################################################################################

# Binaries
install(
	TARGETS ${PROJECT_NAME}
	EXPORT xaymar::utility::profiler
	ARCHIVE LIBRARY RUNTIME FRAMEWORK BUNDLE
	PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
)
if(MSVC AND (BUILD_SHARED_LIBS))
	install(
		FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
		TYPE BIN
		OPTIONAL
	)
endif()

# Public Headers
install(
	DIRECTORY "${PROJECT_SOURCE_DIR}/include/."
	TYPE INCLUDE
)

################################################################################
# Examples
################################################################################
if (${${PREFIX}BUILD_EXAMPLES})
	add_subdirectory("examples")
endif()

################################################################################
# Tests
################################################################################
if (${${PREFIX}BUILD_TESTS})
	add_subdirectory("tests")
endif()
