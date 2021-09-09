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

// Should we force inlining whenever possible? (Improves accuracy at the cost of code size)
#ifdef XMR_UTILITY_PROFILER_ENABLE_FORCEINLINE
#if defined(__GNUC__) || defined(__GNUG__)
#define XMR_UTILITY_PROFILER_FORCEINLINE __attribute__((always_inline))
#define XMR_UTILITY_PROFILER_NOINLINE __attribute__((noinline)) inline
#elif defined(__clang__)
#define XMR_UTILITY_PROFILER_FORCEINLINE __attribute__((always_inline))
#define XMR_UTILITY_PROFILER_NOINLINE __attribute__((noinline)) inline
#elif defined(_MSC_VER)
#define XMR_UTILITY_PROFILER_FORCEINLINE __forceinline
#define XMR_UTILITY_PROFILER_NOINLINE __declspec(noinline) inline
#else
#define XMR_UTILITY_PROFILER_FORCEINLINE inline
#define XMR_UTILITY_PROFILER_NOINLINE
#endif
#else
#define XMR_UTILITY_PROFILER_FORCEINLINE inline
#define XMR_UTILITY_PROFILER_NOINLINE
#endif

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include "profiler.hpp"

// Available on more systems, but requires additional instructions like lfence and cpuid.
//#define XMR_UTILITY_PROFILER_USE_RDTSC

