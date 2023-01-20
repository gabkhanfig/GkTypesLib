#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <Windows.h>

#define sleep(ms) Sleep(ms)
#define _SHOULD_THREAD_SLEEP_LOOP true
#define _THREAD_WRAPPER_LOOP_SLEEP_MILLIS 1

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

		std::thread thread;

		ThreadFunctionType function;

		std::atomic<bool> shouldExecuteFunction;

		std::atomic<bool> hasExecuted;

		std::atomic<bool> pendingKill;

	private:

		/* Loop that runs constantly on the thread. */
		void TickLoop() {
			while (true) {
				if (pendingKill) {
					return;
				}

				if (shouldExecuteFunction) {
					ExecuteThreadFunction();
				}
#if _SHOULD_THREAD_SLEEP_LOOP == true
				sleep(_THREAD_WRAPPER_LOOP_SLEEP_MILLIS);
#endif
			}
		}

		/* Executed the bound thread function. Sets the thread to no longer "want" to execute the function on the next loop iteration, and says that it did execute the bound function. */
		void ExecuteThreadFunction()
		{
			function();
			shouldExecuteFunction = false;
			hasExecuted = true;
		}

	public:

		/* Constructs a new std::thread object, forcing it to run on a continuous loop, waiting for a function to be bound and then subsequently executed.
		See BindFunction(). */
		[[nodiscard("Avoid creating a new thread without keeping track of it. Can cause memory leaks and consume system cpu resources.")]] 
		Thread()
		{
			thread = std::thread{ &Thread::TickLoop, this };
			pendingKill = false;
			shouldExecuteFunction = false;
			hasExecuted = true;
		}

		/* Sets the thread to stop looping, then joins it. */
		~Thread() {
			pendingKill = true;
			thread.join();
		}

		Thread(const Thread&) = delete;
		Thread& operator = (const Thread&) = delete;

		/* Bind a function to the thread for execution.
		@param inFunction: Uses type std::function<void()>. See std::bind() for binding functions with parameters.
		@param shouldExecute: Whether this thread will execute this function automatically. If not, call Execute(). */
		void BindFunction(ThreadFunctionType inFunction, bool shouldExecute = true)
		{
			shouldExecuteFunction = shouldExecute;
			hasExecuted = false;
			function = inFunction;
		}

		/* Forcefully execute whatever bound function is contained, on the thread. */
		void Execute()
		{
			shouldExecuteFunction = true;
			hasExecuted = false;
		}

		/* Get the id of the thread contained. */
		[[nodiscard]] std::thread::id GetThreadId() const { return thread.get_id(); }

		/* Did this thread complete execution of the bound function? */
		[[nodiscard]] bool DidCompleteExecution() const { return hasExecuted; }
	};
}

