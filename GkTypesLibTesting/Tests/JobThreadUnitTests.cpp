#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/Job/JobThread.h"
#include "../GkTest.h"

using JobThread = gk::JobThread;

template<typename ReturnT>
using JobFuture = gk::JobFuture<ReturnT>;

template<typename T>
using Mutex = gk::Mutex<T>;

int freeFunctionReturnIntNoArgs() {
	return 1234;
}

void freeFunctionNoReturnSleep(int sleepMs) {
	Sleep(sleepMs);
}

uint64 freeFunctionReturnOnHeap(uint64 a, uint64 b, uint64 c, uint64 d, uint64 e) {
	return a + b + c + d + e;
}

static void incrementMutex(Mutex<int>* mutex) {
	auto lock = mutex->lock();
	(*lock.get())++;
}

static void addNestedJob(JobThread* jobThread, Mutex<int>* mutex) {
	for (int i = 0; i < 100; i++) {
		jobThread->runJob(incrementMutex, (Mutex<int>*)mutex);
	}
}

namespace UnitTests
{
	TEST(JobThread, CreateJobThread) {
		JobThread* jobThread = new JobThread();
		delete jobThread;
	}

	TEST(JobThread, RunJobFreeFunctionReturnIntNoArgs) {
		JobThread* jobThread = new JobThread();
		JobFuture<int> future = jobThread->runJob(freeFunctionReturnIntNoArgs);
		const int num = future.wait();
		EXPECT_EQ(num, 1234);
		delete jobThread;
	}

	TEST(JobThread, RunJobFreeFunctionNoReturn1Arg) {
		JobThread* jobThread = new JobThread();
		JobFuture future = jobThread->runJob(freeFunctionNoReturnSleep, 1);
		future.wait();
		delete jobThread;
	}

	TEST(JobThread, RunJobFreeFunctionOnHeap) {
		JobThread* jobThread = new JobThread();
		JobFuture future = jobThread->runJob(freeFunctionReturnOnHeap, 1ull, 2ull, 3ull, 4ull, 5ull);
		const uint64 num = future.wait();
		EXPECT_EQ(num, 15);
		delete jobThread;
	}

	TEST(JobThread, ThreadSafeJobRun) {
		JobThread* jobThread = new JobThread();
		Mutex<int> mutex = 0;
		JobFuture nestedFuture = jobThread->runJob(addNestedJob, (JobThread*)jobThread, &mutex);
		for (int i = 0; i < 100; i++) {
			jobThread->runJob(incrementMutex, &mutex);
		}
		nestedFuture.wait();
		jobThread->wait();
		const int mutexNum = *mutex.getDataNoLock();
		EXPECT_EQ(mutexNum, 200);
	}

}