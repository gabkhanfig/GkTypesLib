#pragma once

#include "../basic_types.h"
#include <type_traits>
#include "../error/result.h"

namespace gk
{
	class IAllocator;
	class HeapAllocator;
	struct AllocatorRef;

	/**
	* @return A reference to the global heap allocator.
	*/
	HeapAllocator* globalHeapAllocator();
	
	/**
	* This is a helper function that should be used when an AllocatorRef is needed,
	* and is known to be the global heap allocator.
	* This function circumvents the virtual function call checking
	* if the allocator uses ref counting (but with optimizations on, its *possible* this will be skipped anyways).
	* 
	* @return An AllocatorRef object referencing the global heap allocator.
	*/
	AllocatorRef globalHeapAllocatorRef();

	enum class AllocError {
		OutOfMemory
	};

	/**
	*/
	Result<void*, AllocError> malloc(usize numBytes, usize alignment = alignof(usize));

	/**
	*/
	void free(void* memory, usize numBytes, usize alignment = alignof(usize));

	class IAllocator {
	public:

		/**
		* @return AllocatoRef to this allocator.
		*/
		AllocatorRef toRef();

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's malloc implementation, returning either valid memory
		* for an instance of T, or an error.
		* The Ok T* is guaranteed to be non-null, and properly aligned to the requirements of T.
		* 
		* NOTE: In order to free, call `freeObject(mem)` on the Ok pointer
		* 
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		Result<T*, AllocError> mallocObject();

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's malloc implementation, returning either valid memory
		* for an instance of T, or an error.
		* The Ok T* is guaranteed to be non-null, and properly aligned to `byteAlignment`.
		* `byteAlignment` must be a multiple of alignof(T).
		* 
		* NOTE: In order to free, call `freeAlignedObject(mem, byteAlignment)` on the Ok pointer
		*
		* @param byteAlignment: Allocation alignment.
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		Result<T*, AllocError> mallocAlignedObject(usize byteAlignment);

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's mallocImpl, returning either valid memory
		* for an array of T, or an error.
		* The Ok T* is guaranteed to be non-null, properly aligned to the requirements of T,
		* and valid up to [numElements - 1].
		* `numElements` must be greater than 0.
		* 
		* NOTE: In order to free, call `freeBuffer(mem, numElements)` on the Ok pointer
		*
		* @param numElements: Number of T to allocate for.
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		Result<T*, AllocError> mallocBuffer(usize numElements);

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's malloc implementation, returning either valid memory
		* for an array of T, or an error.
		* The Ok T* is guaranteed to be non-null, properly aligned to aligned to `byteAlignment`,
		* and valid up to [numElements - 1].
		* `numElements` must be greater than 0.
		* `byteAlignment` must be a multiple of alignof(T).
		* 
		* NOTE: In order to free, call `freeAlignedBuffer(mem, numElements, byteAlignment)` on the Ok pointer
		*
		* @param numElements: Number of T to allocate for.
		* @param byteAlignment: Allocation alignment.
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		Result<T*, AllocError> mallocAlignedBuffer(usize numElements, usize byteAlignment);

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `object` MUST have been allocated with `mallocObject()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocObject()`.
		* 
		* @param object: The memory to free. Sets the value of the variable to nullptr.
		*/
		template<typename T>
		void freeObject(T*& object);

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `object` MUST have been allocated with `mallocAlignedObject()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocAlignedObject()`.
		* `byteAlignment` must be the same alignment specified when `object` was initially allocated.
		* It is undefined behaviour for `byteAlignment` in malloc and free to not match.
		*
		* @param object: The memory to free. Sets the value of the variable to nullptr.
		* @param byteAlignment: The byte alignment of the original allocation.
		*/
		template<typename T>
		void freeAlignedObject(T*& object, usize byteAlignment);

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `buffer` MUST have been allocated with `mallocBuffer()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocBuffer()`.
		* `numElements` must match the buffer size specified in the allocation. It is undefined behaviour for `numElements` in malloc
		* and free to not match.
		*
		* @param buffer: The memory to free. Sets the value of the variable to nullptr.
		* @param numElements: Number of T in the buffer.
		*/
		template<typename T>
		void freeBuffer(T*& buffer, usize numElements);

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `buffer` MUST have been allocated with `mallocAlignedBuffer()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocAlignedBuffer()`.
		* `numElements` must match the buffer size specified in the allocation. 
		* It is undefined behaviour for `numElements` in malloc and free to not match.llocator.
		* `byteAlignment` must be the same alignment specified when `buffer` was initially allocated.
		* It is undefined behaviour for `byteAlignment` in malloc and free to not match.
		*
		* @param buffer: The memory to free. Sets the value of the variable to nullptr.
		* @param numElements: Number of T in the buffer.
		* @param byteAlignment: The byte alignment of the original allocation.
		*/
		template<typename T>
		void freeAlignedBuffer(T*& buffer, size_t numElements, usize byteAlignment);

