#include "doctest_proxy.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

void gk::internal::debugBreak()
{
	DebugBreak();
}
