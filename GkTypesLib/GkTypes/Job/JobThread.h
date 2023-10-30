#pragma once

#include "../BasicTypes.h"
#include "JobInfo.h"
#include <Windows.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include "../Array/DynamicArray.h"
#include "../Thread/Mutex.h"

// https://learn.microsoft.com/en-us/windows/win32/sync/using-condition-variables

namespace gk
{
	/* About 1.5 megabytes per instance. */
	struct JobThread
	{
		static constexpr uint32 QUEUE_CAPACITY = 8192;

	private:

		struct JobRingQueue {

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

			void push(JobData&& element) {
				gk_assertm(!isFull(), "Job ring queue is full");
				_buffer[_writeIndex] = std::move(element);
				_writeIndex = (_writeIndex + 1) % QUEUE_CAPACITY;
				_len++;
			}

			JobData pop() {
				gk_assertm(len() > 0, "Job ring queue is empty");
				const size_t i = _readIndex;
				_readIndex = (_readIndex + 1) % QUEUE_CAPACITY;
				_len--;
				return std::move(_buffer[i]);
			}

		private:
			// JobData is 64 byte aligned regardless so it doesn't make sense to use uint32, when there will already be padding
			size_t _len;
			size_t _readIndex;
			size_t _writeIndex;
			JobData _buffer[QUEUE_CAPACITY];
		};

		struct JobQueues {
			JobRingQueue high;
			JobRingQueue medium;
			JobRingQueue low;
		};

		struct ActiveJobs {

			ActiveJobs() :_count(0) {}

			void push(JobData&& element) {
				_buffer[_count] = std::move(element);
				_count++;
			}

			void execute() {
				for (size_t i = 0; i < _count; i++) {
					JobData& job = _buffer[i];
					job.jobFunc.invoke(&job.data);
					job.~JobData();
				}
				_count = 0;
			}

			size_t count() const {
				return _count;
			}

		private:
			// JobData is 64 byte aligned regardless so it doesn't make sense to use uint32, when there will already be paddings
			size_t _count; // num of jobs
			JobData _buffer[QUEUE_CAPACITY];
		};

	public:

		JobThread()
		{
			_thread = std::thread{ &JobThread::threadLoop, this };
			_isPendingKill = false;
			_shouldExecute = false;
			_isExecuting = false;
		}

		JobThread(const JobThread& other) = delete;
		JobThread(JobThread&& other) = delete;

		~JobThread() {
			wait();
			_isPendingKill = true;
			execute();
			_thread.join();
		}

		// Will take ownership of the job
		void queueJob(JobData&& job) {
			gk_assertm(job.jobFunc.isBound(), "Queued job does not have a bound function for execution");
			auto lock = _queue.lock();
			lock.get()->push(std::move(job));
			_queuedJobsCount++;
		}

		// Will take ownership of the jobs
		void queueJobs(gk::darray<JobData>&& jobs) {
			queueJobs(jobs.Data(), jobs.Size());
		}

		// Will take ownership of the jobs
		void queueJobs(JobData* arrayStart, const uint32 count) {
			auto lock = _queue.lock();
			for (uint32 i = 0; i < count; i++) {
				JobData& job = arrayStart[i];
				gk_assertm(job.jobFunc.isBound(), "Queued job does not have a bound function for execution");
				lock.get()->push(std::move(job));
			}
			_queuedJobsCount += count;
		}

		void execute() {
			_shouldExecute = true;
			std::scoped_lock lock(_mutex);
			_condVar.notify_one();
			_isExecuting = true;
		}

		[[nodiscard]] bool isExecuting() const { return _isExecuting; }

		/* Get the total amount of jobs this thread has in queue.l */
		[[nodiscard]] uint32 queuedJobsCount() const {
			return _queuedJobsCount;
		}

		void wait() const {
			while (_isExecuting) {
				std::this_thread::yield();
			}
		}

	private:

		void threadLoop() {
			while (_isPendingKill == false) {
				{ // Wait for shouldExecute, in scope.
					std::unique_lock lck(_mutex);
					_condVar.wait(lck, [&] {return _shouldExecute.load(); });
					_shouldExecute = false;
				}
				executeJobs();
			}
		}

		void loadAllJobs() {
			gk::LockedMutex<JobRingQueue> lock = _queue.lock();
			{
				loadJobs(lock.get(), _activeWork.lock().get());
			}
		}

		void loadJobs(JobRingQueue* queue, ActiveJobs* active) {
			while (queue->len()) {
				active->push(std::move(queue->pop()));
			}
			_queuedJobsCount = 0;
		}

		void executeJobs() {
			loadAllJobs();
			runActiveJobs();
			_isExecuting = false;
		}

		void runActiveJobs() {
			bool shouldReExecute = false;
			{
				gk::LockedMutex<ActiveJobs> activeLock = _activeWork.lock();
				activeLock.get()->execute();
				gk::LockedMutex<JobRingQueue> queueLock = _queue.lock();
				if (queueLock.get()->len() > 0) {
					loadJobs(queueLock.get(), activeLock.get());
					shouldReExecute = true;
				}
			}
			if (shouldReExecute) {
				runActiveJobs();
			}
		}

		/*
		bool stealJobs(JobThread* other) {
			std::scoped_lock lck(_mutex);
			std::scoped_lock lck(other->_mutex);

			if (other->_highPriorityQueue.len() > 0) {
				for (JobData highJob : other->_highPriorityQueue) {
					_highPriorityActiveWork[_highActiveJobCount] = std::move(highJob);
					_highActiveJobCount++;
				}
			}

			if (other->_mediumPriorityQueue.len() > 0) {
				for (JobData highJob : other->_mediumPriorityQueue) {
					_mediumPriorityActiveWork[_mediumActiveJobCount] = std::move(highJob);
					_mediumActiveJobCount++;
				}
			}

			if (other->_lowPriorityQueue.len() > 0) {
				for (JobData highJob : other->_lowPriorityQueue) {
					_lowPriorityActiveWork[_lowActiveJobCount] = std::move(highJob);
					_lowActiveJobCount++;
				}
			}
		}*/

	private:

		std::atomic<bool> _shouldExecute;
		std::atomic<bool> _isExecuting;
		std::atomic<bool> _isPendingKill;

		std::atomic<uint32> _queuedJobsCount;

		std::mutex _mutex;
		std::condition_variable _condVar;
		std::thread _thread;

		gk::Mutex<JobRingQueue> _queue;
		gk::Mutex<ActiveJobs> _activeWork;

	};

	struct JobThreadArray
	{
		JobThread* arr;
		uint32 threadCount;

		JobThreadArray() : arr(nullptr), threadCount(0) {}

		struct iterator
		{
			iterator(JobThread* data) : _data(data) {}

			iterator& operator++() { _data++; return *this; }

			bool operator !=(const iterator& other) const { return _data != other._data; }

			JobThread* operator*() { return _data; }
			const JobThread* operator*() const { return _data; }

		private:
			JobThread* _data;
		};

		/* Iterator begin. */
		iterator begin() const { return iterator(arr); }

		/* Iterator end. */
		iterator end() const { return iterator(arr + threadCount); }
	};
}
