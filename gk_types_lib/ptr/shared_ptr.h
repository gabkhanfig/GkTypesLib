#pragma once

#include "../basic_types.h"
#include "../doctest/doctest_proxy.h"
#include "../allocator/allocator.h"
#include <atomic>
#include <limits>
#include <thread>

namespace gk
{
	/**
	* Smart pointer that has shared ownership of an object T using atomic or non-atomic ref counting.
	* Allows opt-in manual memory management, supporting custom allocators, with `init()` and `deinit()`.
	* Several SharedPtr's may share ownership of the same object.
	* The object will be destroyed and freed when either of these conditions happen:
	* 1. The last remaining SharedPtr owning the object is destroyed.
	* 2. The last remaining SharedPtr owning the object is assigned another object via operator=.
	* 3. The last remaining SharedPtr calls `deinit()`.
	* 
	* @param T: Type of object referenced.
	* @param atomic: Whether to use atomic ref counting or simple integer ref counting.
	*/
	template<typename T, bool atomic = true>
	struct SharedPtr
	{
	private:

		using RefCountT = std::conditional_t<atomic, std::atomic_size_t, usize>;

		struct Inner {
			T object;
			RefCountT refCount;
		};

	public:

		using PtrType = T;

		// require explicit initialization
		SharedPtr() = delete;

		/**
		* Make a copy of this SharedPtr, simply increasing the reference count to the object T by 1.
		*/
		SharedPtr(const SharedPtr& other);

		/**
		* Move a SharedPtr reference into this one. Does not modify ref count.
		*/
		SharedPtr(SharedPtr&& other);

		/**
		* For SharedPtr's created using `init()`, `deinit()` MUST be called prior to assignment.
		* Make a copy of this SharedPtr, simply increasing the reference count to the object T by 1.
		* Decrements the ref count of the object referenced by this SharedPtr before copying.
		* 
		*/
		SharedPtr& operator = (const SharedPtr& other) noexcept;

		/**
		* For SharedPtr's created using `init()`, `deinit()` MUST be called prior to assignment.
		* Move a SharedPtr reference into this one. Does not modify ref count.
		* Decrements the ref count of the object referenced by this SharedPtr before copying.
		*/
		SharedPtr& operator = (SharedPtr&& other) noexcept;

		/**
		* If valid, decrement the ref count of the object referenced by this SharedPtr.
		* If the ref count is 0, free the referenced object T.
		*/
		~SharedPtr();

		/**
		* Creates a new instance of SharedPtr using the global heap allocator.
		*
		* @param args: Arguments to construct T with.
		* @return A shared reference to a T with a ref count of 1.
		*/
		template<typename ...ConstructorArgs>
		static SharedPtr create(ConstructorArgs&&... args);

		/**
		* Explicitly create an invalid SharedPtr for whatever reason.
		* 
		* @return An invalid SharedPtr. Does not reference any object.
		*/
		static SharedPtr null() { return SharedPtr(nullptr); }

		/**
		* Creates a new instance of SharedPtr with a specific allocator at runtime only.
		* For all allocators other than `gk::globalHeapAllocator()`, using `deinit(allocator)` 
		* to free the allocator is required.
		*
		* @param allocator: Used to allocate a new T. Must be non-null.
		* @param args: Arguments to construct T with.
		* @return A shared reference to a T with a ref count of 1.
		*/
		template<typename ...ConstructorArgs>
		static SharedPtr init(IAllocator* allocator, ConstructorArgs&& ...args);

		/**
		* Explicitly decrement the ref count of the shared object, and free the object using `allocator` if necessary.
		* Required to free UniquePtr's allocated `init()` using custom allocators, because the destructor will fail to free.
		* For SharedPtr's created using `create()`, gk::globalHeapAllocator() must be used. 
		* Otherwise, use the same allocator used when `init(allocator)` was called. 
		* Sets this SharedPtr to nullptr.
		*
		* @param allocator: Used to free the owned unique ptr. Must be non-null and the same allocator used for initialization.
		*/
		void deinit(IAllocator* allocator);

		/**
		* Swap the referenced object of this SharedPtr with another.
		*
		* @param other: SharedPtr to swap reference ownership with.
		*/
		void swap(SharedPtr& other);

		/**
		* @return The number of references to the object, or 0 if this SharedPtr does not reference anything.
		*/
		usize refCount() const;

		/**
		* @return If two SharedPtr's reference the same object
		*/
		bool operator==(const SharedPtr& other) const { return inner == other.inner; }

		/**
		* @return If this SharedPtr currently owns a shared object, false otherwise.
		*/
		constexpr bool isValid() const { return inner != nullptr; }

		/**
		* operator bool
		*
		* @return If this SharedPtr currently references an object, false otherwise.
		*/
		constexpr explicit operator bool() const { return isValid(); }

		/**
		* logical not bool
		*
		* @return False if this SharedPtr currently references an object, true otherwise.
		*/
		constexpr bool operator!() const { return !isValid(); }

		/**
		* Indirection operator
		*
		* @return A pointer to the object references by this SharedPtr.
		*/
		constexpr T* operator->() { return &inner->object; }

		/**
		* Indirection operator
		*
		* @return A pointer to the object references by this SharedPtr.
		*/
		constexpr const T* operator->() const { return &inner->object; }

		/**
		* Deference operator
		*
		* @return A mutable reference to the object referenced by this SharedPtr.
		*/
		constexpr T& operator*() { return inner->object; }

		/**
		* Deference operator
		*
		* @return An immutable reference to the object referenced by this SharedPtr.
		*/
		constexpr const T& operator*() const { return inner->object; }

