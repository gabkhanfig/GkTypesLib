#pragma once

#include "../basic_types.h"
#include "../option/option.h"

namespace gk
{
	struct RawMutex {

		RawMutex();
		
		~RawMutex() = default;

		RawMutex(const RawMutex&) = delete;
		RawMutex(RawMutex&&) = delete;
		RawMutex& operator = (const RawMutex&) = delete;
		RawMutex& operator = (RawMutex&&) = delete;

		void lock();

		[[nodiscard]] bool tryLock();

		void unlock();

	private:
#if defined(_WIN32) || defined(WIN32)
		void* srwlock;
#endif
	};


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
		LockedMutex(LockedMutex&& other) noexcept {
			_mutex = other._mutex;
			other._mutex = nullptr;
		}
		LockedMutex& operator = (const LockedMutex& other) = delete;
		LockedMutex& operator = (LockedMutex&& other) = delete;

		~LockedMutex();

		[[nodiscard]] T* get();

		[[nodiscard]] const T* get() const;

		[[nodiscard]] T* operator->() { return get(); }

		[[nodiscard]] const T* operator->() const { return get(); }

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

		Mutex() = default;

		template<typename ...ConstructorArgs>
		Mutex(ConstructorArgs&&... args)
			: _data(args...)
		{}

		Mutex(const Mutex& other) = delete;
		Mutex(Mutex&& other) = delete;
		Mutex& operator = (const Mutex& other) = delete;
		Mutex& operator = (Mutex&& other) = delete;

		Mutex(const T& data)
			: _data(data)
		{}

		Mutex(T&& data)
			: _data(std::move(data))
		{}

		~Mutex() = default;

		/* Does NOT support recursize locking. Is unlocked by the destructor of LockedMutex<T>. */
		[[nodiscard]] LockedMutex<T> lock() {
			this->_lock.lock();
			return LockedMutex(this);
		}

		[[nodiscard]] gk::Option<LockedMutex<T>> tryLock() {
			if (!this->_lock.tryLock()) {
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
			this->_lock.unlock();
		}

	private:

		RawMutex _lock;
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

