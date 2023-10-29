#pragma once

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

		~LockedMutex() {
			(_mutex->*_unlockFunc)();
		}

		LockedMutex& operator = (const LockedMutex& other) = delete;
		LockedMutex& operator = (LockedMutex&& other) = delete;

		[[nodiscard]] T& operator * () {
			return *_data;
		}

		[[nodiscard]] const T& operator * () const {
			return *_data;
		}

	private:

		Mutex<T>* _mutex;
		UnlockFunc _unlockFunc;
		T* _data;

	};

	/* Mutex that forces locking in order to access it's owned/held data. */
	template<typename T>
	struct Mutex {
	public:

		Mutex() {
			InitializeSRWLock(&_lock);
		}

		Mutex(const Mutex& other) = delete;
		Mutex(Mutex&& other) = delete;

		Mutex(const T& data) {
			InitializeSRWLock(&_lock);
			_data = data;
		}

		Mutex(T&& data) {
			InitializeSRWLock(&_lock);
			_data = std::move(data);
		}

		Mutex& operator = (const Mutex& other) = delete;
		Mutex& operator = (Mutex&& other) = delete;

		/* Does not support recursive locking. */
		[[nodiscard]] LockedMutex<T> lock() {
			AcquireSRWLockExclusive(&_lock);
			return LockedMutex(this, &Mutex::unlock, &_data);
		}

		[[nodiscard]] T& getDataNoLock() {
			return _data;
		}

		[[nodiscard]] const T& getDataNoLock() const {
			return _data;
		}

	private:

		/* Is unlocked by the destructor of LockedMutex<T> */
		void unlock() {
			ReleaseSRWLockExclusive(&_lock);
		}

	private:

		SRWLOCK _lock;
		T _data;
	};
}