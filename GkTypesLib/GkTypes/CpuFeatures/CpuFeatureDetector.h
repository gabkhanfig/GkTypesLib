#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include <processthreadsapi.h>
#include <thread>

namespace gk
{
	namespace x86
	{
		[[nodiscard]] inline bool isAvx512Supported() {
			return IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE);
		}

		[[nodiscard]] inline bool isAvx2Supported() {
			return IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
		}	
	}

	/* Get the amount of threads available on this system. Returns a minimum value of 2. */
	[[nodiscard]] inline static const uint32 systemThreadCount() {
		uint32 threadCount = std::thread::hardware_concurrency();
		return threadCount != 0 ? threadCount : 2;
	}
}