#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Thread/ThreadPool.h"

static void DoSomeWork() {
	int* number = new int;
	*number = 10;
	delete number;
}

void AddOneAfterFixedDelay(int* num, long long msdelay) {
	auto begin = std::chrono::steady_clock::now(); 
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	while (std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() < msdelay) {
		end = std::chrono::steady_clock::now();
	}
	*num = *num + 1;
}

namespace UnitTests {

	TEST(ThreadPool, HardwareThreadCount) {
		const unsigned int systemThreadCount = gk::ThreadPool::SystemThreadCount();
		EXPECT_TRUE(systemThreadCount > 1);
		EXPECT_EQ(systemThreadCount, std::thread::hardware_concurrency());
	}

	TEST(ThreadPool, CreateAndDelete) {
		gk::ThreadPool* pool = new gk::ThreadPool(4);
		delete pool;
	}

	TEST(ThreadPool, CreateExecuteAndDelete) {
		gk::ThreadPool* pool = new gk::ThreadPool(4);
		pool->AddFunctionToQueue(DoSomeWork);
		pool->ExecuteQueue();
		delete pool;
	}

	TEST(ThreadPool, ParallelTasksFasterThanConsecutive) {
		const int msDelay = 10;
		int* num1 = new int;
		int* num2 = new int;
		int* num3 = new int;
		int* num4 = new int;
		gk::ThreadPool* pool = new gk::ThreadPool(3);
		pool->AddFunctionToQueue(std::bind(AddOneAfterFixedDelay, num1, msDelay));
		pool->AddFunctionToQueue(std::bind(AddOneAfterFixedDelay, num2, msDelay));
		pool->AddFunctionToQueue(std::bind(AddOneAfterFixedDelay, num3, msDelay));
		pool->AddFunctionToQueue(std::bind(AddOneAfterFixedDelay, num4, msDelay));
		auto begin = std::chrono::steady_clock::now();
		pool->ExecuteQueue();
		auto end = std::chrono::steady_clock::now();
		long long msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
		EXPECT_TRUE(msElapsed < (4 * msDelay)) << "This test must take less than " << 4 * msDelay << "ms to execute across 4 threads";
		delete num1;
		delete num2;
		delete num3;
		delete num4;
		delete pool;
	}

}