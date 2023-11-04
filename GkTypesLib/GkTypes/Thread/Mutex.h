#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
	public:

		LockedMutex(Mutex<T>* mutex) : _mutex(mutex) {}

		LockedMutex(const LockedMutex& other) = delete;
		LockedMutex(LockedMutex&& other) = delete;
		LockedMutex& operator = (const LockedMutex& other) = delete;
		LockedMutex& operator = (LockedMutex&& other) = delete;

		~LockedMutex();

		[[nodiscard]] T* get();

		[[nodiscard]] const T* get() const;

	private:

		Mutex<T>* _mutex;

	};

	/* Lock-free mutex that forces locking in order to access it's owned/held data. */
	template<typename T>
	struct Mutex {
	private:

		//constexpr static uint64 THREAD_ID_BITMASK = 0xFFFFFFFF00000000ULL;
		//constexpr static uint64 THREAD_LOCK_COUNT_BITMASK = 0xFFFFFFFF;

		template<typename>
		friend struct LockedMutex;

	public:

		Mutex() {
			//_lockState.store(0, std::memory_order_release);
		}

		template<typename ...ConstructorArgs>
		Mutex(ConstructorArgs... args)
			: _data(args...)
		{
			//_lockState.store(0, std::memory_order_release);
			InitializeSRWLock(&_lock);
		}

		Mutex(const Mutex& other) = delete;
		Mutex(Mutex&& other) = delete;
		Mutex& operator = (const Mutex& other) = delete;
		Mutex& operator = (Mutex&& other) = delete;

		Mutex(const T& data)
			: _data(data)
		{
			//_lockState.store(0, std::memory_order_release);
			InitializeSRWLock(&_lock);
		}

		Mutex(T&& data)
			: _data(std::move(data))
		{
			//_lockState.store(0, std::memory_order_release);
			InitializeSRWLock(&_lock);
		}

		~Mutex() = default;

		/* Supports recursive locking. Is unlocked by the destructor of LockedMutex<T>. 
		Will yield to the operating system to retry locking. */
		[[nodiscard]] LockedMutex<T> lock() {
			AcquireSRWLockExclusive(&_lock);
			return LockedMutex(this);
			//const uint64 thisThreadId = GetCurrentThreadId();

			//uint64 expected = _lockState.load(std::memory_order_acquire);

			//if (((expected & THREAD_ID_BITMASK) == (thisThreadId) << 32) && (expected & THREAD_LOCK_COUNT_BITMASK) > 0) {
			//	_lockState.store(expected + 1); // support nested lock , std::memory_order_release
			//	return LockedMutex(this);
			//}

			//const uint64 desired = (thisThreadId << 32) | 1; // If thread doesn't own a lock already, this will be the first nested lock.
			//expected = 0;//expected & THREAD_ID_BITMASK;
			//while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
			//	std::this_thread::yield();
			//	expected = 0;//expected & THREAD_ID_BITMASK; // Ensure has no active locks, so the 32 lowest bits are zero
			//}

			//return LockedMutex(this);
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
			ReleaseSRWLockExclusive(&_lock);

			//const uint64 current = _lockState.load(std::memory_order_acquire);

			//gk_assertm([&]() {
			//	//const std::thread::id id = std::this_thread::get_id();
			//	//const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);
			//	const uint64 thisThreadId = GetCurrentThreadId();
			//	return (current & THREAD_ID_BITMASK) == (thisThreadId << 32);		
			//	}(), "Cannot unlock a mutex lock that is not currently owned by the calling thread");
			//gk_assertm((current & THREAD_LOCK_COUNT_BITMASK) > 0, "Cannot unlock a mutex lock that is not locked");

			//if ((current & THREAD_LOCK_COUNT_BITMASK) == 1) {
			//	_lockState.store(0, std::memory_order_release);
			//	return;
			//}
			//_lockState.store((current & THREAD_ID_BITMASK) | ((current & THREAD_LOCK_COUNT_BITMASK) - 1), std::memory_order_release); // support nested lock
		}

	private:

		//std::atomic<uint64> _lockState; // faster than std::mutex
		SRWLOCK _lock;
		T _data;
	};

	template<typename T>
	inline LockedMutex<T>::~LockedMutex()
	{
		_mutex->unlock();
	}

	template<typename T>
	inline T* LockedMutex<T>::get()
	{
		return &_mutex->_data;
	}

	template<typename T>
	inline const T* LockedMutex<T>::get() const
	{
		return &_mutex->_data;
	}
}

