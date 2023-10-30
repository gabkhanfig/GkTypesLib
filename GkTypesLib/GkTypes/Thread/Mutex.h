#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include <atomic>
#include <thread>
#include <windows.h>
#include "ThreadEnums.h"

namespace gk
{
	template<typename T>
	struct Mutex;

	/* Mutex that has been locked, containing a reference to T owned/held by the mutex. Owning Mutex must live in the same scope.
	Usage:
	gk::Mutex<int> mutex;
	LockedMutex<int> val = mutex.lock();
	*val = 5;*/
	template<typename T>
	struct LockedMutex {
	private:

		typedef void(Mutex<T>::* UnlockFunc)();

	public:

		LockedMutex(Mutex<T>* mutex, UnlockFunc unlockFunc, T* data) : _mutex(mutex), _unlockFunc(unlockFunc), _data(data) {}

		LockedMutex(const LockedMutex& other) = delete;
		LockedMutex(LockedMutex&& other) = delete;
		LockedMutex& operator = (const LockedMutex& other) = delete;
		LockedMutex& operator = (LockedMutex&& other) = delete;

		~LockedMutex() {
			(_mutex->*_unlockFunc)();
		}

		[[nodiscard]] T* get() {
			return _data;
		}

		[[nodiscard]] const T* get() const {
			return _data;
		}

	private:

		Mutex<T>* _mutex;
		UnlockFunc _unlockFunc;
		T* _data;

	};

	/* Lock-free mutex that forces locking in order to access it's owned/held data. */
	template<typename T>
	struct Mutex {
	private:

		constexpr static uint64 THREAD_ID_BITMASK = 0xFFFFFFFF00000000ULL;
		constexpr static uint64 THREAD_LOCK_COUNT_BITMASK = 0xFFFFFFFF;

	public:

		Mutex() {
			_lockState.store(0, std::memory_order_release);
		}

		Mutex(const Mutex& other) = delete;
		Mutex(Mutex&& other) = delete;
		Mutex& operator = (const Mutex& other) = delete;
		Mutex& operator = (Mutex&& other) = delete;

		Mutex(const T& data) {
			_lockState.store(0, std::memory_order_release);
			_data = data;
		}

		Mutex(T&& data) {
			_lockState.store(0, std::memory_order_release);
			_data = std::move(data);
		}

		~Mutex() = default;

		/* Supports recursive locking. Is unlocked by the destructor of LockedMutex<T>. 
		Will yield to the operating system to retry locking. */
		[[nodiscard]] LockedMutex<T> lock() {
			const std::thread::id id = std::this_thread::get_id();
			const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);

			uint64 expected = _lockState.load(std::memory_order_acquire);
			//if (((expected & threadIdBitmask) == (thisThreadId) << 32) && (expected & threadLockedBitmask) > 0) { // already owned and is going to get nested
			//	const uint64 desired = expected + 1;
			//	expected = expected & threadIdBitmask; 
			//	//std::cerr << "incrementing lock count for thread id: " << thisThreadId << " to " << (desired & threadLockedBitmask) << std::endl;
			//	while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
			//		_mm_pause();
			//		expected = (thisThreadId << 32) | (expected & threadLockedBitmask);
			//	}
			//	return LockedMutex(this, &Mutex::unlock, &_data);
			//}

			if (((expected & THREAD_ID_BITMASK) == (thisThreadId) << 32) && (expected & THREAD_LOCK_COUNT_BITMASK) > 0) {
				_lockState.store(expected + 1, std::memory_order_release); // support nested lock
				return LockedMutex(this, &Mutex::unlock, &_data);
			}

			const uint64 desired = (thisThreadId << 32) | 1; // If thread doesn't own a lock already, this will be the first nested lock.
			expected = expected & THREAD_ID_BITMASK;
			while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
				std::this_thread::yield();
				expected = expected & THREAD_ID_BITMASK; // Ensure has no active locks, so the 32 lowest bits are zero
			}

			return LockedMutex(this, &Mutex::unlock, &_data);
		}

		/* Supports recursive locking. Is unlocked by the destructor of LockedMutex<T>. 
		Will not yield to the operating system to retry locking. */
		[[nodiscard]] LockedMutex<T> spinlock() {
			const std::thread::id id = std::this_thread::get_id();
			const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);

			uint64 expected = _lockState.load(std::memory_order_acquire);
			//if (((expected & threadIdBitmask) == (thisThreadId) << 32) && (expected & threadLockedBitmask) > 0) { // already owned and is going to get nested
			//	const uint64 desired = expected + 1;
			//	expected = expected & threadIdBitmask; 
			//	//std::cerr << "incrementing lock count for thread id: " << thisThreadId << " to " << (desired & threadLockedBitmask) << std::endl;
			//	while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
			//		_mm_pause();
			//		expected = (thisThreadId << 32) | (expected & threadLockedBitmask);
			//	}
			//	return LockedMutex(this, &Mutex::unlock, &_data);
			//}

			if (((expected & THREAD_ID_BITMASK) == (thisThreadId) << 32) && (expected & THREAD_LOCK_COUNT_BITMASK) > 0) {
				_lockState.store(expected + 1, std::memory_order_release); // support nested lock
				return LockedMutex(this, &Mutex::unlock, &_data);
			}

			const uint64 desired = (thisThreadId << 32) | 1; // If thread doesn't own a lock already, this will be the first nested lock.
			expected = expected & THREAD_ID_BITMASK;
			while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
				_mm_pause();
				expected = expected & THREAD_ID_BITMASK; // Ensure has no active locks, so the 32 lowest bits are zero
			}

			return LockedMutex(this, &Mutex::unlock, &_data);
		}

		[[nodiscard]] T* getDataNoLock() {
			return &_data;
		}

		[[nodiscard]] const T* getDataNoLock() const {
			return &_data;
		}

	private:

		/* Is unlocked by the destructor of LockedMutex<T>. For nested locks, it just decrements the lock count. */
		void unlock() {
			const uint64 current = _lockState.load(std::memory_order_acquire);

			gk_assertm([&]() {
				const std::thread::id id = std::this_thread::get_id();
				const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);
				return (current & THREAD_ID_BITMASK) == (thisThreadId << 32);		
				}(), "Cannot unlock a mutex lock that is not currently owned by the calling thread");
			gk_assertm((current & THREAD_LOCK_COUNT_BITMASK) > 0, "Cannot unlock a mutex lock that is not locked");

			
			_lockState.store((current & THREAD_ID_BITMASK) | ((current & THREAD_LOCK_COUNT_BITMASK) - 1), std::memory_order_release); // support nested lock
		}

	private:

		std::atomic<uint64> _lockState;
		T _data;
	};
}