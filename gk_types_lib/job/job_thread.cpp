#include "job_thread.h"

gk::JobThread::JobThread()
{
	_thread = std::thread{ &JobThread::threadLoop, this };
	_isPendingKill = false;
	_shouldExecute = false;
	_isExecuting = false;
	_queuedJobCount = 0;
}

gk::JobThread::~JobThread()
{
	wait();
	_isPendingKill = true;
	notifyExecute();
	_thread.join();
}

void gk::JobThread::wait() const
{
	std::this_thread::yield();
	while (_isExecuting.load(std::memory_order::acquire) == true) {
		std::this_thread::yield();
	}
}

bool gk::JobThread::isExecuting() const
{
	return _isExecuting.load(std::memory_order::acquire);
}

gk::u32 gk::JobThread::queuedJobCount() const
{
	return _queuedJobCount.load(std::memory_order::acquire);
}

std::thread::id gk::JobThread::getThreadId() const
{
	return this->_thread.get_id();
}

void gk::JobThread::queueJob(JobContainer&& job)
{
	{
		auto queueLock = _queue.lock();
		queueLock.get()->push(std::move(job));
		_queuedJobCount++;
	}
	notifyExecute();
}

void gk::JobThread::notifyExecute()
{
	if (_isExecuting.load(std::memory_order::acquire) == true) {
		// should already be looping the execution, in which if it has any queued jobs, it will execute them.
		return;
	}
	_shouldExecute.store(true, std::memory_order::release);
	std::scoped_lock lock(_mutex);
	_condVar.notify_one();
	_isExecuting.store(true, std::memory_order::release);
}

void gk::JobThread::threadLoop()
{
	while (_isPendingKill.load(std::memory_order::acquire) == false) {
		{
			size_t count;
			{
				count = _queue.lock().get()->len();
			}
			if (count > 0) { // go back and run the jobs
				executeQueuedJobs();
				continue;
			}
		}
		_isExecuting.store(false, std::memory_order::release);
		{ // Wait for shouldExecute, in scope.
			std::unique_lock lck(_mutex);
			_condVar.wait(lck, [&] {return _shouldExecute.load(); });
			_shouldExecute = false;
		}
		executeQueuedJobs();
	}
}

void gk::JobThread::executeQueuedJobs()
{
	auto activeLock = _activeWork.lock();
	{
		auto queueLock = _queue.lock();
		_queuedJobCount.store(0, std::memory_order::release);
		activeLock.get()->collectJobs(queueLock.get());
		// queue lock is unlocked here.
	}
	activeLock.get()->invokeAllJobs();
}

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