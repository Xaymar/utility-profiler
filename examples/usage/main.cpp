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

#include <cinttypes>
#include <cstdio>
#include <iostream>
#include <xmr/utility/profiler/clock/hpc.hpp>
#include <xmr/utility/profiler/clock/tsc.hpp>
#include <xmr/utility/profiler/profiler.hpp>

#define CYCLES_A 10000
#define CYCLES_B 1000000

static int32_t work()
{
	volatile int32_t x = 1;
	for (uint32_t i = 0; i < CYCLES_A; i++) {
		x += i;
	}
	return x;
}

void measure_tsc()
{
	printf("--------------- TSC @%" PRIu64 "Hz\n", xmr::utility::profiler::clock::tsc::frequency());

	auto profiler = xmr::utility::profiler::profiler();
	for (int n = 0; n < CYCLES_B; n++) {
		auto t = xmr::utility::profiler::clock::tsc::now();
		work();
		auto t2 = xmr::utility::profiler::clock::tsc::now();
		profiler.track(t2, t);
	}

	printf("Events   %10" PRIu64 "\n", profiler.total_events());
	printf("Total    %10" PRIu64 "c %10.2fns\n", profiler.total_time(),
		   xmr::utility::profiler::clock::tsc::to_nanoseconds(profiler.total_time()));
	printf("Average  %10.2fc %10.2fns\n", profiler.average_time(),
		   xmr::utility::profiler::clock::tsc::to_nanoseconds(profiler.average_time()));
	{
		auto f = 99.99;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "c %10.2fns\n", f, v, xmr::utility::profiler::clock::tsc::to_nanoseconds(v));
	}
	{
		auto f = 99.90;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "c %10.2fns\n", f, v, xmr::utility::profiler::clock::tsc::to_nanoseconds(v));
	}
	{
		auto f = 99.00;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "c %10.2fns\n", f, v, xmr::utility::profiler::clock::tsc::to_nanoseconds(v));
	}
}

void measure_hpc()
{
	printf("--------------- HPC\n");

	auto profiler = xmr::utility::profiler::profiler();
	for (int n = 0; n < CYCLES_B; n++) {
		auto t = xmr::utility::profiler::clock::hpc::now();
		work();
		auto t2 = xmr::utility::profiler::clock::hpc::now();
		profiler.track(t2, t);
	}

	printf("Events   %10" PRIu64 "\n", profiler.total_events());
	printf("Total    %10" PRIu64 "ns\n", profiler.total_time());
	printf("Average  %10.2fns\n", profiler.average_time());
	{
		auto f = 99.99;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "ns\n", f, v);
	}
	{
		auto f = 99.90;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "ns\n", f, v);
	}
	{
		auto f = 99.00;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "ns\n", f, v);
	}
}

int32_t main(int32_t argc, const char* argv[])
{
	if (xmr::utility::profiler::clock::tsc::is_available() && xmr::utility::profiler::clock::tsc::is_invariant()) {
		measure_tsc();
	} else {
		printf("No support for invariant TSC, skipping test.\n");
	}
	measure_hpc();

	std::cin.get();
	return 0;
}
