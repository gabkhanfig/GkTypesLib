// use this instead of doctest.h
#pragma once

#include <iostream>
#include <type_traits>



namespace gk
{
	namespace internal
	{
		template<typename S, typename T, typename = void>
		struct is_to_stream_writable : std::false_type {};

		template<typename S, typename T>
		struct is_to_stream_writable<S, T,
			std::void_t<  decltype(std::declval<S&>() << std::declval<T>())  > >
			: std::true_type {};

		void debugBreak();

		template<typename T>
		void debugPrint(const char* expressionString, const T& obj) {
			std::cout << expressionString;
			if constexpr (is_to_stream_writable<std::ostream, T>::value) {
				if constexpr (std::is_pointer_v<T>) {
					if (obj == nullptr) {
						std::cout << " ( " << "nullptr" << " )";
					}
					else {
						std::cout << " ( " << obj << " )";
					}
				}
				else if constexpr (std::is_same_v<bool, T>) {
					std::cout << " ( " << (obj ? "true" : "false") << " )";
				}
				else {
					std::cout << " ( " << obj << " )";
				}
			}
		}
	}
}

#define _GK_STRINGIFY_IMPL(s) #s
#define GK_STRINGIFY(s) _GK_STRINGIFY_IMPL(s)

#define GK_DEBUG_PRINT(val) gk::internal::debugPrint(GK_STRINGIFY(val), val)

#if defined GK_TYPES_LIB_DEBUG || defined GK_TYPES_LIB_TEST
namespace gk {
	static constexpr bool RUNTIME_ASSERTS_ON = true;
}

#define DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"

#define test_case DOCTEST_TEST_CASE
#define subcase DOCTEST_SUBCASE
#define test_suite DOCTEST_TEST_SUITE

