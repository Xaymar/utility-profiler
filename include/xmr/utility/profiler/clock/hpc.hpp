// Copyright(C) 2021 Michael Fabian Dirks
//
// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.If not, see < https://www.gnu.org/licenses/>.

#ifndef XMR_UTILITY_PROFILER_CLOCK_HPC_HPP
#define XMR_UTILITY_PROFILER_CLOCK_HPC_HPP
#pragma once

#include <chrono>
#include "xmr/utility/profiler/profiler.hpp"

namespace xmr {
	namespace utility {
		namespace profiler {
			namespace clock {
				namespace hpc {
					XMR_UTILITY_PROFILER_FORCEINLINE
					static uint64_t now()
					{
						auto t = std::chrono::high_resolution_clock::now();
						return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
					}
				} // namespace hpc

			} // namespace clock

		} // namespace profiler

	} // namespace utility

} // namespace xmr

#endif
