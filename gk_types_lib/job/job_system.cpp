#include "job_system.h"

gk::JobSystem::JobSystem(u32 inThreadCount)
	: _threadCount(inThreadCount), _currentOptimalThread(0)
{
	//gk_assertm(inThreadCount >= 2, "JobSystem thread count must be greater than or equal to 2. Tried to do with " << inThreadCount);
	_threads = new JobThread[inThreadCount];
}

gk::JobSystem::~JobSystem()
{
	wait();
	delete[] _threads;
}

void gk::JobSystem::wait() const
{
	std::this_thread::yield();
	for (u32 i = 0; i < _threadCount; i++) {
		_threads[i].wait();
	}
}

gk::JobThread* gk::JobSystem::getOptimalThreadForExecution()
{
	const u32 oldCurrentOptimal = _currentOptimalThread.load(std::memory_order::acquire);

	u32 minimumQueueLoad = 0xffffffff;
	bool isOptimalExecuting = true;
	u32 currentOptimal = oldCurrentOptimal;

	for (u32 i = 0; i < _threadCount; i++) {
		const u32 checkIndex = (oldCurrentOptimal + i) % _threadCount;
		const bool isNotExecuting = !_threads[checkIndex].isExecuting();
		const u32 queueLoad = _threads[checkIndex].queuedJobCount();

		if (isNotExecuting && queueLoad == 0) { // thread is idle and should execute
			_currentOptimalThread.store((checkIndex + 1) % _threadCount);
			return &_threads[checkIndex];
		}
		// if thread has stuff in queue, it should be already executing.
		if (minimumQueueLoad > queueLoad) {
			currentOptimal = checkIndex;
			minimumQueueLoad = queueLoad;
		}
	}
	// next iteration, start checking on the thread after this one.
	_currentOptimalThread.store((currentOptimal + 1) % _threadCount);
	return &_threads[currentOptimal];
}

#if GK_TYPES_LIB_TEST

using JobSystem = gk::JobSystem;

test_case("ConstructDestruct") {
	JobSystem* jobSystem = new JobSystem(2);
	delete jobSystem;
}

#endif


