#pragma once

//#include <windows.h>
//#include "ThreadEnums.h"
#include "../BasicTypes.h"
#include "../Asserts.h"
#include <atomic>
#include <thread>
#include <windows.h>
#include "ThreadEnums.h"

namespace gk
{
	template<typename T>
	struct RwLock;

	template<typename T>
	struct LockedReader {
	private:
		typedef void(RwLock<T>::* UnlockFunc)() const;
	public:

		LockedReader(const RwLock<T>* rwlock, UnlockFunc unlockFunc, const T* data) : _rwlock(rwlock), _unlockFunc(unlockFunc), _data(data) {}

		LockedReader(const LockedReader&) = delete;
		LockedReader(LockedReader&&) = delete;
		LockedReader& operator = (const LockedReader&) = delete;
		LockedReader& operator = (LockedReader&&) = delete;

		~LockedReader() {
			(_rwlock->*_unlockFunc)();
		}

		[[nodiscard]] const T* get() const {
			return _data;
		}

	private:

		const RwLock<T>* _rwlock;
		UnlockFunc _unlockFunc;
		const T* _data;

	};

	template<typename T>
	struct LockedWriter {
	private:
		typedef void(RwLock<T>::* UnlockFunc)();
	public:

		LockedWriter(RwLock<T>* rwlock, UnlockFunc unlockFunc, T* data) : _rwlock(rwlock), _unlockFunc(unlockFunc), _data(data) {}

		LockedWriter(const LockedWriter&) = delete;
		LockedWriter(LockedWriter&&) = delete; 
		LockedWriter& operator = (const LockedWriter&) = delete;
		LockedWriter& operator = (LockedWriter&&) = delete;

		~LockedWriter() {
			(_rwlock->*_unlockFunc)();
		}

		[[nodiscard]] T* get() {
			return _data;
		}

	private:

		RwLock<T>* _rwlock;
		UnlockFunc _unlockFunc;
		T* _data;

	};

	template<typename T>
	struct RwLock {
	private:

		constexpr static uint64 IS_NOT_OWNED = 0xFFFFFFFF00000000ULL;
		constexpr static uint64 THREAD_ID_BITMASK = 0xFFFFFFFF00000000ULL;
		constexpr static uint64 THREAD_LOCK_COUNT_BITMASK = 0xFFFFFFFF;

	public:

		RwLock() {
			_lockState.store(IS_NOT_OWNED, std::memory_order_release);
		}

		RwLock(const RwLock&) = delete;
		RwLock(RwLock&&) = delete;
		RwLock& operator = (const RwLock&) = delete;
		RwLock& operator = (RwLock&&) = delete;

		~RwLock() = default;

		RwLock(const T& data)
			: _data(data)
		{
			_lockState.store(IS_NOT_OWNED, std::memory_order_release);
		}

		RwLock(T&& data)
			: _data(std::move(data))
		{
			_lockState.store(IS_NOT_OWNED, std::memory_order_release);
		}

		/* Supports recursive locking. Is unlocked by the destructor of LockedReader<T>. 
		Will yield to the operating system to retry locking. */
		[[nodiscard]] LockedReader<T> read() const {
			uint64 expected = IS_NOT_OWNED | (_lockState.load(std::memory_order_acquire) & THREAD_LOCK_COUNT_BITMASK);

			while (!_lockState.compare_exchange_weak(expected, expected + 1, std::memory_order_release)) {
				std::this_thread::yield();
				expected = IS_NOT_OWNED | (expected & THREAD_LOCK_COUNT_BITMASK);
			}

			return LockedReader(this, &RwLock::unlockRead, &_data);
		}

		/* Supports recursive locking. Is unlocked by the destructor of LockedReader<T>. 
		Will not yield to the operating system to retry locking. */
		[[nodiscard]] LockedReader<T> spinRead() const {
			uint64 expected = IS_NOT_OWNED | (_lockState.load(std::memory_order_acquire) & THREAD_LOCK_COUNT_BITMASK);

			while (!_lockState.compare_exchange_weak(expected, expected + 1, std::memory_order_release)) {
				_mm_pause();
				expected = IS_NOT_OWNED | (expected & THREAD_LOCK_COUNT_BITMASK);
			}

			return LockedReader(this, &RwLock::unlockRead, &_data);
		}

