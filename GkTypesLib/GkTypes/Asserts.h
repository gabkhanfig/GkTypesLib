#pragma once

#include "Core.h"

#define STR(s) #s

#if GK_CHECK

#define PRINT_ASSERT_MESSAGE(message) std::cout << "[ASSERT FAILED]: File: " << __FILE__ << " Line: " << __LINE__ << '\n' << message

#define gk_assert(condition) {\
	if (!(condition)) {\
		PRINT_ASSERT_MESSAGE(STR(condition));\
		DebugBreak();\
	}\
}

/* Message can be a const char*, gk::string, std::string, or anything else that can printed using std::cout. */
#define gk_assertm(condition, message) {\
	if(!(condition)) {\
		PRINT_ASSERT_MESSAGE(STR(condition) << " " << message);\
		DebugBreak();\
	}\
}

#define gk_assertNotNull(ptr) {\
	if(ptr == nullptr) {\
		PRINT_ASSERT_MESSAGE(STR(ptr) " == nullptr");\
		DebugBreak();\
	}\
}

#else

#define gk_assert(condition)
#define gk_assertm(condition, message)
#define gk_assertNotNull(ptr)

#endif