	private:

		friend struct AllocatorRef;

		virtual Result<void*, AllocError> mallocImpl(usize numBytes, usize alignment) = 0;

		virtual void freeImpl(void* buffer, usize numBytes, usize alignment) = 0;

		virtual bool trackRefCount() const = 0;

		virtual void incrementRefCount() {}

		virtual void decrementRefCount() {}
	};
	
	class HeapAllocator : public gk::IAllocator {
		virtual Result<void*, AllocError> mallocImpl(usize numBytes, usize alignment) override;

		virtual void freeImpl(void* buffer, usize numBytes, usize alignment) override;

		virtual bool trackRefCount() const override { return false; }

	public:
		static AllocatorRef globalInstance();
	};
	
	/**
	* Stores a reference to an IAllocator object.
	* If the IAllocator supports reference counting, it will be used.
	* To conserve memory and performance, AllocatorRef internally uses tagged pointers.
	* AllocatorRef can be constructed using a pointer to an IAllocator*.
	*/
	struct AllocatorRef {
		friend class HeapAllocator;

		/**
		* Allow for other constexpr objects to store an empty allocator.
		*/
		constexpr AllocatorRef() : inner(0) {}

		AllocatorRef(IAllocator* inAllocator);

		constexpr AllocatorRef(AllocatorRef&& other) noexcept;

		constexpr AllocatorRef& operator = (AllocatorRef&& other) noexcept;

		constexpr AllocatorRef(const AllocatorRef& other);

		constexpr AllocatorRef& operator = (const AllocatorRef& other);

		constexpr ~AllocatorRef();

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's malloc implementation, returning either valid memory
		* for an instance of T, or an error.
		* The Ok T* is guaranteed to be non-null, and properly aligned to the requirements of T.
		*
		* NOTE: In order to free, call `freeObject(mem)` on the Ok pointer
		*
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		forceinline Result<T*, AllocError> mallocObject() { return getAllocatorObject()->mallocObject<T>(); }

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's malloc implementation, returning either valid memory
		* for an instance of T, or an error.
		* The Ok T* is guaranteed to be non-null, and properly aligned to `byteAlignment`.
		* `byteAlignment` must be a multiple of alignof(T).
		*
		* NOTE: In order to free, call `freeAlignedObject(mem, byteAlignment)` on the Ok pointer
		*
		* @param byteAlignment: Allocation alignment.
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		forceinline Result<T*, AllocError> mallocAlignedObject(size_t byteAlignment) { return getAllocatorObject()->mallocAlignedObject<T>(byteAlignment); }

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's mallocImpl, returning either valid memory
		* for an array of T, or an error.
		* The Ok T* is guaranteed to be non-null, properly aligned to the requirements of T,
		* and valid up to [numElements - 1].
		* `numElements` must be greater than 0.
		*
		* NOTE: In order to free, call `freeBuffer(mem, numElements)` on the Ok pointer
		*
		* @param numElements: Number of T to allocate for.
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		forceinline Result<T*, AllocError> mallocBuffer(size_t numElements) { return getAllocatorObject()->mallocBuffer<T>(numElements); }

