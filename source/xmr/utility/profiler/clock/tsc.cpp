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

#include "xmr/utility/profiler/clock/tsc.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <vector>
#endif

static bool     tsc_checked      = false;
static bool     tsc_available    = false;
static bool     tsc_invariant    = false;
static uint64_t tsc_frequency_hz = 0;

static void tsc_initialize()
{
	// If we already have the information we need, do nothing.
	if (tsc_checked)
		return;

	int32_t regs[4]              = {0};
	int32_t max_cpuid_function   = 0;
	int32_t max_cpuidex_function = 0;

	{ // Check the limits of CPUID and CPUIDEX
		__cpuidex(regs, 0x0, 0x0);
		max_cpuid_function = regs[0];
		__cpuidex(regs, 0x80000000, 0x0);
		max_cpuidex_function = regs[0];
	}

	if (max_cpuidex_function >= 0x80000001) { // Is the TSC available?
		/* https://www.felixcloutier.com/x86/cpuid
		 * EAX Extended Processor Signature and Feature Bits.
		 * EBX Reserved.
		 * ECX Bit 00: LAHF/SAHF available in 64-bit mode.*
		 *     Bits 04 - 01: Reserved.
		 *     Bit 05: LZCNT.
		 *     Bits 07 - 06: Reserved.
		 *     Bit 08: PREFETCHW.
		 *     Bits 31 - 09: Reserved.
		 * EDX Bits 10 - 00: Reserved.
		 *     Bit 11: SYSCALL/SYSRET.**
		 *     Bits 19 - 12: Reserved = 0.
		 *     Bit 20: Execute Disable Bit available.
		 *     Bits 25 - 21: Reserved = 0.
		 *     Bit 26: 1-GByte pages are available if 1.
		 *     Bit 27: RDTSCP and IA32_TSC_AUX are available if 1.
		 *     Bit 28: Reserved = 0.
		 *     Bit 29: Intel® 64 Architecture available if 1.
		 *     Bits 31 - 30: Reserved = 0.
		 * NOTES:
		 * * LAHFandSAHFarealwaysavailableinothermodes,regardlessoftheenumerationofthisfeatureflag.
		 * ** Intel processors support SYSCALL and SYSRET only in 64-bit mode. This feature flag is always enumerated as 0 outside 64-bit mode.
		 */
		__cpuidex(regs, 0x80000001, 0x0);
		tsc_available = ((regs[3] >> 27) & 1) != 0;
	}

	if (tsc_available && (max_cpuidex_function >= 0x80000007)) { // Is the TSC invariant?
		__cpuidex(regs, 0x80000007, 0x0);
		tsc_invariant = ((regs[3] >> 8) & 1) != 0;
	}

	if (tsc_available) { // Figure out TSC frequency if possible.

		if ((tsc_frequency_hz == 0) && (max_cpuid_function >= 0x15)) { // CPUID 0x15: Core Crystal Clock
			__cpuidex(regs, 0x15, 0x0);
			if ((regs[0] != 0) && (regs[1] != 0) && (regs[2] != 0)) {
				// We have TSC information from CPUID.
				tsc_frequency_hz = (regs[2] * regs[0]) / regs[1];
			}
		}

		if ((tsc_frequency_hz == 0)
			&& (max_cpuid_function >= 0x16)) { // CPUID 0x16: Base Frequency, Maximum Frequency, Bus Frequency, ...
			__cpuidex(regs, 0x16, 0x0);
			if ((regs[0] & 0xFFFF) != 0) {
				tsc_frequency_hz = static_cast<uint64_t>(regs[0] & 0xFFFF) * 1000000;
			}
		}

#ifdef _WIN32
		if ((tsc_frequency_hz == 0) && tsc_invariant) {
			// TSC Frequency matches P0 state if the TSC is invariant.
			uint32_t frequency;
			DWORD    frequency_size = sizeof(frequency);

			std::vector<wchar_t> key;
			wchar_t*             fmtstr = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%ld";
			key.resize(swprintf(nullptr, 0, fmtstr, GetCurrentProcessorNumber()) + 1);
			swprintf(key.data(), key.size(), fmtstr, GetCurrentProcessorNumber());

			//   Computer\HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\#\~Mhz
			DWORD ec = RegGetValueW(HKEY_LOCAL_MACHINE, key.data(), L"~Mhz", RRF_RT_REG_DWORD | RRF_ZEROONFAILURE, NULL,
									&frequency, &frequency_size);
			if (ec == ERROR_SUCCESS) {
				tsc_frequency_hz = static_cast<uint64_t>(frequency) * 1000000; // Mhz -> Hz
			}
		}
#endif
	}

	tsc_checked = true;
}

bool xmr::utility::profiler::clock::tsc::is_available()
{
	tsc_initialize();

	return tsc_available;
}

bool xmr::utility::profiler::clock::tsc::is_invariant()
{
	tsc_initialize();

	return tsc_invariant;
}

uint64_t xmr::utility::profiler::clock::tsc::frequency()
{
	tsc_initialize();

	return tsc_frequency_hz;
}
