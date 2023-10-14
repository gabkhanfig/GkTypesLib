#pragma once
#include "../GkTypesLib/GkTypes/BasicTypes.h"
#include <Windows.h>

/* Example of usage:
COMPTIME_TEST(Number, IsEven, {
	int num = 2;
	return (num % 2) == 0; }); 
*/
#define COMPTIME_TEST(test_case_name, test_name, test_block) \
consteval bool CompTimeTest_ ## test_case_name ## _ ## test_name() ## test_block \
static_assert(CompTimeTest_ ## test_case_name ## _ ## test_name ## ());

/* https://stackoverflow.com/questions/29174938/googletest-and-memory-leaks
Only works in debug mode (_DEBUG macro).
On destruction, or by executing CheckLeak(), the memory state will be checked for any leaks. */
class MemoryLeakDetector {

public:

	MemoryLeakDetector();

	void CheckLeak();

	~MemoryLeakDetector();

private:

	void reportFailure(uint64 unfreedBytes);
	_CrtMemState memState_;

};

