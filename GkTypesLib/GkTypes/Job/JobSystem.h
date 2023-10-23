#pragma once

#include "../BasicTypes.h"
#include "../Event/Event.h"
#include "../Array/DynamicArray.h"
#include "JobInfo.h"
#include "JobThread.h"

namespace gk
{
	class JobSystem
	{
	public:

		/* Initializes the job system with the specified number of threads as a static globally accessible system in the program.
		See gk::systemThreadCount. For using as much as the system as possible, gk::systemThreadCount() - 1 is ideal. */
		static void init(const uint32 threadsNum) {
			gk_assertm(threadsNum >= 2, "Job system requires 2 or more threads");
			gk_assertm(jobThreads.arr == nullptr, "Cannot initialize job system when it has been already");
			jobThreads.arr = new JobThread[threadsNum];
			jobThreads.threadCount = threadsNum;
			std::cout << "[gk::JobSystem] Initialized with " << threadsNum << " job threads\n";
		}

		/* Finishes all jobs and de-initializes the job system, and frees all used resources. */
		static void deinit() {
			gk_assertm(jobThreads.arr != nullptr, "Cannot de-initialized job system when it hasn't been initialized");
			executeQueue();
			delete[] jobThreads.arr;
			jobThreads.arr = nullptr;
			jobThreads.threadCount = 0;
			std::cout << "[gk::JobSystem] Deinitialized\n";
		}

		/* Queues a job on the optimal thread WITHOUT executing. Job must be passed in by move. */
		static void queueJob(JobData&& job) {
			JobThread* thread = getOptimalThreadForExecution();
			thread->queueJob(std::move(job));
		}

		/* Queues jobs across optimal threads WITHOUT executing. Jobs must be passed in by move. */
		static void queueJobs(gk::darray<JobData>&& jobs) {
			queueJobs(jobs.Data(), jobs.Size());
		}

		/* Queues jobs across optimal threads WITHOUT executing. Will invalidate the contents of the array by moving them. */
		static void queueJobs(JobData* arrayStart, const uint32 count) {
			if (count == 0) return;
			gk::darray<JobThread*> threads = getOptimalThreadsForExecution();
			gk_assertm(threads.Size() > 0, "Must have at least one thread to queue jobs onto");

			const uint32 threadCount = threads.Size();
			const uint32 jobsPerThread = count / threadCount;
			const uint32 remainder = count % (jobsPerThread * threadCount);

			JobData* jobs = arrayStart;
			for (uint32 i = 0; i < threadCount; i++) {
				const bool isWithinRemainder = i < remainder;
				const uint32 numJobsForThread = isWithinRemainder ? jobsPerThread + 1 : jobsPerThread;
				gk_assertm((jobs + numJobsForThread) <= (arrayStart + count), "Attempted to access jobs outside of array bounds");

				JobThread* jobThread = threads[i];
				jobThread->queueJobs(jobs, numJobsForThread);
				jobs += numJobsForThread;
			}
		}

		/* Queues a job on the optimal thread and executes it. Job must be passed in by move. */
		static void runJob(JobData&& job) {
			JobThread* thread = getOptimalThreadForExecution();
			thread->queueJob(std::move(job));
			thread->execute();
		}

		/* Queues jobs on the optimal threads and executes them. Jobs must be passed in by move. */
		static void runJobs(gk::darray<JobData>&& jobs) {
			runJobs(jobs.Data(), jobs.Size());
		}

		/* Queues jobs across optimal threads and executes them. Will invalidate the contents of the array by moving them. */
		static void runJobs(JobData* arrayStart, const uint32 count) {
			if (count == 0) return;
			gk::darray<JobThread*> threads = getOptimalThreadsForExecution();
			gk_assertm(threads.Size() > 0, "Must have at least one thread to queue jobs onto");

			const uint32 threadCount = threads.Size();
			const uint32 jobsPerThread = count / threadCount;
			const uint32 remainder = count % (jobsPerThread * threadCount);

			JobData* jobs = arrayStart;
			for (uint32 i = 0; i < threadCount; i++) {
				const bool isWithinRemainder = i < remainder;
				const uint32 numJobsForThread = isWithinRemainder ? jobsPerThread + 1 : jobsPerThread;
				gk_assertm((jobs + numJobsForThread) <= (arrayStart + count), "Attempted to access jobs outside of array bounds");

				JobThread* jobThread = threads[i];
				jobThread->queueJobs(jobs, numJobsForThread);
				jobThread->execute();
				jobs += numJobsForThread;
			}
		}

		/* Forces execution of any job threads that have jobs in their queues. */
		static void executeQueue() {
			for (JobThread* jobThread : jobThreads) {
				if (jobThread->queuedJobsCount() > 0 && !jobThread->isExecuting()) {
					jobThread->execute();
				}
			}
		}

		/* Simply waits for the job system to finish executing. */
		static void wait() {
			std::this_thread::yield();
			for (JobThread* jobThread : jobThreads) {
				jobThread->wait();
			}
		}

	private:

		static JobThread* getOptimalThreadForExecution() {
			JobThread* optimal = nullptr;
			uint32 minimumQueueLoad = MAXUINT32;
			bool isOptimalExecuting = true;

			for (JobThread* jobThread : jobThreads) {
				const bool isNotExecuting = !jobThread->isExecuting();
				const uint32 queueLoad = jobThread->queuedJobsCount();
				if (isNotExecuting && queueLoad == 0) {
					return jobThread;
				}
				// It can be assumed that from here, either the thread is busy or it has a non-zero queue load
				if (isNotExecuting) {
					if (minimumQueueLoad > queueLoad) {
						optimal = jobThread;
						minimumQueueLoad = queueLoad;
						isOptimalExecuting = false;
						continue;
					}
				}
				if (minimumQueueLoad > queueLoad && isOptimalExecuting) { // prioritize jobs that are not executing currently
					optimal = jobThread;
					minimumQueueLoad = queueLoad;
				}
			}
			return optimal;
		}

		/* Returns all threads that aren't currently executing, or if there are none, all of them. */
		static gk::darray<JobThread*> getOptimalThreadsForExecution() {
			gk::darray<JobThread*> optimal;
			optimal.Reserve(jobThreads.threadCount);

			for (JobThread* jobThread : jobThreads) {
				if (!jobThread->isExecuting()) {
					optimal.Add(jobThread);
				}
			}
			if (optimal.Size() == 0) {
				for (JobThread* jobThread : jobThreads) {
					optimal.Add(jobThread);
				}
			}
			return optimal;
		}

	private:

		inline static JobThreadArray jobThreads;

	};
}
