#pragma once

#include <atomic>
#include "../BasicTypes.h"
#include "../Thread/Mutex.h"
#include "../Asserts.h"
#include <thread>

namespace gk
{
	namespace job
	{
		template<typename ReturnT = void>
		struct JobFuture;

		namespace internal
		{
			template<typename ReturnT>
			struct JobFutureSharedMutex {

				using DataT = std::conditional_t<std::is_same_v<ReturnT, void>, bool, ReturnT>;

				struct MutexData {
					bool isReady;
					DataT actualData;

					MutexData(const DataT& inData)
						: isReady(false), actualData(inData) {}

					MutexData(DataT&& inData)
						: isReady(false), actualData(std::move(inData)) {}
				};

				struct SharedData {
					std::atomic<uint64> counter;
					gk::Mutex<MutexData> mutex;

					SharedData(const DataT& inData)
						: counter(1), mutex(inData) {}

					SharedData(DataT&& inData)
						: counter(1), mutex(std::move(inData)) {}

				};

				// only member
				SharedData* data;

				JobFutureSharedMutex() : data(nullptr) {}

				JobFutureSharedMutex(const JobFutureSharedMutex& other) {
					data = other.data;
					if (data == nullptr) return;

					data->counter++;
				}

				JobFutureSharedMutex(JobFutureSharedMutex&& other) noexcept {
					data = other.data;
					other.data = nullptr;
				}

				~JobFutureSharedMutex() {
					decrementCounter();
				}

				JobFutureSharedMutex& operator = (const JobFutureSharedMutex& other) {
					decrementCounter();

					data = other.data;
					if (data == nullptr) return *this;

					data->counter++;
					return *this;
				}

				JobFutureSharedMutex& operator = (JobFutureSharedMutex&& other) noexcept {
					decrementCounter();
					data = other.data;
					other.data = nullptr;
					return *this;
				}

				static JobFutureSharedMutex makeShared(const DataT& inData) {
					JobFutureSharedMutex shared;
					shared.data = new SharedData(inData);
					//std::cout << "counter make shared yuh: " << shared.data->counter << std::endl;
					return shared;
				}

				static JobFutureSharedMutex makeShared(DataT&& inData) {
					JobFutureSharedMutex shared;
					shared.data = new SharedData(std::move(inData));
					//std::cout << "counter make shared yuh: " << shared.data->counter << std::endl;
					return shared;
				}

			private:

				void decrementCounter() {
					if (data == nullptr) return;
					//std::cout << "counter decrement yuh: " <<  data->counter << std::endl;
					data->counter--;
					if (data->counter == 0) {
						//std::cout << "no refs, deleting";
						delete data;
					}
				}

			};

			template<typename ReturnT>
			struct WithinJobFuture
			{
				using DataT = std::conditional_t<std::is_same_v<ReturnT, void>, bool, ReturnT>;

				WithinJobFuture(const DataT& inData) {
					shared = std::move(JobFutureSharedMutex<DataT>::makeShared(inData));
				}

				WithinJobFuture(DataT&& inData) noexcept {
					shared = std::move(JobFutureSharedMutex<DataT>::makeShared(std::move(inData)));
				}

				WithinJobFuture(const WithinJobFuture& other) = delete;
				WithinJobFuture(WithinJobFuture&& other) noexcept : shared(std::move(other.shared)) {}

				WithinJobFuture& operator = (const WithinJobFuture& other) = delete;
				WithinJobFuture& operator = (WithinJobFuture&& other) noexcept { shared = std::move(other.shared); }

				JobFuture<ReturnT> makeUserJobFuture() const;

				void set(const DataT& inData) {
					gk_assertm(shared.data != nullptr, "Cannot set on invalid future. It's possible was moved");
					const uint64 count = shared.data->counter.load(std::memory_order::acquire);
					if (count < 2) { // don't update unnecessarily
						//std::cout << "future count is less than 2, dont bother\n";
						return;
					}
					auto lock = shared.data->mutex.lock();
					lock.get()->actualData = inData;
					lock.get()->isReady = true;
				}

				void set(DataT&& inData) {
					gk_assertm(shared.data != nullptr, "Cannot set on invalid future. It's possible was moved");
					const uint64 count = shared.data->counter.load(std::memory_order::acquire);
					if (count < 2) { // don't update unnecessarily
						//std::cout << "future count is less than 2, dont bother\n";
						return;
					}
					auto lock = shared.data->mutex.lock();
					lock.get()->actualData = std::move(inData);
					lock.get()->isReady = true;
				}

			private:

				JobFutureSharedMutex<DataT> shared;
			};
		} // namespace internal

		template<typename ReturnT>
		struct JobFuture {
		private:

			using DataT = std::conditional_t<std::is_same_v<ReturnT, void>, bool, ReturnT>;
			friend struct internal::WithinJobFuture<ReturnT>;

		public:

			JobFuture(const JobFuture&) = delete;
			JobFuture(JobFuture&& other) noexcept : shared(std::move(other.shared)) {}

			JobFuture& operator = (const JobFuture&) = delete;
			JobFuture& operator = (JobFuture&& other) noexcept { shared = std::move(other.shared); }

			~JobFuture() = default;

			/**/
			[[nodiscard]] ReturnT wait() {
				gk_assertm(shared.data != nullptr, "Cannot wait on invalid future. It's possible has had wait() called already, or was moved");
				while (true) {
					auto optionalLock = shared.data->mutex.tryLock();
					if (!optionalLock.none()) {
						auto lock = optionalLock.someMove();
						if (lock.get()->isReady) {
							if constexpr (std::is_same<ReturnT, void>::value) {
								return;
							}
							else {
								return std::move(lock.get()->actualData);
							}
						}
						//else {
						//	std::cout << "is not ready\n";
						//}
					}
					std::this_thread::yield();
				}
			}

		private:

			JobFuture(const internal::JobFutureSharedMutex<DataT>& other)
				: shared(other) {} // make a copy

		private:

			internal::JobFutureSharedMutex<DataT> shared;
		};


		template<typename ReturnT>
		inline JobFuture<ReturnT> internal::WithinJobFuture<ReturnT>::makeUserJobFuture() const
		{
			return JobFuture<ReturnT>(this->shared);
		}
	} // namespace job
} // namespace gk