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

#ifndef XMR_UTILITY_TSC_PROFILER_HPP
#define XMR_UTILITY_TSC_PROFILER_HPP
#pragma once

#include "xmr/utility/profiler/profiler.hpp"
#ifdef WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace xmr {
	namespace utility {
		namespace profiler {
			namespace clock {
				namespace tsc {
					/** Check if TSC is available to use.
					 * 
					 * @return true if available, otherwise false
					 */
					bool is_available();

					/** Check if TSC is invariant.
					 *
					 * Invariant means that it should be safe to measure code using TSC across multiple cores.
					 * 
					 * @return true if invariant, otherwise false
					 */
					bool is_invariant();

					/** Get the TSC frequency in Hz
					 *
					 * This can vary across cores if TSC is not invariant.
					 * 
					 * @return Best match frequency in Hz of the TSC.
					 */
					uint64_t frequency();

					XMR_UTILITY_PROFILER_FORCEINLINE
					uint64_t now()
					{
#ifdef XMR_UTILITY_PROFILER_USE_RDTSC
						return __rdtsc();
#else
						uint32_t waste;
						return __rdtscp(&waste);
#endif
					}

					/** Convert TSC cycles to seconds.
					 * 
					 * @tparam T 
					 * @param time Time in cycles
					 * @return Time in seconds
					 */
					template<typename T>
					static double to_seconds(T time)
					{
						// TSC to seconds:        s = time / FreqHz
						auto v1 = frequency();
						auto v2 = time;
						return static_cast<double>(v2) / static_cast<double>(v1);
					}

					/** Convert TSC cycles to milliseconds.
					 * 
					 * @tparam T 
					 * @param time Time in cycles
					 * @return Time in milliseconds
					 */
					template<typename T>
					static double to_milliseconds(T time)
					{
						// TSC to milliseconds:  ms = time / (FreqHz / 1000)
						auto v1 = frequency() / 1000;
						auto v2 = time;
						return static_cast<double>(v2) / static_cast<double>(v1);
					}

					/** Convert TSC cycles to microseconds.
					 * 
					 * @tparam T 
					 * @param time Time in cycles
					 * @return Time in microseconds
					 */
					template<typename T>
					static double to_microseconds(T time)
					{
						// TSC to microseconds:  µs = time / (FreqHz / 1000000)
						auto v1 = frequency() / 1000000;
						auto v2 = time;
						return static_cast<double>(v2) / static_cast<double>(v1);
					}

					/** Convert TSC cycles to nanoseconds.
					 * 
					 * @tparam T 
					 * @param time Time in cycles
					 * @return Time in nanoseconds
					 */
					template<typename T>
					static double to_nanoseconds(T time)
					{
						// TSC to nanoseconds:   ns = (time * 1000) / (FreqHz / 1000000)
						auto v1 = frequency() / 1000000;
						auto v2 = time * 1000;
						return static_cast<double>(v2) / static_cast<double>(v1);
					}

					/** Convert TSC cycles to picoseconds.
					 * 
					 * @tparam T 
					 * @param time Time in cycles
					 * @return Time in picoseconds
					 */
					template<typename T>
					static double to_picoseconds(T time)
					{
						// TSC to picoseconds:   ps = (time * 1000000) / (FreqHz / 1000000)
						auto v1 = frequency() / 1000000;
						auto v2 = time * 1000000;
						return static_cast<double>(v2) / static_cast<double>(v1);
					}
				} // namespace tsc

			} // namespace clock

		} // namespace profiler

	} // namespace utility

} // namespace xmr
#endif
