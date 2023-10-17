#pragma once

#include "Core.h"
#include <Windows.h>
#include <psapi.h>
#include <libloaderapi.h>
#include <string>
#include "BasicTypes.h"
#include <immintrin.h>
#include <iostream>
#include "Asserts.h"

namespace gk
{
	[[nodiscard]] static bool IsDataInConstSegment(const void* data)
	{
#ifdef _MSC_VER
		static MODULEINFO moduleInfo;
		static const bool b = GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL), &moduleInfo, sizeof(MODULEINFO));
		static const uintptr_t entryPoint = uintptr_t(moduleInfo.EntryPoint);
		static const uintptr_t endPoint = uintptr_t(moduleInfo.EntryPoint) + uintptr_t(moduleInfo.SizeOfImage);
		return uintptr_t(data) > entryPoint && uintptr_t(data) < endPoint;
#else
		return false;
#endif
	}

	[[nodiscard]] constexpr uint64 UpperPowerOfTwo(uint64 v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v |= v >> 32;
		v++;
		return v;
	}

	[[nodiscard]] static bool isAligned(const void* ptr, uint64 alignment) noexcept {
		auto iptr = reinterpret_cast<uint64>(ptr);
		return !(iptr % alignment);
	}

}