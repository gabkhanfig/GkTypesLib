#pragma once

#include "Core.h"
#include <Windows.h>
#include <psapi.h>
#include <libloaderapi.h>
#include <string>
#include "BasicTypes.h"
#include <immintrin.h>
#include <iostream>

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

	[[nodiscard]] constexpr static size_t Strlen(const char* str) {
		return std::char_traits<char>::length(str);
	}

	[[nodiscard]] static uint64 strnlen(const char* str, uint64 maxSize) {
		const char* found = (const char*)memchr(str, '\0', maxSize);
		return found ? (uint64)(found - str) : maxSize;
	}

	[[nodiscard]] constexpr static bool StrEqual(const char* str1, const char* str2, size_t num) {
		if (str1 == str2) {
			return true;
		}
		for (int i = 0; i < num; i++) {
			if (str1[i] != str2[i]) {
				return false;
			}
		}
		return true;
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

	/* Expects 32 byte alignment */
	[[nodiscard]] static bool AVX2CheckEqual32ByteBlocks(const void* left, const void* right) {
#if GK_CHECK
		if (!isAligned(left, 32)) {
			std::cout << uint64(left);
			DebugBreak();
		}
		if (!isAligned(right, 32)) {
			std::cout << uint64(right);
			DebugBreak();
		}
#endif
		const __m256i vectorLeft = _mm256_loadu_epi8(left);
		const __m256i vectorRight = _mm256_loadu_epi8(right);
		const __m256i compareResult = _mm256_cmpeq_epi8(vectorLeft, vectorRight);
		const int bitmask = _mm256_movemask_epi8(compareResult);
		return bitmask == -1; // all bits flipped (~0)
	}



}