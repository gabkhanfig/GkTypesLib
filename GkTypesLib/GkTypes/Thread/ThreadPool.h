#pragma once

#include "Thread.h"
#include <queue>
#include "../Array/DynamicArray.h"
#include <iostream>

#define _HARDWARE_LOWEST_ALLOWED_THREAD_COUNT 1

namespace gk
{
	class ThreadPool
	{
	private:

		gk::darray<gk::Thread::ThreadFunctionType> functionQueue;

		gk::darray<gk::Thread*> threads;

	public:

		/* Get the amount of threads available on this system. Returns a minimum value of 1. */
		[[nodiscard]] static const unsigned int SystemThreadCount() {
			unsigned int hardwareCount = std::thread::hardware_concurrency();
			return hardwareCount > _HARDWARE_LOWEST_ALLOWED_THREAD_COUNT ? hardwareCount : _HARDWARE_LOWEST_ALLOWED_THREAD_COUNT;
		}

		/**/
		[[nodiscard("Avoid creating a thread pool without keeping track of it. Can cause memory leaks and consume system cpu resources.")]] 
		ThreadPool(unsigned int _threadCount)
		{
			threads.Reserve(_threadCount);
			for (uint32 i = 0; i < _threadCount; i++) {
				gk::Thread* thread = new gk::Thread();
				threads.Add(thread);
			}
		}

		/* Delete and join all threads. */
		~ThreadPool()
		{
			unsigned int threadCount = GetThreadCount();
			for (uint32 i = 0; i < threadCount; i++) {
				delete threads[i];
			}
		}

		/* Get the amount of threads this threadpool is using. */
		[[nodiscard]] unsigned int GetThreadCount() const { return threads.Size(); }

		/* Bind a function to the queue for eventual execution when ExecuteQueue() is called.
		@param function: Uses type std::function<void()>. See std::bind() for binding functions with parameters. */
		size_t AddFunctionToQueue(const gk::Thread::ThreadFunctionType& function)
		{
			functionQueue.Add(function);
			return functionQueue.Size();
		}

		/**/
		bool AllThreadsReady() const {
			bool AllThreadsDone = true;
			for (uint32 i = 0; i < GetThreadCount(); i++) {
				gk::Thread* thread = threads[i];
				if (!thread->IsReady()) {
					AllThreadsDone = false;
				}
			}
			return AllThreadsDone;
		}

		/* Executes everything in the pool queue until the pool is empty and all thread have completed. Also uses the calling thread as a worker. 
		@param waitUntilAllComplete: If true, waits to return until every thread is complete, otherwise returns when primary thread only is known to be complete. */
		void ExecuteQueue(bool waitUntilAllComplete = true) {
			const uint32 totalFunctions = functionQueue.Size();
			if (totalFunctions == 0) return;

			const uint32 availableExecuteThreads = GetThreadCount() + 1;
			const uint32 maxFunctionsPerThread = static_cast<uint32>(std::ceil(double(totalFunctions) / double(availableExecuteThreads)));

			uint32 functionIndex = 0;
			uint32 threadIndex = 0;
			uint32 functionsOnThisThread = 0;

			while (!AllThreadsReady());

			while (functionIndex < totalFunctions) {
				if (threadIndex == threads.Size()) { // No more gk::Threads? Use this thread.
					gk::Thread::ThreadFunctionType& function = functionQueue[functionIndex];
					function(); 
					functionIndex++;
					continue;
				}

				gk::Thread* thread = threads[threadIndex];
				thread->BindFunction(std::move(functionQueue[functionIndex]));

				functionIndex++;
				functionsOnThisThread++;
				if (functionsOnThisThread == maxFunctionsPerThread) {
					functionsOnThisThread = 0;
					threadIndex++;
					thread->Execute();
				}
			}

			functionQueue.Empty();

			if (!waitUntilAllComplete) return;
			while (!AllThreadsReady());

			/*
			// Check if all the threads are ready. If not, wait.
			while (!AllThreadsReady());

			//darray<darray<Thread::ThreadFunctionType>> functions;
			//for (uint32 i = 0; i < GetThreadCount() + 1; i++) {
			//	functions.Add(darray<Thread::ThreadFunctionType>());
			//}

			while (functionQueue.Size()) {
				for (uint32 i = 0; i < GetThreadCount() + 1; i++) {
					if (functionQueue.Size() == 0) break;
					functions[i].Add(functionQueue.front());
					functionQueue.pop();
				}
			}

			for (uint32 i = 0; i < GetThreadCount(); i++) {
				gk::Thread* thread = threads[i];
				darray<Thread::ThreadFunctionType>& arr = functions[i];
				for (uint32 func = 0; func < arr.Size(); func++) {
					thread->BindFunction(functions[i].At(func));
				}
				thread->Execute();
			}
			darray<Thread::ThreadFunctionType>& last = functions[functions.Size() - 1];
			for (uint32 i = 0; i < last.Size(); i++) {
				last[i]();
			}

			if (!waitUntilAllComplete) return;
			while (!AllThreadsReady());*/
		}

	};
}

#undef _HARDWARE_LOWEST_ALLOWED_THREAD_COUNT