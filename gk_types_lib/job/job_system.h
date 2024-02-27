#pragma once

#include "../basic_types.h"
#include "job_thread.h"

namespace gk
{
	struct JobSystem
	{
	public:

		JobSystem(u32 inThreadCount);

		JobSystem(const JobSystem&) = delete;
		JobSystem(JobSystem&&) = delete;
		JobSystem& operator = (const JobSystem&) = delete;
		JobSystem& operator = (JobSystem&&) = delete;

		~JobSystem();

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

		void wait() const;

	private:

		/* Will atomically change the _currentOptimalThread member to be the one
		after the selected optimal thread. */
		JobThread* getOptimalThreadForExecution();

	private:

		JobThread* _threads;
		// naturally cannot be modified. In order to change thread counts, the old object MUST be destroyed and replaced.
		const u32 _threadCount;
		std::atomic<u32> _currentOptimalThread;
	};

} // namespace gk
