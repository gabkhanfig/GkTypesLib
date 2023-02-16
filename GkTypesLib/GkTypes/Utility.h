#pragma once

#include <Windows.h>
#include <psapi.h>
#include <libloaderapi.h>
#include <string>

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
}