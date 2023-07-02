#pragma once
#include "../GkTypesLib/GkTypes/BasicTypes.h"

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

