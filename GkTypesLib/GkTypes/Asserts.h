#pragma once

#include "Core.h"
#include <iostream>

#define STR(s) #s

#if _MSC_VER
#define GK_DEBUG_BREAK() __debugbreak()
#else
#define GK_DEBUG_BREAK()
#endif

#if GK_CHECK

#define PRINT_ASSERT_MESSAGE(message) std::cout << "[ASSERT FAILED]: File: " << __FILE__ << " Line: " << __LINE__ << '\n' << message

#define gk_assert(condition) {\
	if (!(condition)) {\
		if(std::is_constant_evaluated()) {throw;}\
		PRINT_ASSERT_MESSAGE(STR(condition));\
		GK_DEBUG_BREAK();\
	}\
}

/* Message can be a const char*, gk::string, std::string, or anything else that can printed using std::cout. */
#define gk_assertm(condition, message) {\
	if(!(condition)) {\
		if(std::is_constant_evaluated()) {throw;}\
		PRINT_ASSERT_MESSAGE("condition: " << STR(condition) << "\n" << message);\
		GK_DEBUG_BREAK();\
	}\
}

#define gk_assertNotNull(ptr) {\
	if(ptr == nullptr) {\
		if(std::is_constant_evaluated()) {throw;}\
		PRINT_ASSERT_MESSAGE(STR(ptr) " == nullptr");\
		GK_DEBUG_BREAK();\
	}\
}

#else

#define gk_assert(condition)
#define gk_assertm(condition, message)
#define gk_assertNotNull(ptr)

#endif