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
#include <xmr/utility/hpc_profiler.hpp>
#include <xmr/utility/tsc_profiler.hpp>

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
	printf("--------------- TSC @%" PRIu64 "Hz\n", xmr::utility::profiler::tsc_profiler::frequency());

	auto    profiler = xmr::utility::profiler::tsc_profiler();
	for (int n = 0; n < CYCLES_B; n++) {
		auto t = profiler.start();
		work();
		profiler.stop(t);
	}

	printf("Events   %10" PRIu64 "\n", profiler.total_events());
	printf("Total    %10" PRIu64 "c %10.2fns\n", profiler.total_time(),
		   xmr::utility::profiler::tsc_profiler::to_nanoseconds(profiler.total_time()));
	printf("Average  %10.2fc %10.2fns\n", profiler.average_time(),
		   xmr::utility::profiler::tsc_profiler::to_nanoseconds(profiler.average_time()));
	{
		auto f = 99.99;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "c %10.2fns\n", f, v, xmr::utility::profiler::tsc_profiler::to_nanoseconds(v));
	}
	{
		auto f = 99.90;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "c %10.2fns\n", f, v, xmr::utility::profiler::tsc_profiler::to_nanoseconds(v));
	}
	{
		auto f = 99.00;
		auto v = profiler.percentile_events(f / 100);
		printf("%5.2file %10" PRIu64 "c %10.2fns\n", f, v, xmr::utility::profiler::tsc_profiler::to_nanoseconds(v));
	}
}

void measure_hpc()
{
	printf("--------------- HPC\n");

	auto profiler = xmr::utility::profiler::hpc_profiler();
	for (int n = 0; n < CYCLES_B; n++) {
		auto t = profiler.start();
		work();
		profiler.stop(t);
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
	if (xmr::utility::profiler::tsc_profiler::is_available() && xmr::utility::profiler::tsc_profiler::is_invariant()) {
		measure_tsc();
	} else {
		printf("No support for invariant TSC, skipping test.\n");
	}
	measure_hpc();

	// TSC =    3400000000
	// ns  =    1000000000
	// ps  = 1000000000000

	// tsc = 1/3 400 000 000 = 0.294ns
	// ns  = 1/1 000 000 000 = 1.000ns

	std::cin.get();
	return 0;
}