		/**
		* Does not call constructor, nor memset's to 0.
		* Calls allocator's malloc implementation, returning either valid memory
		* for an array of T, or an error.
		* The Ok T* is guaranteed to be non-null, properly aligned to aligned to `byteAlignment`,
		* and valid up to [numElements - 1].
		* `numElements` must be greater than 0.
		* `byteAlignment` must be a multiple of alignof(T).
		*
		* NOTE: In order to free, call `freeAlignedBuffer(mem, numElements, byteAlignment)` on the Ok pointer
		*
		* @param numElements: Number of T to allocate for.
		* @param byteAlignment: Allocation alignment.
		* @return An Ok variant with an uninitialized T*, or an Error.
		*/
		template<typename T>
		forceinline Result<T*, AllocError> mallocAlignedBuffer(size_t numElements, size_t byteAlignment) { return getAllocatorObject()->mallocAlignedBuffer<T>(numElements, byteAlignment); }

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `object` MUST have been allocated with `mallocObject()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocObject()`.
		*
		* @param object: The memory to free. Sets the value of the variable to nullptr.
		*/
		template<typename T>
		void freeObject(T*& object) { return getAllocatorObject()->freeObject<T>(object); }

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `object` MUST have been allocated with `mallocAlignedObject()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocAlignedObject()`.
		* `byteAlignment` must be the same alignment specified when `object` was initially allocated.
		* It is undefined behaviour for `byteAlignment` in malloc and free to not match.
		*
		* @param object: The memory to free. Sets the value of the variable to nullptr.
		* @param byteAlignment: The byte alignment of the original allocation.
		*/
		template<typename T>
		void freeAlignedObject(T*& object, size_t byteAlignment) { return getAllocatorObject()->freeAlignedObject<T>(object, byteAlignment); }

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `buffer` MUST have been allocated with `mallocBuffer()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocBuffer()`.
		* `numElements` must match the buffer size specified in the allocation. It is undefined behaviour for `numElements` in malloc
		* and free to not match.
		*
		* @param buffer: The memory to free. Sets the value of the variable to nullptr.
		* @param numElements: Number of T in the buffer.
		*/
		template<typename T>
		void freeBuffer(T*& buffer, size_t numElements) { return getAllocatorObject()->freeBuffer<T>(buffer, numElements); }

		/**
		* Does not call destructor.
		* Calls allocator's freeImpl. `buffer` MUST have been allocated with `mallocAlignedBuffer()` on this allocator.
		* It is undefined behaviour to pass a pointer that was not allocated with `mallocAlignedBuffer()`.
		* `numElements` must match the buffer size specified in the allocation.
		* It is undefined behaviour for `numElements` in malloc and free to not match.llocator.
		* `byteAlignment` must be the same alignment specified when `buffer` was initially allocated.
		* It is undefined behaviour for `byteAlignment` in malloc and free to not match.
		*
		* @param buffer: The memory to free. Sets the value of the variable to nullptr.
		* @param numElements: Number of T in the buffer.
		* @param byteAlignment: The byte alignment of the original allocation.
		*/
		template<typename T>
		void freeAlignedBuffer(T*& buffer, size_t numElements, size_t byteAlignment) { return getAllocatorObject()->freeAlignedBuffer<T>(buffer, numElements, byteAlignment); }

		IAllocator* getAllocatorObject();

		constexpr bool operator == (const AllocatorRef& other) const {
			return inner == other.inner;
		}

		bool operator == (const IAllocator* other) const;

	private:

		AllocatorRef(usize forceInner) : inner(forceInner) {} // utility for global heap

		Result<void*, AllocError> mallocImpl(usize numBytes, usize alignment);

		void freeImpl(void* buffer, usize numBytes, usize alignment);

		void destruct();

		void constructCopy(const AllocatorRef& other);

		void assignMove(AllocatorRef&& other) noexcept;

		void assignCopy(const AllocatorRef& other);

	private:

		usize inner;

	}; // struct AllocatorRef
} // namespace gk

