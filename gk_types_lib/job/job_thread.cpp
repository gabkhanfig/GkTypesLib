#include "job_thread.h"

#if GK_TYPES_LIB_TEST

namespace gk
{
	namespace unitTests
	{
		static int freeFunctionReturnIntNoArgs() {
			return 1234;
		}

		static void freeFunctionNoReturnSleep(int sleepMs) {
			Sleep(sleepMs);
		}

		static u64 freeFunctionReturnOnHeap(u64 a, u64 b, u64 c, u64 d, u64 e) {
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
	}
}

using gk::unitTests::freeFunctionReturnIntNoArgs;
using gk::unitTests::freeFunctionNoReturnSleep;
using gk::unitTests::freeFunctionReturnOnHeap;
using gk::unitTests::incrementMutex;
using gk::unitTests::addNestedJob;

using JobThread = gk::JobThread;

template<typename ReturnT>
using JobFuture = gk::JobFuture<ReturnT>;

template<typename T>
using Mutex = gk::Mutex<T>;

test_case("CreateJobThread") {
	JobThread* jobThread = new JobThread();
	delete jobThread;
}

test_case("RunJobFreeFunctionReturnIntNoArgs") {
	JobThread* jobThread = new JobThread();
	JobFuture<int> future = jobThread->runJob(freeFunctionReturnIntNoArgs);
	const int num = future.wait();
	check_eq(num, 1234);
	delete jobThread;
}

test_case("RunJobFreeFunctionNoReturn1Arg") {
	JobThread* jobThread = new JobThread();
	JobFuture future = jobThread->runJob(freeFunctionNoReturnSleep, 1);
	future.wait();
	delete jobThread;
}

test_case("RunJobFreeFunctionOnHeap") {
	JobThread* jobThread = new JobThread();
	JobFuture future = jobThread->runJob(freeFunctionReturnOnHeap, 1ull, 2ull, 3ull, 4ull, 5ull);
	const gk::u64 num = future.wait();
	check_eq(num, 15);
	delete jobThread;
}

test_case("ThreadSafeJobRun") {
	JobThread* jobThread = new JobThread();
	Mutex<int> mutex = 0;
	JobFuture nestedFuture = jobThread->runJob(addNestedJob, (JobThread*)jobThread, &mutex);
	for (int i = 0; i < 100; i++) {
		jobThread->runJob(incrementMutex, &mutex);
	}
	nestedFuture.wait();
	jobThread->wait();
	const int mutexNum = *mutex.getDataNoLock();
	check_eq(mutexNum, 200);
}

#endif