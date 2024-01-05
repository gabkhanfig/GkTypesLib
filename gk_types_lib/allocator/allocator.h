#pragma once

#include "../basic_types.h"
#include <atomic>
#include <type_traits>
#include "../error/result.h"

namespace gk
{
	enum class AllocError {
		OutOfMemory
	};

	Result<void*, AllocError> malloc(usize numBytes, usize alignment = alignof(usize));

	void free(void* memory, usize numBytes, usize alignment = alignof(usize));

	struct MemoryLayout {
		size_t size;
		size_t alignment;
	};

	struct Allocator;

	class IAllocator {
		friend struct Allocator;

		virtual Result<void*, AllocError> mallocImpl(MemoryLayout layout) = 0;

		virtual void freeImpl(void* buffer, MemoryLayout layout) = 0;

		virtual IAllocator* clone() = 0;

		virtual void destroy() = 0;
	};

	struct Allocator {
	private:

		constexpr Allocator(IAllocator* inAllocator) : inner(inAllocator) {}

	public:

		/**
		* Allow for other constexpr objects to store an empty allocator.
		*/
		constexpr Allocator() : inner(nullptr) {}

		constexpr Allocator(const Allocator& other)
		{
			if (other.inner == nullptr) {
				inner = nullptr;
				return;
			}

			inner = other.inner->clone();
		}

		constexpr Allocator(Allocator&& other) noexcept
			: inner(other.inner)
		{
			other.inner = nullptr;
		}

		constexpr ~Allocator() {
			if (inner) {
				inner->destroy();
				inner = nullptr;
			}
		}

		constexpr Allocator& operator = (const Allocator& other) {
			if (inner) {
				inner->destroy();
			}
			if (other.inner) {
				inner = other.inner->clone();
			}
			else {
				inner = nullptr;
			}
			return *this;
		}

		constexpr Allocator& operator = (Allocator&& other) noexcept {
			if (inner) {
				inner->destroy();
			}
			inner = other.inner;
			other.inner = nullptr;
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
		static Allocator create(ConstructorArgs&&... args) {
			IAllocator* allocator = new AllocatorT(args...);
			return Allocator{ allocator };
		}

		///**
		//* Makes a new instance of an Allocator given a class that implements IAllocator, and some optional arguments for the class's constructor. 
		//* 
		//* @param AllocatorT: Any that implements IAllocator
		//* @param args: Variadic arguments to pass into the child IAllocator constructor
		//*/
		//template<typename AllocatorT, typename ...ConstructorArgs>
		//	//requires(std::is_base_of_v<IAllocator, AllocatorT>)
		//static Allocator makeShared(ConstructorArgs... args) {
		//	IAllocator* allocator = new AllocatorT(args...);
		//	AtomcRefCountAllocator* ref = new AtomcRefCountAllocator(allocator);
		//	Allocator out;
		//	out._allocatorRef = ref;
		//	return out;
		//}

		/**
		* Makes a clone of this Allocator, which just increments the ref count to the shared IAllocator object.
		*
		* @return A new shared owner of an IAllocator
		*/
		constexpr Allocator clone() const {
			if (inner) {
				return Allocator{ inner->clone() };
			}
			else {
				return Allocator();
			}
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocObject() {
			MemoryLayout layout{ sizeof(T), alignof(T) };
			Result<void*, AllocError> allocResult = mallocImpl(layout);
			if (allocResult.isError()) {
				return ResultErr<AllocError>(allocResult.errorCopy());
			}
			T* mem = (T*)allocResult.okCopy();
			check_message((size_t(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocAlignedObject(size_t byteAlignment) {
			MemoryLayout layout{ sizeof(T), byteAlignment };
			Result<void*, AllocError> allocResult = mallocImpl(layout);
			if (allocResult.isError()) {
				return ResultErr<AllocError>(allocResult.errorCopy());
			}
			T* mem = (T*)allocResult.okCopy();
			check_message((size_t(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocBuffer(size_t numElements) {
			MemoryLayout layout{ sizeof(T) * numElements, alignof(T) };
			Result<void*, AllocError> allocResult = mallocImpl(layout);
			if (allocResult.isError()) {
				return ResultErr<AllocError>(allocResult.errorCopy());
			}
			T* mem = (T*)allocResult.okCopy();
			check_message((size_t(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call constructor */
		template<typename T>
		Result<T*, AllocError> mallocAlignedBuffer(size_t numElements, size_t byteAlignment) {
			MemoryLayout layout{ sizeof(T) * numElements, byteAlignment };
			Result<void*, AllocError> allocResult = mallocImpl(layout);
			if (allocResult.isError()) {
				return ResultErr<AllocError>(allocResult.errorCopy());
			}
			T* mem = (T*)allocResult.okCopy();
			check_message((size_t(mem) % alignof(T)) == 0, "Allocator mallocObject returned a pointer not aligned to the alignment requirements of the type T");
			return ResultOk<T*>(mem);
		}

		/* Does not call destructor */
		template<typename T>
		void freeObject(T*& object) {
			check_message(object != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T), alignof(T) };
			freeImpl((void*)object, layout);
			object = nullptr;
		}

		/* Does not call destructor */
		template<typename T>
		void freeAlignedObject(T*& object, size_t byteAlignment) {
			check_message(object != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T), byteAlignment };
			freeImpl((void*)object, layout);
			object = nullptr;
		}

		/* Does not call destructor */
		template<typename T>
		void freeBuffer(T*& buffer, size_t numElements) {
			check_message(buffer != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T) * numElements, alignof(T) };
			freeImpl((void*)buffer, layout);
			buffer = nullptr;
		}

		/* Does not call destructor */
		template<typename T>
		void freeAlignedBuffer(T*& buffer, size_t numElements, size_t byteAlignment) {
			check_message(buffer != nullptr, "Cannot free nullptr");
			MemoryLayout layout{ sizeof(T) * numElements, byteAlignment };
			freeImpl((void*)buffer, layout);
			buffer = nullptr;
		}

		constexpr bool operator == (const Allocator& other) const {
			return inner == other.inner;
		}

	private:

		Result<void*, AllocError> mallocImpl(MemoryLayout layout);

		void freeImpl(void* buffer, MemoryLayout layout);

	private:

		IAllocator* inner;
	};

	/**
	* @return A "copy" of the global heap allocator.
	*/
	Allocator globalHeapAllocator();


} // namespace gk