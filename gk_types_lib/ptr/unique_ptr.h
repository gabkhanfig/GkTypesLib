#pragma once

#include "../allocator/allocator.h"

namespace gk 
{
	template<typename T>
	struct UniquePtr 
	{
		using PtrType = T;

		// require explicit initialization
		constexpr UniquePtr() = delete;
		// prevent copying
		constexpr UniquePtr(const UniquePtr&) = delete;
		constexpr UniquePtr& operator=(const UniquePtr&) = delete;

		/**
		* Takes ownership of the pointed-to object.
		* 
		* @param inPtr: The pointed-to object to take ownership of.
		*/
		constexpr UniquePtr(T* inPtr) : ptr(inPtr) {}

		/**
		* Moves the other's pointer into this one, setting the other to nullptr.
		*/
		constexpr UniquePtr(UniquePtr&& other) noexcept;

		/**
		* Moves the other's pointer into this one, setting the other to nullptr.
		*/
		constexpr UniquePtr& operator=(UniquePtr&& other) noexcept;

		/**
		* Frees this UniquePtr if it's non-null, and sets it to nullptr.
		* For UniquePtr's that use a custom allocator, use `deinit()` prior to destruction. 
		*/
		constexpr ~UniquePtr();

		/**
		* Creates a new instance of UniquePtr using the global heap allocator at runtime,
		* or just operator new at constexpr time.
		* 
		* @param args: Arguments to construct T with.
		* @return A unique instance of T.
		*/
		template<typename ...ConstructorArgs>
		static constexpr UniquePtr create(ConstructorArgs&&... args);

		/**
		* Explicitly create a nullptr UniquePtr for whatever reason.
		* 
		* @return 
		*/
		static constexpr UniquePtr null() { return UniquePtr(nullptr); }

		/**
		* Creates a new instance of UniquePtr with a specific allocator at runtime only.
		* For all allocators other than `gk::globalHeapAllocator()`, using `deinit(allocator)` to free the allocator
		* is required for non-leak destruction.
		* 
		* @param allocator: Used to allocate a new T. Must be non-null.
		* @param args: Arguments to construct T with.
		* @return A unique instance of T.
		*/
		template<typename ...ConstructorArgs>
		static UniquePtr init(IAllocator* allocator, ConstructorArgs&&... args);

		/**
		* Required to free UniquePtr's allocated `init()` using custom allocators.
		* 
		* @param allocator: Used to free the owned unique ptr. Must be non-null.
		*/
		void deinit(IAllocator* allocator);

		/**
		* @return If this UniquePtr currently owns an object, false otherwise.
		*/
		constexpr bool isValid() const { return ptr != nullptr; }

		/**
		* operator bool
		* 
		* @return If this UniquePtr currently owns an object, false otherwise.
		*/
		constexpr explicit operator bool() const { return isValid(); }

		/**
		* logical not bool
		*
		* @return False if this UniquePtr currently owns an object, true otherwise.
		*/
		constexpr bool operator!() const { return !isValid(); }

		/**
		* Indirection operator
		* 
		* @return A pointer to the object owned by this UniquePtr.
		*/
		constexpr T* operator->() { return ptr; }

		/**
		* Indirection operator
		*
		* @return A pointer to the object owned by this UniquePtr.
		*/
		constexpr const T* operator->() const { return ptr; }

		/**
		* Deference operator
		* 
		* @return A mutable reference to the object owned by this UniquePtr.
		*/
		constexpr T& operator*() { return *ptr; }

		/**
		* Deference operator
		*
		* @return An immutable reference to the object owned by this UniquePtr.
		*/
		constexpr const T& operator*() const { return *ptr; }

		/**
		* Does not relinquish ownership of the held object. Should only ever return null with a default initialized,
		* or explicitly destructed UniquePtr. Otherwise, will be a valid pointer.
		* 
		* @return A pointer to the object owned by the UniquePtr, or nullptr if no object is owned.
		*/
		constexpr T* get() { return ptr; }

		/**
		* Does not relinquish ownership of the held object. Should only ever return null with a default initialized,
		* or explicitly destructed UniquePtr. Otherwise, will be a valid pointer.
		*
		* @return A pointer to the object owned by the UniquePtr, or nullptr if no object is owned.
		*/
		constexpr const T* get() const { return ptr; }

		/**
		* Relinquishes ownership of the held object. It is the programmers responsibility to correctly free the returned pointer
		* using whatetver allocator it was allocated with.
		* Should only ever return null with a default initialized,
		* or explicitly destructed UniquePtr. Otherwise, will be a valid pointer.
		* 
		* @return A pointer to the object that was owned by this UniquePtr, or nullptr if no object was owned.
		*/
		constexpr T* release();

	private:

		constexpr void freeHeldPtr();

	private:

		T* ptr;
	};

}

template<typename T>
inline constexpr gk::UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept
	: ptr(other.ptr)
{
	other.ptr = nullptr;
}

template<typename T>
inline constexpr gk::UniquePtr<T>& gk::UniquePtr<T>::operator=(UniquePtr&& other) noexcept
{
	freeHeldPtr();
	ptr = other.ptr;
	other.ptr = nullptr;
	return *this;
}

template<typename T>
inline constexpr gk::UniquePtr<T>::~UniquePtr()
{
	freeHeldPtr();
}

template<typename T>
inline constexpr T* gk::UniquePtr<T>::release()
{
	T* out = ptr;
	ptr = nullptr;
	return out;
}

template<typename T>
inline constexpr void gk::UniquePtr<T>::freeHeldPtr()
{
	if (ptr == nullptr) return;

	if (std::is_constant_evaluated()) {
		delete ptr;
	}
	else {
		ptr->~T();
		gk::globalHeapAllocator()->freeObject(ptr);
	}
}

template<typename T>
template<typename ...ConstructorArgs>
inline constexpr gk::UniquePtr<T> gk::UniquePtr<T>::create(ConstructorArgs && ...args)
{
	if (std::is_constant_evaluated()) {
		return UniquePtr(new T(args...));
	}
	else {
		T* mem = gk::globalHeapAllocator()->mallocObject<T>().ok();
		new(mem) T(args...);
		return UniquePtr(mem);
	}
}

template<typename T>
template<typename ...ConstructorArgs>
inline gk::UniquePtr<T> gk::UniquePtr<T>::init(IAllocator* allocator, ConstructorArgs && ...args)
{
	check_ne(allocator, nullptr);

	T* mem = allocator->mallocObject<T>().ok();
	new(mem) T(args...);
	return UniquePtr(mem);
}

template<typename T>
inline void gk::UniquePtr<T>::deinit(IAllocator* allocator)
{
	check_ne(allocator, nullptr);

	allocator->freeObject<T>(ptr);
	check_eq(ptr, nullptr);
}
