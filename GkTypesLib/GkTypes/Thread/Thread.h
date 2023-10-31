#pragma once

#include <thread>
#include <atomic>
#include <functional>
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

		static_assert(std::is_same<_Thrd_id_t, uint32>::value, "std::thread::id id integer type must be an unsigned int");
		static_assert(sizeof(std::thread::id) == 4, "std::thread:id must have a size of 4 bytes");

		/* Can use std::bind to bind a function with parameters. */
		typedef std::function<void()> ThreadFunctionType;

		/* Constructs a new std::thread object, forcing it to run on a continuous loop, waiting for a function to be bound and then subsequently executed.
		See BindFunction(). */
		[[nodiscard("Avoid creating a new thread without keeping track of it. Can cause memory leaks and consume system cpu resources.")]] 
		Thread()
		{
			pendingKill.store(false, std::memory_order_relaxed);
			shouldExecuteFunction.store(false, std::memory_order_relaxed);
			hasExecuted.store(true, std::memory_order_relaxed);
			thread = std::thread{ &Thread::ThreadLoop, this };
		}

		/* Sets the thread to stop looping, then joins it. */
		~Thread() {
			while (!IsReady());
			pendingKill = true;
			Execute();
			thread.join();
		}

		Thread(const Thread&) = delete;
		Thread& operator = (const Thread&) = delete;

		/* Bind a function to the thread for execution.
		@param inFunction: Uses type std::function<void()>. See std::bind() for binding functions with parameters. */
		void BindFunction(const ThreadFunctionType& inFunction)
		{
			if (hasExecuted.load(std::memory_order_relaxed) != false) {
				hasExecuted.store(false, std::memory_order_relaxed);
			}
			functions.Add(inFunction);
		}

		void BindFunction(ThreadFunctionType&& inFunction)
		{
			if (hasExecuted.load(std::memory_order_relaxed) != false) {
				hasExecuted.store(false, std::memory_order_relaxed);
			}
			functions.Add(std::move(inFunction));
		}

		/* Forcefully execute whatever bound function is contained, on the thread. */
		void Execute()
		{
			shouldExecuteFunction.store(true, std::memory_order_relaxed);
			hasExecuted.store(false, std::memory_order_relaxed);
			std::unique_lock<std::mutex> lock(mutex);
			condVar.notify_all();
		}

		/* Get the id of the thread contained. */
		[[nodiscard]] uint32 GetThreadId() const { 
			const std::thread::id id = thread.get_id();
			return *(uint32*)&id; 
		}

		/* Useful for checking against std::this_thread::get_id() */
		[[nodiscard]] std::thread::id StdThreadId() const noexcept {
			return thread.get_id();
		}

		/* Did this thread complete execution of the bound function? */
		[[nodiscard]] bool IsReady() const { return hasExecuted.load(std::memory_order_relaxed); }

	private:

		/* Loop that runs constantly on the thread. */
		void ThreadLoop() {
			while (!pendingKill.load()) {
				std::unique_lock<std::mutex> lck(mutex);
				condVar.wait(lck, [&]{return this->shouldExecuteFunction.load(); });
				ExecuteThreadFunctions();
			}
		}

		/* Executed the bound thread function. Sets the thread to no longer "want" to execute the function on the next loop iteration, and says that it did execute the bound function. */
		void ExecuteThreadFunctions()
		{
			shouldExecuteFunction.store(false, std::memory_order_relaxed);
			if (functions.Size() == 0) {
				hasExecuted.store(true, std::memory_order_relaxed);
				return;
			}

			darray<ThreadFunctionType> copy = std::move(functions);
			functions = darray<ThreadFunctionType>();
			for (uint32 i = 0; i < copy.Size(); i++) {
				copy[i]();
			}
			hasExecuted.store(true, std::memory_order_relaxed);
		}

	private:

		/* Mutex for condition variable. */
		std::mutex mutex;

		/* Handles blocking and resuming the threads execution. */
		std::condition_variable condVar;

		/* The actual thread that will have functions ran on. */
		std::thread thread;

		/* Functions to execute on this thread. See Execute(); */
		darray<ThreadFunctionType> functions;

		/* Flag that tracks whether the functions should execute. Tracked by the condition variable. */
		std::atomic<bool> shouldExecuteFunction;

		/* Whether this thread has completed execution of whatever tasks it did previously. */
		std::atomic<bool> hasExecuted;

		/* Informs that this thread will be destroyed, and stops the . Calls std::thread::join(). */
		std::atomic<bool> pendingKill;

	};


}

