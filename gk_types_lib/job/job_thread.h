#pragma once

#include "../basic_types.h"
#include "job_container.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include "../sync/mutex.h"

namespace gk
{
	/* About 1.5 megabytes per instance. */
	struct JobThread
	{
		static constexpr u32 QUEUE_CAPACITY = 8192;

	private:

		template<typename ReturnT>
		using WithinJobFuture = gk::internal::WithinJobFuture<ReturnT>;

		using JobContainer = gk::internal::JobContainer;

		struct ActiveJobs;

		struct JobRingQueue {

			friend struct ActiveJobs;

			JobRingQueue() : _len(0), _readIndex(0), _writeIndex(0) {}

			[[nodiscard]] bool isFull() const {
				return _len == QUEUE_CAPACITY;
			}

			[[nodiscard]] bool isEmpty() const {
				return _len == 0;
			}

			[[nodiscard]] size_t len() const {
				return _len;
			}

			void push(JobContainer&& element) {
				check_message(!isFull(), "Job ring queue is full");
				_buffer[_writeIndex] = std::move(element);
				_writeIndex = (_writeIndex + 1) % QUEUE_CAPACITY;
				_len++;
			}

		private:
			// JobData is 64 byte aligned regardless so it doesn't make sense to use u32, when there will already be padding
			size_t _len;
			size_t _readIndex;
			size_t _writeIndex;
			JobContainer _buffer[QUEUE_CAPACITY];
		};

		struct ActiveJobs {

			ActiveJobs() :_count(0) {}

			void collectJobs(JobRingQueue* queue) {
				size_t count = 0;
				size_t moveIndex = queue->_readIndex;
				size_t writeIndex = _count;
				while (count < queue->_len) {
					this->_buffer[writeIndex] = std::move(queue->_buffer[moveIndex]);
					moveIndex = (moveIndex + 1) % QUEUE_CAPACITY;
					writeIndex++;
					count++;
				}
				this->_count += count;
				queue->_len = 0;
				queue->_readIndex = 0;
				queue->_writeIndex = 0;
			}

			void invokeAllJobs() {
				for (size_t i = 0; i < _count; i++) {
					JobContainer& job = _buffer[i];
					job.invoke();
					job.~JobContainer();
				}
				_count = 0;
			}

			size_t getCount() const {
				return _count;
			}

		private:
			// JobData is 64 byte aligned regardless so it doesn't make sense to use u32, when there will already be paddings
			size_t _count; // num of jobs
			JobContainer _buffer[QUEUE_CAPACITY];
		};

	public:

		JobThread();

		JobThread(const JobThread& other) = delete;
		JobThread(JobThread&& other) = delete;
		JobThread& operator = (const JobThread& other) = delete;
		JobThread& operator = (JobThread&& other) = delete;

		~JobThread();

		/* Member function. */
		template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
		JobFuture<ReturnT> runJob(ReturnT(FuncClassT::* inFunc)(Args...), ObjT* inObject, Args&&... inArgs) {
			WithinJobFuture<ReturnT> inFuture = makeInternalFuture<ReturnT>();
			JobFuture<ReturnT> outFuture = inFuture.makeUserJobFuture();
			JobContainer job = JobContainer::bindMember(inObject, inFunc, std::move(inFuture), std::forward<Args>(inArgs)...);
			queueJob(std::move(job));
			return outFuture;
		}

		/* Const member function. */
		template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
		JobFuture<ReturnT> runJob(ReturnT(FuncClassT::* inFunc)(Args...) const, const ObjT* inObject, Args&&... inArgs) {
			WithinJobFuture<ReturnT> inFuture = makeInternalFuture<ReturnT>();
			JobFuture<ReturnT> outFuture = inFuture.makeUserJobFuture();
			JobContainer job = JobContainer::bindConstMember(inObject, inFunc, std::move(inFuture), std::forward<Args>(inArgs)...);
			queueJob(std::move(job));
			return outFuture;
		}

		/* Free function. */
		template<typename ReturnT, typename... Args>
		JobFuture<ReturnT> runJob(ReturnT(*inFunc)(Args...), Args&&... inArgs) {
			WithinJobFuture<ReturnT> inFuture = makeInternalFuture<ReturnT>();
			JobFuture<ReturnT> outFuture = inFuture.makeUserJobFuture();
			JobContainer job = JobContainer::bindFreeFunction(inFunc, std::move(inFuture), std::forward<Args>(inArgs)...);
			queueJob(std::move(job));
			return outFuture;
		}

		/* Compatibility with std::function, std::bind, etc. */
		template<typename ReturnT>
		JobFuture<ReturnT> runJob(std::function<ReturnT()>&& inFunc) {
			WithinJobFuture<ReturnT> inFuture = makeInternalFuture<ReturnT>();
			JobFuture<ReturnT> outFuture = inFuture.makeUserJobFuture();
			JobContainer job = JobContainer::bindStdFunction(inFunc, std::move(inFuture));
			queueJob(std::move(job));
			return outFuture;
		}

		void wait() const;

		[[nodiscard]] bool isExecuting() const;

		[[nodiscard]] u32 queuedJobCount() const;

		[[nodiscard]] std::thread::id getThreadId() const;

	private:

		void queueJob(JobContainer&& job);

		void notifyExecute();

		template<typename ReturnT>
		WithinJobFuture<ReturnT> makeInternalFuture() {
			if constexpr (std::is_same_v<ReturnT, void>) {
				return WithinJobFuture<ReturnT>(false);
			}
			else if constexpr (std::is_arithmetic_v<ReturnT>) {
				return WithinJobFuture<ReturnT>(0);
			}
			else if constexpr (std::is_pointer_v<ReturnT>) {
				return WithinJobFuture<ReturnT>(nullptr);
			}
			else {
				return WithinJobFuture<ReturnT>(ReturnT());
			}
		}

		void threadLoop();

		void executeQueuedJobs();

	private:

		std::atomic<bool> _isExecuting;
		std::atomic<bool> _shouldExecute;
		std::atomic<bool> _isPendingKill;

		std::atomic<u32> _queuedJobCount;

		std::mutex _mutex;
		std::condition_variable _condVar;
		std::thread _thread;

		gk::Mutex<JobRingQueue> _queue;
		gk::Mutex<ActiveJobs> _activeWork;

	};
} // namespace gk
