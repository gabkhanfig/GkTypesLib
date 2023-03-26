#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <Windows.h>
#include "../Array/DynamicArray.h"
#include <condition_variable>
#include <mutex>

namespace gk 
{
	/* Wrapper class to handle creating and keeping a persistent thread.
Use the BindFunction function to set this thread to execute another function.
Only executes a function one time after binding. */
	class Thread
	{
	public:

		/* Can use std::bind to bind a function with parameters. */
		typedef std::function<void()> ThreadFunctionType;

	private:

		/* The actual thread that will have functions ran on. */
		std::thread thread;

		/* Mutex for condition variable. */
		std::mutex mutex;

		/* Handles blocking and resuming the threads execution for timing under 1ms. */
		std::condition_variable condVar;

		/* Functions to execute on this thread. See Execute(); */
		darray<ThreadFunctionType> functions;
		
		/* Flag that tracks whether the functions should execute. Tracked by the condition variable. */
		std::atomic<bool> shouldExecuteFunction;

		/* Whether this thread has completed execution of whatever tasks it did previously. */
		std::atomic<bool> hasExecuted;

		/* Informs that this thread will be destroyed, and stops the . Calls std::thread::join(). */
		std::atomic<bool> pendingKill;

	public:

		/* Constructs a new std::thread object, forcing it to run on a continuous loop, waiting for a function to be bound and then subsequently executed.
		See BindFunction(). */
		[[nodiscard("Avoid creating a new thread without keeping track of it. Can cause memory leaks and consume system cpu resources.")]] 
		Thread()
		{
			pendingKill = false;
			shouldExecuteFunction = false;
			hasExecuted = true;
			thread = std::thread{ &Thread::ThreadLoop, this };
		}

		/* Sets the thread to stop looping, then joins it. */
		~Thread() {
			pendingKill = true;
			Execute();
			thread.join();
		}

		Thread(const Thread&) = delete;
		Thread& operator = (const Thread&) = delete;

		/* Bind a function to the thread for execution.
		@param inFunction: Uses type std::function<void()>. See std::bind() for binding functions with parameters.
		@param shouldExecute: Whether this thread will execute this function automatically. If not, call Execute(). */
		void BindFunction(ThreadFunctionType inFunction)
		{
			hasExecuted = false;
			functions.Add(inFunction);
		}

		/* Forcefully execute whatever bound function is contained, on the thread. */
		void Execute()
		{
			shouldExecuteFunction = true;
			hasExecuted = false; 
			condVar.notify_one();
		}

		/* Get the id of the thread contained. */
		[[nodiscard]] std::thread::id GetThreadId() const { return thread.get_id(); }

		/* Did this thread complete execution of the bound function? */
		[[nodiscard]] bool IsReady() const { return hasExecuted; }

	private:

		/* Loop that runs constantly on the thread. */
		void ThreadLoop() {
			while (!pendingKill) {
				std::unique_lock<std::mutex> lck(mutex);
				condVar.wait(lck, std::bind(&Thread::ShouldExecute, this));
				ExecuteThreadFunctions();
				std::this_thread::yield;
			}
		}

		/**/
		bool ShouldExecute() const {
			return shouldExecuteFunction;
		}

		/* Executed the bound thread function. Sets the thread to no longer "want" to execute the function on the next loop iteration, and says that it did execute the bound function. */
		void ExecuteThreadFunctions()
		{
			shouldExecuteFunction = false;
			if (functions.Size() == 0) {
				hasExecuted = true;
				return;
			}

			darray<ThreadFunctionType> copy = functions;
			functions.Empty();
			for (int i = 0; i < copy.Size(); i++) {
				copy[i]();
			}
			hasExecuted = true;
		}

	};


}

