#include "cpu_feature_detector.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#include <thread>

bool gk::x86::isAvx512Supported()
{
	return IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE);
}

bool gk::x86::isAvx2Supported()
{
	return IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
}

gk::u32 gk::systemThreadCount()
{
	u32 threadCount = std::thread::hardware_concurrency();
	return threadCount != 0 ? threadCount : 2;
}
