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

#ifndef XMR_UTILITY_PROFILER_HPP
#define XMR_UTILITY_PROFILER_HPP
#pragma once
#include "xmr/utility/profiler/config.hpp"

#include <map>
#include <mutex>

namespace xmr {
	namespace utility {
		namespace profiler {
			/** Single-Type Event Profiler
		     */
			class profiler {
				std::mutex                   _lock;         // Prevent out of order modification of elements.
				std::map<uint64_t, uint64_t> _timings;      // Map of nanoseconds<->calls
				uint64_t                     _total_counts; // Total number of calls.

				public:
				~profiler(){};

				/** Create a new profiler.
				 */
				profiler() : _lock(), _timings(), _total_counts(0){};

				/** Track a profiled event.
				 *
				 * Inserts the given time difference into the timings list.
				 * 
				 * @param time_end The end time recorded for the event.
				 * @param time_start The start time recorded for the event.
				 * @return Difference between time_end and time_start.
				 */
				uint64_t track(uint64_t time_end, uint64_t time_start)
				{
					// Calculate time difference.
					uint64_t difference;
					if (time_end >= time_start) {
						// Time has not wrapped over 0, we can simply calculate the difference between a and b.
						difference = time_end - time_start;
					} else { // Time has wrapped over 0.
						// Time has wrapped over 0, need to subtract b from max.int then add a.
						difference = (std::numeric_limits<uint64_t>::max() - time_end) + time_start;
					}

					// Try and insert the new time into the map.
					std::unique_lock<std::mutex> l(_lock);
					auto                         entry = _timings.find(difference);
					if (entry != _timings.end()) {
						entry->second++;
					} else {
						_timings.emplace(difference, uint64_t(1));
					}

					// Increment total count.
					_total_counts++;

					// Return difference.
					return difference;
				};

				/** Clear any recorded profiler timings.
				 */
				void clear()
				{
					std::unique_lock<std::mutex> l(_lock);
					_total_counts = 0;
					_timings.clear();
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
			};
		} // namespace profiler

	} // namespace utility

} // namespace xmr

#endif