		/**
		* Does not decrement ref count of the referenced object. Should only ever return null with a default initialized,
		* or explicitly destructed SharedPtr. Otherwise, will be a valid pointer.
		*
		* @return A pointer to the object referenced by the SharedPtr, or nullptr if no object is owned.
		*/
		constexpr T* get() { return &inner->object; }

		/**
		* Does not decrement ref count of the referenced object. Should only ever return null with a default initialized,
		* or explicitly destructed SharedPtr. Otherwise, will be a valid pointer.
		*
		* @return A pointer to the object referenced by the SharedPtr, or nullptr if no object is owned.
		*/
		constexpr const T* get() const { return &inner->object; }

	private:

		SharedPtr(Inner* inInner) : inner(inInner) {}

		void incrementRefCount() const;

		void decrementRefCount(IAllocator* allocator) const;

	private:

		mutable Inner* inner;

	};
}

template<typename T, bool atomic>
inline gk::SharedPtr<T, atomic>::SharedPtr(const SharedPtr& other)
{
	if (other.inner == nullptr) {
		inner = nullptr;
		return;
	}

	other.incrementRefCount();
	inner = other.inner;
}

template<typename T, bool atomic>
inline gk::SharedPtr<T, atomic>::SharedPtr(SharedPtr&& other)
	: inner(other.inner)
{
	other.inner = nullptr;
}

template<typename T, bool atomic>
inline gk::SharedPtr<T, atomic>& gk::SharedPtr<T, atomic>::operator=(const SharedPtr& other) noexcept
{
	if (inner != nullptr) {
		decrementRefCount(gk::globalHeapAllocator());
	}
	if (other.inner == nullptr) {
		return *this;
	}

	other.incrementRefCount();
	inner = other.inner;
	return *this;
}

template<typename T, bool atomic>
inline gk::SharedPtr<T, atomic>& gk::SharedPtr<T, atomic>::operator=(SharedPtr&& other) noexcept
{
	if (inner != nullptr) {
		decrementRefCount(gk::globalHeapAllocator());
	}
	inner = other.inner;
	other.inner = nullptr;
	return *this;
}

template<typename T, bool atomic>
inline gk::SharedPtr<T, atomic>::~SharedPtr()
{
	if (inner == nullptr) {
		return;
	}

	decrementRefCount(gk::globalHeapAllocator());
}

template<typename T, bool atomic>
template<typename ...ConstructorArgs>
inline gk::SharedPtr<T, atomic> gk::SharedPtr<T, atomic>::create(ConstructorArgs && ...args)
{
	Inner* newInner = gk::globalHeapAllocator()->mallocObject<Inner>().ok();
	new (&newInner->object) T(args...);
	newInner->refCount = 1;
	return SharedPtr(newInner);
}

template<typename T, bool atomic>
inline gk::usize gk::SharedPtr<T, atomic>::refCount() const
{
	if (inner == nullptr) return 0;

	if constexpr (std::is_same_v<RefCountT, usize>) {
		return inner->refCount;
	}
	else {
		return inner->refCount.load(std::memory_order_acquire);
	}
}

template<typename T, bool atomic>
template<typename ...ConstructorArgs>
inline gk::SharedPtr<T, atomic> gk::SharedPtr<T, atomic>::init(IAllocator* allocator, ConstructorArgs&& ...args)
{
	Inner* newInner = allocator->mallocObject<Inner>().ok();
	new (&newInner->object) T(args...);
	newInner->refCount = 1;
	return SharedPtr(newInner);
}

template<typename T, bool atomic>
inline void gk::SharedPtr<T, atomic>::deinit(IAllocator* allocator)
{
	if (inner == nullptr) [[unlikely]] return;

	check_ne(allocator, nullptr);

	decrementRefCount(allocator);
	check_eq(inner, nullptr);
}

template<typename T, bool atomic>
inline void gk::SharedPtr<T, atomic>::swap(SharedPtr& other)
{
	Inner* temp = other.inner;
	other.inner = inner;
	inner = temp;
}

template<typename T, bool atomic>
inline void gk::SharedPtr<T, atomic>::incrementRefCount() const
{
	constexpr usize MAX_REF_COUNT = ~(0ULL);
	if constexpr (std::is_same_v<RefCountT, usize>) {
		check_ne(inner->refCount, MAX_REF_COUNT);
		inner->refCount++;
	}
	else {
		//std::atomic_size_t refCount;
		usize current = inner->refCount.load(std::memory_order_acquire);
		check_ne(current, MAX_REF_COUNT);
		while (!inner->refCount.compare_exchange_weak(current, current + 1, std::memory_order_release, std::memory_order_relaxed)) {
			check_ne(current, MAX_REF_COUNT);
			std::this_thread::yield();
		}

	}
}

template<typename T, bool atomic>
inline void gk::SharedPtr<T, atomic>::decrementRefCount(IAllocator* allocator) const
{
	if constexpr (std::is_same_v<RefCountT, usize>) {
		check_ne(inner->refCount, 0);
		inner->refCount--;
		if(inner->refCount == 0) {
			//std::cout << "ref count is 0, destroying\n";
			inner->~Inner();
			allocator->freeObject(inner);
			check_eq(inner, nullptr);
		}
	}
	else {
		//std::atomic_size_t refCount;
		usize current = inner->refCount.load(std::memory_order_acquire);
		check_ne(current, 0);
		while (!inner->refCount.compare_exchange_weak(current, current - 1, std::memory_order_release, std::memory_order_relaxed)) {
			check_ne(current, 0);
			std::this_thread::yield();
		}
		if ((current - 1) == 0) {
			//std::cout << "ref count is 0, destroying\n";
			inner->~Inner();
			allocator->freeObject(inner);
			check_eq(inner, nullptr);
		}
	}
}
