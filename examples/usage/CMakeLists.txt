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

project(
	example_usage
)

add_executable(${PROJECT_NAME}
	"main.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		xmr_utility_profiler
)

add_dependencies(examples example_usage)