#define comptime_test_case(test_case_name, test_name, test_block) \
consteval bool CompTimeTest_ ## test_case_name ## _ ## test_name() ## { test_block return true; } \
static_assert(CompTimeTest_ ## test_case_name ## _ ## test_name ## ());

#else
namespace gk {
	static constexpr bool RUNTIME_ASSERTS_ON = false;
}

#define test_case
#define subcase
#define test_suite

#define comptime_test_case(test_case_name, test_name, test_block)

#endif

/**
* Assert that a given condition is true. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, runtime checks will be disabled.
* Also works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
* 
* @param cond: Condition to assert true
*/
#define check(cond) \
if(std::is_constant_evaluated()) { \
	if(!(cond)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) { \
	if(doctest::getContextOptions() == nullptr) { \
		if(!(cond)) { \
			std::cout << __FILE__ << ' ' << __LINE__ << '\n';\
			std::cout << "check failed:\n	";\
			GK_DEBUG_PRINT(cond);\
			gk::internal::debugBreak(); \
		}\
	}\
	else{ \
		DOCTEST_FAST_CHECK_UNARY(cond);\
	}\
}

/**
* Assert that a given condition is false. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, runtime checks will be disabled.
* Also works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param cond: Condition to assert false
*/
#define check_not(cond) \
if(std::is_constant_evaluated()) { \
	if(cond) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) { \
	if(doctest::getContextOptions() == nullptr) { \
		if(cond) { \
			std::cout << __FILE__ << ' ' << __LINE__ << '\n';\
			std::cout << "check false failed:\n	";\
			GK_DEBUG_PRINT(cond);\
			gk::internal::debugBreak(); \
		}\
	}\
	else{ \
		DOCTEST_FAST_CHECK_UNARY_FALSE(cond);\
	}\
}

/**
* check_message will not log anything outside of tests.
* https://github.com/doctest/doctest/blob/master/doc/markdown/assertions.md#using-asserts-out-of-a-testing-context
*/

/**
* Assert that a given condition is true. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, runtime checks will be disabled.
* During tests, will print an error message if the assert fails. 
* Also works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param cond: Condition to assert true
*/
#define check_message(cond, ...)\
if(std::is_constant_evaluated()) { \
	if(!(cond)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) { \
	/*std::cout << "runtime assert\n";*/ \
	if(doctest::getContextOptions() == nullptr) { \
		if(!(cond)) { \
			std::cout << __FILE__ << ' ' << __LINE__ << '\n';\
			std::cout << "check failed:\n	";\
			GK_DEBUG_PRINT(cond);\
			gk::internal::debugBreak(); \
		}\
	}\
	else{ \
		DOCTEST_CHECK_MESSAGE(cond, __VA_ARGS__);\
	}\
}

/**
* check_false_message will not log anything outside of tests.
* https://github.com/doctest/doctest/blob/master/doc/markdown/assertions.md#using-asserts-out-of-a-testing-context
*/

/**
* Assert that a given condition is false. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, runtime checks will be disabled.
* During tests, will print an error message if the assert fails.
* Also works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param cond: Condition to assert false
*/
#define check_false_message(cond, ...)\
if(std::is_constant_evaluated()) { \
	if(cond) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) { \
	if(doctest::getContextOptions() == nullptr) { \
		if(cond) { \
			std::cout << __FILE__ << ' ' << __LINE__ << '\n';\
			std::cout << "check false failed:\n	";\
			GK_DEBUG_PRINT(cond);\
			gk::internal::debugBreak(); \
		}\
	}\
	else{ \
		DOCTEST_CHECK_FALSE_MESSAGE(cond, __VA_ARGS__);\
	}\
}

/**
* Assert that two values are equal. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, this macro will do nothing at runtime.
* Works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
* 
* @param a == b
*/
#define check_eq(a, b) \
if(std::is_constant_evaluated()) { \
	if((a) != (b)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) {\
	if(doctest::getContextOptions() == nullptr) {\
		if ((a) != (b)) {\
			std::cout << __FILE__ << ' ' << __LINE__ << '\n';\
			std::cout << "check equal failed:\n	A: ";\
			GK_DEBUG_PRINT(a);\
			std::cout << "\n	B: ";\
			GK_DEBUG_PRINT(b);\
			std::cout << '\n';\
			gk::internal::debugBreak();\
		}\
	}\
	else {\
		DOCTEST_FAST_CHECK_EQ(a, b);\
	}\
}

/**
* Assert that two values are NOT equal. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, this macro will do nothing at runtime.
* Works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param a != b
*/
#define check_ne(a, b)\
if (std::is_constant_evaluated()) {\
		if ((a) == (b)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) {\
		if (doctest::getContextOptions() == nullptr) {\
				if ((a) == (b)) {\
						std::cout << __FILE__ << ' ' << __LINE__ << '\n'; \
						std::cout << "check not equal failed:\n	A: "; \
						GK_DEBUG_PRINT(a); \
						std::cout << "\n	B: "; \
						GK_DEBUG_PRINT(b); \
						std::cout << '\n'; \
						gk::internal::debugBreak();\
				}\
		}\
		else {\
				DOCTEST_FAST_CHECK_NE(a, b);\
		}\
}

/**
* Assert that `a` is greater than `b`. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, this macro will do nothing at runtime.
* Works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param a > b
*/
#define check_gt(a, b)\
if (std::is_constant_evaluated()) {\
		if ((a) <= (b)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) {\
		if (doctest::getContextOptions() == nullptr) {\
				if ((a) <= (b)) {\
						std::cout << __FILE__ << ' ' << __LINE__ << '\n'; \
						std::cout << "check greater than failed:\n	A: "; \
						GK_DEBUG_PRINT(a); \
						std::cout << "\n	B: "; \
						GK_DEBUG_PRINT(b); \
						std::cout << '\n'; \
						gk::internal::debugBreak();\
				}\
		}\
		else {\
				DOCTEST_FAST_CHECK_GT(a, b);\
		}\
}

/**
* Assert that `a` is less than `b`. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, this macro will do nothing at runtime.
* Works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param a < b
*/
#define check_lt(a, b)\
if (std::is_constant_evaluated()) {\
		if ((a) >= (b)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) {\
		if (doctest::getContextOptions() == nullptr) {\
				if ((a) >= (b)) {\
						std::cout << __FILE__ << ' ' << __LINE__ << '\n'; \
						std::cout << "check less than failed:\n	A: "; \
						GK_DEBUG_PRINT(a); \
						std::cout << "\n	B: "; \
						GK_DEBUG_PRINT(b); \
						std::cout << '\n'; \
						gk::internal::debugBreak();\
				}\
		}\
		else {\
				DOCTEST_FAST_CHECK_LT(a, b);\
		}\
}

/**
* Assert that `a` is greater than or equal to `b`. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, this macro will do nothing at runtime.
* Works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param a >= b
*/
#define check_ge(a, b)\
if (std::is_constant_evaluated()) {\
		if ((a) < (b)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) {\
		if (doctest::getContextOptions() == nullptr) {\
				if ((a) < (b)) {\
						std::cout << __FILE__ << ' ' << __LINE__ << '\n'; \
						std::cout << "check greater than or equal to failed:\n	A: "; \
						GK_DEBUG_PRINT(a); \
						std::cout << "\n	B: "; \
						GK_DEBUG_PRINT(b); \
						std::cout << '\n'; \
						gk::internal::debugBreak();\
				}\
		}\
		else {\
				DOCTEST_FAST_CHECK_GE(a, b);\
		}\
}

/**
* Assert that `a` is less than or equal to `b`. Works in both constexpr and runtime.
* If `gk::RUNTIME_ASSERTS_ON` is false, this macro will do nothing at runtime.
* Works if there is no doctest context. For example, any destructors running AFTER the destructor
* of the doctest need this ability. This can happen to global variables.
*
* @param a <= b
*/
#define check_le(a, b)\
if (std::is_constant_evaluated()) {\
		if ((a) > (b)) throw; \
}\
else if constexpr (gk::RUNTIME_ASSERTS_ON) {\
		if (doctest::getContextOptions() == nullptr) {\
				if ((a) > (b)) {\
						std::cout << __FILE__ << ' ' << __LINE__ << '\n'; \
						std::cout << "check less than or equal to failed:\n	A: "; \
						GK_DEBUG_PRINT(a); \
						std::cout << "\n	B: "; \
						GK_DEBUG_PRINT(b); \
						std::cout << '\n'; \
						gk::internal::debugBreak();\
				}\
		}\
		else {\
				DOCTEST_FAST_CHECK_LE(a, b);\
		}\
}