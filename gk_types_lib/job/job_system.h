#pragma once

#include "../basic_types.h"
#include "job_thread.h"

namespace gk
{
	struct JobSystem
	{
	public:

		JobSystem(u32 inThreadCount)
			: _threadCount(inThreadCount), _currentOptimalThread(0)
		{
			//gk_assertm(inThreadCount >= 2, "JobSystem thread count must be greater than or equal to 2. Tried to do with " << inThreadCount);
			_threads = new JobThread[inThreadCount];
		}

		JobSystem(const JobSystem&) = delete;
		JobSystem(JobSystem&&) = delete;
		JobSystem& operator = (const JobSystem&) = delete;
		JobSystem& operator = (JobSystem&&) = delete;

		~JobSystem() {
			wait();
			delete[] _threads;
		}

		/* Member function. */
		template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
		JobFuture<ReturnT> runJob(ReturnT(FuncClassT::* inFunc)(Args...), ObjT* inObject, Args&&... inArgs) {
			JobThread* jobThread = getOptimalThreadForExecution();
			return jobThread->runJob(inFunc, inObject, std::forward<Args>(inArgs)...);
		}

		/* Const member function. */
		template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
		JobFuture<ReturnT> runJob(ReturnT(FuncClassT::* inFunc)(Args...) const, const ObjT* inObject, Args&&... inArgs) {
			JobThread* jobThread = getOptimalThreadForExecution();
			return jobThread->runJob(inFunc, inObject, std::forward<Args>(inArgs)...);
		}

		/* Free function. */
		template<typename ReturnT, typename... Args>
		JobFuture<ReturnT> runJob(ReturnT(*inFunc)(Args...), Args&&... inArgs) {
			JobThread* jobThread = getOptimalThreadForExecution();
			return jobThread->runJob(inFunc, std::forward<Args>(inArgs)...);
		}

		/* Compatibility with std::function, std::bind, etc. */
		template<typename ReturnT>
		JobFuture<ReturnT> runJob(std::function<ReturnT()>&& inFunc) {
			JobThread* jobThread = getOptimalThreadForExecution();
			return jobThread->runJob(std::move(inFunc));
		}

		void wait() const {
			std::this_thread::yield();
			for (u32 i = 0; i < _threadCount; i++) {
				_threads[i].wait();
			}
		}

	private:

		/* Will atomically change the _currentOptimalThread member to be the one
		after the selected optimal thread. */
		JobThread* getOptimalThreadForExecution() {
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

	private:

		JobThread* _threads;
		// naturally cannot be modified. In order to change thread counts, the old object MUST be destroyed and replaced.
		const u32 _threadCount;
		std::atomic<u32> _currentOptimalThread;
	};

} // namespace gk
