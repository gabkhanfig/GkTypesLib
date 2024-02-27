#pragma once

#include <atomic>
#include "../basic_types.h"
#include "../sync/mutex.h"
#include "../doctest/doctest_proxy.h"
#include <thread>
#include "../ptr/shared_ptr.h"

namespace gk
{
	template<typename ReturnT = void>
	struct JobFuture;

	namespace internal
	{
		template<typename ReturnT>
		struct JobFutureSharedMutex {

			using DataT = std::conditional_t<std::is_same_v<ReturnT, void>, bool, ReturnT>;

			struct MutexData {
				bool isReady = false;
				DataT actualData;

				MutexData() {
					if constexpr (std::is_same_v<ReturnT, void>) {
						actualData = false;
					}
					else if constexpr (std::is_arithmetic_v<ReturnT>) {
						actualData = 0;
					}
					else if constexpr (std::is_pointer_v<ReturnT>) {
						actualData = nullptr;
					}
					else {
						std::construct_at(&this->actualData, ReturnT());
					}
				}
			};

			SharedPtr<Mutex<MutexData>> data;
			static JobFutureSharedMutex makeShared(const DataT& inData) {
				JobFutureSharedMutex shared{ .data = SharedPtr<Mutex<MutexData>>::create() };
				Mutex<MutexData>* m = shared.data.get();
				LockedMutex<MutexData> lock = m->lock();
				std::construct_at(&lock->actualData, inData);
				return shared;
			}

			static JobFutureSharedMutex makeShared(DataT&& inData) {
				JobFutureSharedMutex shared{ .data = SharedPtr<Mutex<MutexData>>::create() };
				Mutex<MutexData>* m = shared.data.get();
				LockedMutex<MutexData> lock = m->lock();
				std::construct_at(&lock->actualData, std::move(inData));
				return shared;
			}

		};

		template<typename ReturnT>
		struct WithinJobFuture
		{
			using DataT = std::conditional_t<std::is_same_v<ReturnT, void>, bool, ReturnT>;

			WithinJobFuture(const DataT& inData)
				: shared(JobFutureSharedMutex<DataT>::makeShared(inData))
			{
				//shared = std::move(JobFutureSharedMutex<DataT>::makeShared(inData));
			}

			WithinJobFuture(DataT&& inData) noexcept 
				: shared(JobFutureSharedMutex<DataT>::makeShared(std::move(inData)))
			{
				//shared = std::move(JobFutureSharedMutex<DataT>::makeShared(std::move(inData)));
			}

			WithinJobFuture(const WithinJobFuture& other) = delete;
			WithinJobFuture(WithinJobFuture&& other) noexcept : shared(std::move(other.shared)) {}

			WithinJobFuture& operator = (const WithinJobFuture& other) = delete;
			WithinJobFuture& operator = (WithinJobFuture&& other) noexcept { shared = std::move(other.shared); }

			JobFuture<ReturnT> makeUserJobFuture() const;

			void set(const DataT& inData) {
				//gk_assertm(shared.data != nullptr, "Cannot set on invalid future. It's possible was moved");
				const u64 count = shared.data->counter.load(std::memory_order::acquire);
				if (count < 2) { // don't update unnecessarily
					//std::cout << "future count is less than 2, dont bother\n";
					return;
				}
				auto lock = shared.data->lock();
				lock.get()->actualData = inData;
				lock.get()->isReady = true;
			}

			void set(DataT&& inData) {
				//gk_assertm(shared.data != nullptr, "Cannot set on invalid future. It's possible was moved");
				//const u64 count = shared.data->counter.load(std::memory_order::acquire);
				if (this->shared.data.refCount() < 2) { // don't update unnecessarily
					//std::cout << "future count is less than 2, dont bother\n";
					return;
				}
				auto lock = shared.data->lock();
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
			//gk_assertm(shared.data != nullptr, "Cannot wait on invalid future. It's possible has had wait() called already, or was moved");
			while (true) {
				auto optionalLock = shared.data->tryLock();
				if (!optionalLock.none()) {
					auto lock = optionalLock.some();
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
} // namespace gk