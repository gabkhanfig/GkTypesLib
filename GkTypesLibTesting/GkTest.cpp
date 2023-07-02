#include "gtest/gtest.h"
#include "GkTest.h"
#include <crtdbg.h>

#include "windows.h"

MemoryLeakDetector::MemoryLeakDetector()
{
	_CrtMemCheckpoint(&memState_);
}

void MemoryLeakDetector::CheckLeak()
{
  _CrtMemState stateNow, stateDiff;
  _CrtMemCheckpoint(&stateNow);
  int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);
  if (diffResult)
    reportFailure(stateDiff.lSizes[1]);
}

MemoryLeakDetector::~MemoryLeakDetector()
{
  CheckLeak();
}

void MemoryLeakDetector::reportFailure(uint64 unfreedBytes)
{
  FAIL() << "Memory leak of " << unfreedBytes << " byte(s) detected.";
}
