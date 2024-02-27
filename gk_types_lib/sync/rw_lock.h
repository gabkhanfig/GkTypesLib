#pragma once

#include "../basic_types.h"
#include "../option/option.h"

namespace gk
{
	/// Platform specific RwLock. Locking and unlocking must occur manually.
	/// See `gk::RwLock` struct for automatic RAII.
	struct RawRwLock {

		RawRwLock();
		
		~RawRwLock() = default;

		/// Moving and copying an rwlock may never happen because other threads may
		/// be waiting on memory that has become invalid.

		RawRwLock(const RawRwLock&) = delete;
		RawRwLock(RawRwLock&&) = delete;
		RawRwLock& operator = (const RawRwLock&) = delete;
		RawRwLock& operator = (RawRwLock&&) = delete;

		/// Acquire a shared lock. The calling thread MAY NOT acquire an exclusive lock
		/// while this shared lock is active. To unlock, call `unlockShared()`.
		void lockShared();

		/// Try to acquire a shared lock. The calling thread MAY NOT acquire an exclusive lock
		/// while this shared lock is active. To unlock, call `unlockShared()`.
		/// @return true if the lock was acquired, false if it failed to acquire.
		[[nodiscard]] bool tryLockShared();

		/// Acquire an exclusive lock. The calling thread MAY NOT acquire an shared lock
		/// while this exclusive lock is active. To unlock, call `unlockExclusive()`.
		void lockExclusive();

		/// Try to acquire an exclusive lock. The calling thread MAY NOT acquire an shared lock
		/// while this exclusive lock is active. To unlock, call `unlockExclusive()`.
		/// @return true if the lock was acquired, false if it failed to acquire.
		[[nodiscard]] bool tryLockExclusive();

		/// Unlock a shared lock that was acquired by the calling thread.
		void unlockShared();

		/// Unlock an exclusive lock that was acquired by the calling thread.
		void unlockExclusive();

	private:	
#if defined(_WIN32) || defined(WIN32)
		void* srwlock;
#endif
	};

	template<typename T>
	struct RwLock;

	template<typename T>
	struct LockedReader {
	public:

		LockedReader(const RwLock<T>* rwlock) : _rwlock(rwlock) {}

		LockedReader(const LockedReader&) = delete;
		LockedReader(LockedReader&& other) noexcept {
			_rwlock = other._rwlock;
			other._rwlock = nullptr;
		}
		LockedReader& operator = (const LockedReader&) = delete;
		LockedReader& operator = (LockedReader&&) = delete;

		~LockedReader();

		[[nodiscard]] const T* get() const;

		[[nodiscard]] const T* operator->() const { return get(); }

	private:

		friend struct gk::Option<LockedReader<T>>;

		LockedReader() : _rwlock(nullptr) {}

	private:

		const RwLock<T>* _rwlock;

	};

	template<typename T>
	struct LockedWriter {
	private:
		typedef void(RwLock<T>::* UnlockFunc)();
	public:

		LockedWriter(RwLock<T>* rwlock) : _rwlock(rwlock) {}

		LockedWriter(const LockedWriter&) = delete;
		LockedWriter(LockedWriter&& other) noexcept {
			_rwlock = other._rwlock;
			other._rwlock = nullptr;
		}
		LockedWriter& operator = (const LockedWriter&) = delete;
		LockedWriter& operator = (LockedWriter&&) = delete;

		~LockedWriter();

		[[nodiscard]] T* get();

		[[nodiscard]] T* operator->() { return get(); }

	private:

		friend struct gk::Option<LockedWriter<T>>;

		LockedWriter() : _rwlock(nullptr) {}

	private:

		RwLock<T>* _rwlock;

	};

	template<typename T>
	struct RwLock {
	private:

		template<typename>
		friend struct LockedReader;

		template<typename>
		friend struct LockedWriter;

	public:

		RwLock() = default;

		template<typename ...ConstructorArgs>
		RwLock(ConstructorArgs&&... args)
			: _data(args...)
		{}

		RwLock(const RwLock&) = delete;
		RwLock(RwLock&&) = delete;
		RwLock& operator = (const RwLock&) = delete;
		RwLock& operator = (RwLock&&) = delete;

		~RwLock() = default;

		RwLock(const T& data)
			: _data(data)
		{}

		RwLock(T&& data)
			: _data(std::move(data))
		{}

		/**
		* Get a shared read-only lock of the owned RwLock data.
		* The lock is released on the destructor of LockedReader<T>.
		* 
		* @return A guard around shared, read-only data access
		*/
		[[nodiscard]] LockedReader<T> read() const {
			this->_lock.lockShared();
			return LockedReader(this);
		}

		/**
		* Try to get a shared read-only lock of the owned RwLock data.
		* The lock is released on the destructor of LockedReader<T>.
		* Returns None if there is a writer, otherwise there will be a lock.
		* 
		* @return An optional guard around shared, read-only data access. Will be None is access is blocked.
		*/
		[[nodiscard]] Option<LockedReader<T>> tryRead() const {
			if (!this->_lock.tryLockShared()) {
				return Option<LockedReader<T>>();
			}
			return Option<LockedReader<T>>(this);
		}

		/**
		* Get an exclusive, read/write lock of the owned RwLock data.
		* The lock is released on the destructor of LockedWriter<T>.
		* 
		* @return A guard around exclusive, read/write data access.
		*/
		[[nodiscard]] LockedWriter<T> write() {
			this->_lock.lockExclusive();
			return LockedWriter(this);
		}

		/**
		* Try to get an exclusive, read/write lock of the owned RwLock data.
		* The lock is released on the destructor of LockedWriter<T>.
		* Returns None if there are readers, otherwise there will be a lock.
		* 
		* @return An optional guard around shared, exclusive, read/write data access. Will be None is access is blocked.
		*/
		[[nodiscard]] Option<LockedWriter<T>> tryWrite() {
			if (!this->_lock.tryLockExclusive()) {
				return Option<LockedWriter<T>>();
			}
			return Option<LockedWriter<T>>(this);
		}

		/**
		* Unsafely get mutable access to the data held without any locking.
		* Getting data while it's being written to is undefined behaviour,
		* and may introduce all sorts of multithreading bugs.
		* 
		* @return A mutable pointer to the held data.
		*/
		[[nodiscard]] T* unsafeGetDataNoLock() {
			return &_data;
		}

		/**
		* Unsafely get immutable access to the data held without any locking.
		* Getting data while it's being written to is undefined behaviour,
		* and may introduce all sorts of multithreading bugs.
		*
		* @return An immutable pointer to the held data.
		*/
		[[nodiscard]] const T* unsafeGetDataNoLock() const {
			return &_data;
		}

	private:

		/* Is unlocked by the destructor of LockedReader<T> */
		void unlockRead() const {
			this->_lock.unlockShared();
		}

		/* Is unlocked by the destructor of LockedWriter<T> */
		void unlockWrite() {
			this->_lock.unlockExclusive();
		}

	private:

		mutable RawRwLock _lock;
		T _data;

	};

	template<typename T>
	inline LockedReader<T>::~LockedReader()
	{
		if(_rwlock)
			_rwlock->unlockRead();
	}

	template<typename T>
	inline const T* LockedReader<T>::get() const
	{
		return &_rwlock->_data;
	}

	template<typename T>
	inline LockedWriter<T>::~LockedWriter()
	{
		if(_rwlock)
			_rwlock->unlockWrite();
	}

	template<typename T>
	inline T* LockedWriter<T>::get()
	{
		return &_rwlock->_data;
	}
}