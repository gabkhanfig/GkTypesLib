#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include <processthreadsapi.h>

namespace gk
{
	namespace x86
	{
		bool isAvx512Supported() {
			return IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE);
		}

		bool isAvx2Supported() {
			return IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
		}
	}
}