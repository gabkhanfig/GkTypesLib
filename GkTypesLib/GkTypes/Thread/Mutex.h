#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include "../Option/Option.h"
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
		LockedMutex(LockedMutex&& other) {
			_mutex = other._mutex;
			other._mutex = nullptr;
		}
		LockedMutex& operator = (const LockedMutex& other) = delete;
		LockedMutex& operator = (LockedMutex&& other) = delete;

		~LockedMutex();

		[[nodiscard]] T* get();

		[[nodiscard]] const T* get() const;

	private:

		friend struct gk::Option<LockedMutex<T>>;

		LockedMutex() : _mutex(nullptr) {}

	private:

		Mutex<T>* _mutex;

	};

	/* Lock-free mutex that forces locking in order to access it's owned/held data. */
	template<typename T>
	struct Mutex {
	private:


		template<typename>
		friend struct LockedMutex;

	public:

		Mutex() {
			InitializeSRWLock(&_lock);
		}

		template<typename ...ConstructorArgs>
		Mutex(ConstructorArgs... args)
			: _data(args...)
		{
			InitializeSRWLock(&_lock);
		}

		Mutex(const Mutex& other) = delete;
		Mutex(Mutex&& other) = delete;
		Mutex& operator = (const Mutex& other) = delete;
		Mutex& operator = (Mutex&& other) = delete;

		Mutex(const T& data)
			: _data(data)
		{
			InitializeSRWLock(&_lock);
		}

		Mutex(T&& data)
			: _data(std::move(data))
		{
			InitializeSRWLock(&_lock);
		}

		~Mutex() = default;

		/* Does NOT support recursize locking. Is unlocked by the destructor of LockedMutex<T>. */
		[[nodiscard]] LockedMutex<T> lock() {
			AcquireSRWLockExclusive(&_lock);
			return LockedMutex(this);
		}

		[[nodiscard]] gk::Option<LockedMutex<T>> tryLock() {
			if (!TryAcquireSRWLockExclusive(&_lock)) {
				return gk::Option<LockedMutex<T>>();
			}
			return gk::Option<LockedMutex<T>>(this);
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
		}

	private:

		//std::atomic<uint64> _lockState; // faster than std::mutex
		SRWLOCK _lock;
		T _data;
	};

	template<typename T>
	inline LockedMutex<T>::~LockedMutex()
	{
		if (_mutex) {
			_mutex->unlock();
		}	
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

