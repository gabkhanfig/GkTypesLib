#include "gtest/gtest.h"
#include "GkTest.h"
#include <crtdbg.h>

#include "windows.h"
#include "../GkTypesLib/GkTypes/Job/JobSystem.h"

MemoryLeakDetector::MemoryLeakDetector()
{
	_CrtMemCheckpoint(&memState_);
}

void MemoryLeakDetector::CheckLeak()
{
  _CrtMemState stateNow, stateDiff;
  _CrtMemCheckpoint(&stateNow);
  int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);
  if (diffResult) {
    reportFailure(stateDiff.lSizes[1]);
  }
  else {
    std::cerr << "No memory leak detected!\n";
  }
}

MemoryLeakDetector::~MemoryLeakDetector()
{
  CheckLeak();
}

void MemoryLeakDetector::reportFailure(uint64 unfreedBytes)
{
  FAIL() << "Memory leak of " << unfreedBytes << " byte(s) detected.";
}

void runGkTests(int argc, char** argv)
{
  std::cout << "\nValidating gk::JobSystem intialization and deinitialization\n";
  gk::JobSystem::init(2);
  gk::JobSystem::deinit();
  gk::JobSystem::init(4); // JobSystem will be initialized to have 4 threads for all JobSystem tests
  
  ::testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
}