template<typename T>
inline gk::Result<T*, gk::AllocError> gk::IAllocator::mallocObject()
{
	Result<void*, AllocError> allocResult = this->mallocImpl(sizeof(T), alignof(T));
	if (allocResult.isError()) {
		return ResultErr<AllocError>(allocResult.error());
	}

	T* mem = (T*)allocResult.okCopy();
	check_message((usize(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");

	return ResultOk<T*>(mem);
}

template<typename T>
inline gk::Result<T*, gk::AllocError> gk::IAllocator::mallocAlignedObject(usize byteAlignment)
{
	check_message(byteAlignment % alignof(T) == 0, "byteAlignment must be a multiple of the alignment of T");

	Result<void*, AllocError> allocResult = this->mallocImpl(sizeof(T), byteAlignment);
	if (allocResult.isError()) {
		return ResultErr<AllocError>(allocResult.error());
	}

	T* mem = (T*)allocResult.okCopy();
	check_message((usize(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");

	return ResultOk<T*>(mem);
}

template<typename T>
inline gk::Result<T*, gk::AllocError> gk::IAllocator::mallocBuffer(usize numElements)
{
	check_gt(numElements, 0);

	Result<void*, AllocError> allocResult = this->mallocImpl(sizeof(T) * numElements, alignof(T));
	if (allocResult.isError()) {
		return ResultErr<AllocError>(allocResult.error());
	}

	T* mem = (T*)allocResult.okCopy();
	check_message((usize(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");

	return ResultOk<T*>(mem);
}

template<typename T>
inline gk::Result<T*, gk::AllocError> gk::IAllocator::mallocAlignedBuffer(usize numElements, usize byteAlignment)
{
	check_gt(numElements, 0);
	check_message(byteAlignment % alignof(T) == 0, "byteAlignment must be a multiple of the alignment of T");

	Result<void*, AllocError> allocResult = this->mallocImpl(sizeof(T) * numElements, byteAlignment);
	if (allocResult.isError()) {
		return ResultErr<AllocError>(allocResult.error());
	}

	T* mem = (T*)allocResult.okCopy();
	check_message((usize(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");

	return ResultOk<T*>(mem);
}

template<typename T>
inline void gk::IAllocator::freeObject(T*& object)
{
	check_message(object != nullptr, "Cannot free nullptr");

	freeImpl((void*)object, sizeof(T), alignof(T));
	object = nullptr;
}

template<typename T>
inline void gk::IAllocator::freeAlignedObject(T*& object, usize byteAlignment)
{
	check_message(object != nullptr, "Cannot free nullptr");
	check_message(byteAlignment % alignof(T) == 0, "byteAlignment must be a multiple of the alignment of T");
	check_message((usize(object) % alignof(T)) == 0, "Cannot free a pointer that is not aligned to the alignment requirements of the type T");

	freeImpl((void*)object, sizeof(T), byteAlignment);
	object = nullptr;
}

template<typename T>
inline void gk::IAllocator::freeBuffer(T*& buffer, usize numElements)
{
	check_message(buffer != nullptr, "Cannot free nullptr");
	check_gt(numElements, 0);

	freeImpl((void*)buffer, sizeof(T) * numElements, alignof(T));
	buffer = nullptr;
}

template<typename T>
inline void gk::IAllocator::freeAlignedBuffer(T*& buffer, size_t numElements, usize byteAlignment)
{
	check_message(buffer != nullptr, "Cannot free nullptr");
	check_gt(numElements, 0);
	check_message(byteAlignment % alignof(T) == 0, "byteAlignment must be a multiple of the alignment of T");
	check_message((usize(buffer) % alignof(T)) == 0, "Cannot free a pointer that is not aligned to the alignment requirements of the type T");

	freeImpl((void*)buffer, sizeof(T) * numElements, byteAlignment);
	buffer = nullptr;
}

constexpr gk::AllocatorRef::AllocatorRef(AllocatorRef&& other) noexcept
	: inner(other.inner)
{
	other.inner = 0;
}

inline constexpr gk::AllocatorRef::~AllocatorRef()
{
	if (std::is_constant_evaluated()) {
		return;
	}
	destruct();
}

inline constexpr gk::AllocatorRef& gk::AllocatorRef::operator=(AllocatorRef&& other) noexcept
{
	if (std::is_constant_evaluated()) {
		return *this;
	}
	assignMove(std::move(other));
	return *this;
}

inline constexpr gk::AllocatorRef& gk::AllocatorRef::operator=(const AllocatorRef& other)
{
	if (std::is_constant_evaluated()) {
		return *this;
	}
	assignCopy(other);
	return *this;
}

inline constexpr gk::AllocatorRef::AllocatorRef(const AllocatorRef& other)
{
	if (std::is_constant_evaluated()) {
		inner = 0;
		return;
	}
	constructCopy(other);
}