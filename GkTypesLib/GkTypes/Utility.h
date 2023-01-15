#pragma once

#include <Windows.h>
#include <psapi.h>
#include <libloaderapi.h>

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
}