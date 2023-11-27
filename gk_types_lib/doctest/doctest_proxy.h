// use this instead of doctest.h
#pragma once

#include <iostream>

namespace gk
{
	namespace internal
	{
		void debugBreak();
	}
}

#define comptimeAssert(condition) if((condition) == false) throw;
#define comptimeAssertNot(condition) if((condition) == true) throw;
#define comptimeAssertEq(v1, v2) if(v1 != v2) throw;
#define comptimeAssertNe(v1, v2) if(v1 == v2) throw;
#define comptimeAssertGt(v1, v2) if(v1 <= v2) throw;
#define comptimeAssertLt(v1, v2) if(v1 >= v2) throw;
#define comptimeAssertGe(v1, v2) if(v1 < v2) throw;
#define comptimeAssertLe(v1, v2) if(v1 > v2) throw;

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

#define check_eq DOCTEST_FAST_CHECK_EQ
#define check_ne DOCTEST_FAST_CHECK_NE
#define check_gt DOCTEST_FAST_CHECK_GT
#define check_lt DOCTEST_FAST_CHECK_LT
#define check_ge DOCTEST_FAST_CHECK_GE
#define check_le DOCTEST_FAST_CHECK_LE

#else
namespace gk {
	static constexpr bool RUNTIME_ASSERTS_ON = false;
}

#define test_case
#define subcase
#define test_suite

#define comptime_test_case

#define check_eq
#define check_ne
#define check_gt
#define check_lt
#define check_ge
#define check_le

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
	/*std::cout << "runtime assert\n";*/ \
	if(doctest::getContextOptions() == nullptr) { \
		if(!(cond)) { \
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
	/*std::cout << "runtime assert\n";*/ \
	if(doctest::getContextOptions() == nullptr) { \
		if(cond) { \
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
	/*std::cout << "runtime assert\n";*/ \
	if(doctest::getContextOptions() == nullptr) { \
		if(cond) { \
			gk::internal::debugBreak(); \
		}\
	}\
	else{ \
		DOCTEST_CHECK_FALSE_MESSAGE(cond, __VA_ARGS__);\
	}\
}