		/* Supports recursive locking. Is unlocked by the destructor of LockedWriter<T>. 
		Will yield to the operating system to retry locking. */
		[[nodiscard]] LockedWriter<T> write() {
			const std::thread::id id = std::this_thread::get_id();
			const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);

			uint64 expected = _lockState.load(std::memory_order_acquire);

			if (((expected & THREAD_ID_BITMASK) == (thisThreadId) << 32) && (expected & THREAD_LOCK_COUNT_BITMASK) > 0) {
				_lockState.store(expected + 1, std::memory_order_release); // support nested lock
				return LockedWriter(this, &RwLock::unlockWrite, &_data);
			}

			const uint64 desired = (thisThreadId << 32) | 1; // If thread doesn't own a lock already, this will be the first nested lock.
			expected = IS_NOT_OWNED;
			while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
				std::this_thread::yield();
				expected = IS_NOT_OWNED; // Ensure has no active locks, so the 32 lowest bits are zero
			}

			return LockedWriter(this, &RwLock::unlockWrite, &_data);
		}

		/* Supports recursive locking. Is unlocked by the destructor of LockedWriter<T>. 
		Will not yield to the operating system to retry locking. */
		[[nodiscard]] LockedWriter<T> spinWrite() {
			const std::thread::id id = std::this_thread::get_id();
			const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);

			uint64 expected = _lockState.load(std::memory_order_acquire);

			if (((expected & THREAD_ID_BITMASK) == (thisThreadId) << 32) && (expected & THREAD_LOCK_COUNT_BITMASK) > 0) {
				_lockState.store(expected + 1, std::memory_order_release); // support nested lock
				return LockedWriter(this, &RwLock::unlockWrite, &_data);
			}

			const uint64 desired = (thisThreadId << 32) | 1; // If thread doesn't own a lock already, this will be the first nested lock.
			expected = IS_NOT_OWNED;
			while (!_lockState.compare_exchange_weak(expected, desired, std::memory_order_release)) {
				_mm_pause();
				expected = IS_NOT_OWNED; // Ensure has no active locks, so the 32 lowest bits are zero
			}

			return LockedWriter(this, &RwLock::unlockWrite, &_data);
		}

		[[nodiscard]] T* getDataNoLock() {
			return &_data;
		}

		[[nodiscard]] const T* getDataNoLock() const {
			return &_data;
		}

	private:

		/* Is unlocked by the destructor of LockedReader<T> */
		void unlockRead() const {
			const uint64 current = _lockState.load(std::memory_order_acquire);

			gk_assertm((current & THREAD_ID_BITMASK) == IS_NOT_OWNED, "Cannot unlock a reader rwlock that is not read-owned");
			gk_assertm((current & THREAD_LOCK_COUNT_BITMASK) > 0, "Cannot unlock a reader rwlock that is not locked");

			uint64 expected = current;
			// must compare exchange because of all the threads that may be trying to unlock
			while (!_lockState.compare_exchange_weak(expected, expected - 1, std::memory_order_release)) { 
				_mm_pause();
			}
		}

		/* Is unlocked by the destructor of LockedWriter<T> */
		void unlockWrite() {
			const uint64 current = _lockState.load(std::memory_order_acquire);

			gk_assertm([&]() {
				const std::thread::id id = std::this_thread::get_id();
				const uint64 thisThreadId = static_cast<uint64>(*(uint32*)&id);
				return (current & THREAD_ID_BITMASK) == (thisThreadId << 32);
				}(), "Cannot unlock a mutex lock that is not currently owned by the calling thread");
			gk_assertm((current & THREAD_LOCK_COUNT_BITMASK) > 0, "Cannot unlock a mutex lock that is not locked");

			uint64 nestedCount = (current & THREAD_LOCK_COUNT_BITMASK);
			if (nestedCount == 1) {
				_lockState.store(IS_NOT_OWNED, std::memory_order_release); // no more nest
			}
			else {
				_lockState.store((current & THREAD_ID_BITMASK) | (nestedCount - 1), std::memory_order_release); // support nested lock
			}
		}

	private:

		mutable std::atomic<uint64> _lockState;
		T _data;

	};
}