#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include <atomic>
#include "../Error/Result.h"

namespace gk
{
	struct MemoryLayout {
		size_t size;
		size_t alignment;
	};

	enum class AllocError {
		OutOfMemory
	};

	struct Allocator;

	class IAllocator {
		friend struct Allocator;

		virtual Result<void*, AllocError> mallocImpl(MemoryLayout layout) = 0;

		virtual void freeImpl(void* buffer, MemoryLayout layout) = 0;
	};

	struct Allocator {

	private:
		struct AtomcRefCountAllocator {
			IAllocator* allocator;
			std::atomic<size_t> refCount;

			AtomcRefCountAllocator(IAllocator* inAllocator)
				: allocator(inAllocator), refCount(1) {}

			AtomcRefCountAllocator(const AtomcRefCountAllocator&) = delete;
			AtomcRefCountAllocator(AtomcRefCountAllocator&&) = delete;
			AtomcRefCountAllocator& operator = (const AtomcRefCountAllocator&) = delete;
			AtomcRefCountAllocator& operator = (AtomcRefCountAllocator&&) = delete;

			~AtomcRefCountAllocator() {
				delete allocator;
			}
		};

	public:

		/**
		* Allow for other constexpr objects to store an empty allocator.
		*/
		constexpr Allocator() : _allocatorRef(nullptr) {}

		constexpr Allocator(const Allocator& other) {
			_allocatorRef = other._allocatorRef;
			if (_allocatorRef == nullptr) return;
			_allocatorRef->refCount++;
		}

		constexpr Allocator(Allocator&& other) noexcept {
			_allocatorRef = other._allocatorRef;
			other._allocatorRef = nullptr;
		}

		constexpr ~Allocator() {
			decrementCounter();
		}

		constexpr Allocator& operator = (const Allocator& other) {
			decrementCounter();
			_allocatorRef = other._allocatorRef;
			if (_allocatorRef == nullptr) return *this;
			_allocatorRef->refCount++;
			return *this;
		}

		constexpr Allocator& operator = (Allocator&& other) noexcept {
			decrementCounter();
			_allocatorRef = other._allocatorRef;
			other._allocatorRef = nullptr;
			return *this;
		}

		/**
		* Makes a new instance of an Allocator given a class that implements IAllocator, and some optional arguments for the class's constructor. 
		* 
		* @param AllocatorT: Any that implements IAllocator
		* @param args: Variadic arguments to pass into the child IAllocator constructor
		*/
		template<typename AllocatorT, typename ...ConstructorArgs>
			requires(std::is_base_of_v<IAllocator, AllocatorT>)
		static Allocator makeShared(ConstructorArgs... args) {
			IAllocator* allocator = new AllocatorT(args...);
			AtomcRefCountAllocator* ref = new AtomcRefCountAllocator(allocator);
			Allocator out;
			out._allocatorRef = ref;
			return out;
		}

		/**
		* Makes a clone of this Allocator, which just increments the ref count to the shared IAllocator object.
		*
		* @return A new shared owner of an IAllocator
		*/
		constexpr Allocator clone() const {
			return Allocator(*this);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocObject() {
			MemoryLayout layout{ sizeof(T), alignof(T) };
			T* mem = (T*)_allocatorRef->allocator->mallocImpl(layout).ok();
			gk_assertm((size_t(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocAlignedObject(size_t byteAlignment) {
			MemoryLayout layout{ sizeof(T), byteAlignment };
			T* mem = (T*)_allocatorRef->allocator->mallocImpl(layout).ok();
			gk_assertm((size_t(mem) % byteAlignment) == 0, "Allocator mallocAlignedObject returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocBuffer(size_t numElements) {
			MemoryLayout layout{ sizeof(T) * numElements, alignof(T) };
			T* mem = (T*)_allocatorRef->allocator->mallocImpl(layout).ok();
			gk_assertm((size_t(mem) % alignof(T)) == 0, "Allocator mallocBuffer returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocAlignedBuffer(size_t numElements, size_t byteAlignment) {
			MemoryLayout layout{ sizeof(T) * numElements, byteAlignment };
			T* mem = (T*)_allocatorRef->allocator->mallocImpl(layout).ok();
			gk_assertm((size_t(mem) % alignof(T)) == 0, "Allocator mallocAlignedBuffer returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call destructor */
		template<typename T>
		void freeObject(T*& object) {
			gk_assertm(object != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T), alignof(T) };
			_allocatorRef->allocator->freeImpl((void*)object, layout);
			object = nullptr;
		}

		/* Does not call destructor */
		template<typename T>
		void freeAlignedObject(T*& object, size_t byteAlignment) {
			gk_assertm(object != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T), byteAlignment };
			_allocatorRef->allocator->freeImpl((void*)object, layout);
			object = nullptr;
		}

		/* Does not call destructor */
		template<typename T>
		void freeBuffer(T*& buffer, size_t numElements) {
			gk_assertm(buffer != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T) * numElements, alignof(T) };
			_allocatorRef->allocator->freeImpl((void*)buffer, layout);
			buffer = nullptr;
		}

		/* Does not call destructor */
		template<typename T>
		void freeAlignedBuffer(T*& buffer, size_t numElements, size_t byteAlignment) {
			gk_assertm(buffer != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T) * numElements, byteAlignment };
			_allocatorRef->allocator->freeImpl((void*)buffer, layout);
			buffer = nullptr;
		}

	private:

		constexpr void decrementCounter() {
			if (_allocatorRef == nullptr) return;

			_allocatorRef->refCount--;
			if (_allocatorRef->refCount.load(std::memory_order::acquire) == 0) {
				delete _allocatorRef;
			}
		}

	private:

		AtomcRefCountAllocator* _allocatorRef;
	};
} // namespace gk