#ifdef WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace xmr {
	namespace utility {
		namespace profiler {
			/** TSC Profiler
			 *
			 * When the TSC is available, this provides a way to measure how long any work
			 * takes with incredible precision. If the TSC is invariant, it can even allow
			 * timing across different CPU cores.
			 */
			class tsc_profiler {
				std::mutex                   _lock;         // Prevent out of order modification of elements.
				std::map<uint64_t, uint64_t> _timings;      // Map of cycles<->calls
				uint64_t                     _total_counts; // Total number of calls.

				public:
				~tsc_profiler(){};

				/** Create a new TSC-based profiler.
				 *
				 * This will throw an exception if TSC is unavailable.
				 */
				tsc_profiler();

				/** Start profiling
				 * 
				 * @return The start time of the profiling, to be passed into stop().
				 */
				XMR_UTILITY_PROFILER_FORCEINLINE
				uint64_t start()
				{
					// 1. Collect the start TSC value and return it.
#ifdef XMR_UTILITY_PROFILER_USE_RDTSC
					return __rdtsc();
#else
					uint32_t waste;
					return __rdtscp(&waste);
#endif
				};

				/** Stop profiling and insert time into the map.
				 * 
				 * @param time The time value returned by start().
				 * @return The difference in time between start() and stop().
				*/
				XMR_UTILITY_PROFILER_FORCEINLINE
				uint64_t stop(uint64_t time)
				{
					// 1. Collect the end TSC value.
#ifdef XMR_UTILITY_PROFILER_USE_RDTSC
					uint64_t time_end = __rdtsc();
#else
					uint32_t waste;
					uint64_t time_end = __rdtscp(&waste);
#endif

					// 2. Verify that the end time is after the start time.
					uint64_t diff;
					if (time_end >= time) {
						// 3. Get the difference between the two.
						diff = time_end - time;
					} else { // Time has wrapped back to 0.
						// 3. Calculate the difference as if we just wrapped over the maximum value.
						diff = (std::numeric_limits<uint64_t>::max() - time_end) + time;
					}

					std::unique_lock<std::mutex> l(_lock);

					// 4. Insert into map.
					auto                         entry = _timings.find(diff);
					if (entry != _timings.end()) {
						entry->second++;
					} else {
						_timings.emplace(diff, uint64_t(1));
					}

					// 5. Increment total count.
					_total_counts++;

					// 6. Return difference.
					return diff;
				};

				/** Clear any recorded profiler timings.
				 */
				void clear()
				{
					std::unique_lock<std::mutex> l(_lock);
					_total_counts = 0;
					_timings.clear();
				}

				std::map<uint64_t, uint64_t> get()
				{
					std::unique_lock<std::mutex> l(_lock);
					return _timings;
				}

				public /*Statistics*/:

				/** Get the total number of profiled events.
				 * 
				 * @return Total number of profiled events.
				 */
				uint64_t total_events()
				{
					return _total_counts;
				}

				/** Get the total time spent in events.
				 * 
				 * @return Total time spent in events.
				*/
				uint64_t total_time()
				{
					uint64_t                     time = 0;
					std::unique_lock<std::mutex> l(_lock);
					for (auto kv : _timings) {
						time += kv.first * kv.second;
					}
					return time;
				}

				/** Get the average time spent in events.
				 * 
				 * @return Average time spent in events.
				 */
				double average_time()
				{
					return static_cast<double>(total_time()) / static_cast<double>(total_events());
				}

				/** Percentile (by time)
				 * 
				 * @tparam T 
				 * @param percentile The percentile (as 0.0 - 1.0) to find a match for.
				 * @param time The time that matches the percentile.
				 * @param amount The number of events matching the time.
				 * @return The time that matches the percentile.
				*/
				template<typename T>
				uint64_t percentile_time(T percentile, uint64_t& time, uint64_t& amount)
				{
					static T                    threshold = static_cast<T>(0.000001);
					std::lock_guard<std::mutex> l(_lock);

					// Don't crash if nothing has been tracked.
					if (_total_counts == 0) {
						return 0;
					}

					// Submit correct response at <0% and >100%.
					if ((percentile - threshold) <= (static_cast<T>(0.0) + threshold)) {
						// Percentile is "equal" or below 0.
						return _timings.begin()->first;
					} else if ((percentile + threshold) >= (static_cast<T>(1.0) - threshold)) {
						// Percentile is "equal" or above 1.
						return _timings.rbegin()->first;
					}

					// Calculate overall delta.
					uint64_t tmin   = _timings.begin()->first;
					uint64_t tmax   = _timings.rbegin()->first;
					uint64_t tdelta = tmax - tmin;
					double   delta  = static_cast<double>(tdelta);

					// Find percentile (map order is smallest to largest).
					for (auto itr : _timings) {
						uint64_t cdelta  = itr.first - tmin;
						double   fdelta  = static_cast<double>(cdelta);
						T        percent = static_cast<T>(fdelta) / static_cast<T>(delta);
						if (percent >= (percentile - threshold)) {
							return itr.first;
						}
					}

					return _timings.rbegin()->first;
				}

				/** Percentile (by events)
				 * 
				 * @tparam T 
				 * @param percentile The percentile (as 0.0 - 1.0) to find a match for.
				 * @return The time that matches the percentile.
				*/
				template<typename T>
				uint64_t percentile_events(T percentile)
				{
					static T                    threshold = static_cast<T>(0.000001);
					std::lock_guard<std::mutex> l(_lock);

					// Don't crash if nothing has been tracked.
					if (_total_counts == 0) {
						return 0;
					}

					// Submit correct response at <0% and >100%.
					if ((percentile - threshold) <= (static_cast<T>(0.0) + threshold)) {
						// Percentile is "equal" or below 0.
						return _timings.begin()->first;
					} else if ((percentile + threshold) >= (static_cast<T>(1.0) - threshold)) {
						// Percentile is "equal" or above 1.
						return _timings.rbegin()->first;
					}

					// Calculate overall delta.
					uint64_t accum = 0;
					uint64_t max   = total_events();
					double   delta = static_cast<double>(max);

					// Find percentile (map order is smallest to largest).
					for (auto itr : _timings) {
						accum += itr.second;
						T percent = static_cast<T>(accum) / static_cast<T>(max);
						if (percent >= (percentile - threshold)) {
							return itr.first;
						}
					}

					return _timings.rbegin()->first;
				}

				public:
				/** Check if TSC is available to use.
				 * 
				 * @return true if available, otherwise false
				 */
				static bool is_available();

				/** Check if TSC is invariant.
				 *
				 * Invariant means that it should be safe to measure code using TSC across multiple cores.
				 * 
				 * @return true if invariant, otherwise false
				 */
				static bool is_invariant();

				/** Get the TSC frequency in Hz
				 *
				 * This can vary across cores if TSC is not invariant.
				 * 
				 * @return Best match frequency in Hz of the TSC.
				 */
				static uint64_t frequency();

				template<typename T>
				static double to_seconds(T time)
				{
					// TSC to seconds:        s = time / FreqHz
					uint64_t v1 = frequency();
					T        v2 = time;
					double   v  = static_cast<double>(v2) / static_cast<double>(v1);
					return v;
				}

				template<typename T>
				static double to_milliseconds(T time)
				{
					// TSC to seconds:        s = time / FreqHz
					// TSC to milliseconds:  ms = time / (FreqHz / 1000)
					uint64_t v1 = frequency() / 1000;
					T        v2 = time;
					double   v  = static_cast<double>(v2) / static_cast<double>(v1);
					return v;
				}

				template<typename T>
				static double to_microseconds(T time)
				{
					// TSC to microseconds:  µs = time / (FreqHz / 1000000)
					uint64_t v1 = frequency() / 1000000;
					T        v2 = time;
					double   v  = static_cast<double>(v2) / static_cast<double>(v1);
					return v;
				}

				template<typename T>
				static double to_nanoseconds(T time)
				{
					// TSC to nanoseconds:   ns = (time * 1000) / (FreqHz / 1000000)
					uint64_t v1 = frequency() / 1000000;
					T        v2 = time * 1000;
					double   v  = static_cast<double>(v2) / static_cast<double>(v1);
					return v;
				}

				template<typename T>
				static double to_picoseconds(T time)
				{
					// TSC to picoseconds:   ps = (time * 1000000) / (FreqHz / 1000000)
					uint64_t v1 = frequency() / 1000000;
					T        v2 = time * 1000000;
					double   v  = static_cast<double>(v2) / static_cast<double>(v1);
					return v;
				}
			};
		} // namespace profiler

	} // namespace utility

} // namespace xmr

#endif
