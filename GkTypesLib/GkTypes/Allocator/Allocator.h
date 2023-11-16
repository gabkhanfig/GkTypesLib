#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"

namespace gk
{
	namespace allocator
	{
		struct Layout {
			size_t size;
			size_t alignment;
		};

		/* Interface for all runtime allocators. */
		class Allocator {
		public:

			/* Pointer is guaranteed to be correctly aligned */
			template<typename T, typename ...ConstructorArgs>
			T* newObject(ConstructorArgs... args) {
				Layout layout{ sizeof(T), alignof(T) };
				T* objectMemory = (T*)mallocImpl(layout);
				gk_assertm(objectMemory != nullptr, "Allocator failed to malloc");
				gk_assertm((size_t(objectMemory) % alignof(T)) == 0, "Allocator buffer malloc returned a pointer not aligned to the alignment requirements of the type T");
				new(objectMemory) T(args...);
				return objectMemory;
			}

			template<typename T, typename ...ConstructorArgs>
			T* newAlignedObject(size_t alignment, ConstructorArgs... args) {
				gk_assertm(alignof(T) <= alignment, "Cannot do aligned malloc with an alignment smaller than the alignment requirements of the type T");
				Layout layout{ sizeof(T), alignment };
				T* objectMemory = (T*)mallocImpl(layout);
				gk_assertm(objectMemory != nullptr, "Allocator failed to malloc");
				gk_assertm((size_t(objectMemory) % alignof(T)) == 0, "Allocator buffer malloc returned a pointer not aligned to the alignment requirements of the type T");
				new(objectMemory) T(args...);
				return objectMemory;
			}

			/* Pointer is guaranteed to be correctly aligned */
			template<typename T, typename ...ConstructorArgs>
			T* newBuffer(size_t numElements, ConstructorArgs... args) {
				Layout layout{ sizeof(T) * numElements, alignof(T) };
				T* bufferMemory = (T*)mallocImpl(layout);

				gk_assertm(bufferMemory != nullptr, "Allocator failed to malloc");
				gk_assertm((size_t(bufferMemory) % alignof(T)) == 0, "Allocator buffer malloc returned a pointer not aligned to the alignment requirements");
				if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<T>) {
					return bufferMemory;
				}
				for (size_t i = 0; i < numElements; i++) {
					new(bufferMemory + i) T(args...); // placement new construct for each instance.
				}
				return bufferMemory;
			}

			template<typename T, typename ...ConstructorArgs>
			T* newAlignedBuffer(size_t numElements, size_t byteAlignment, ConstructorArgs... args) {
				gk_assertm(alignof(T) <= byteAlignment, "Cannot do aligned malloc with an alignment smaller than the alignment requirements of the type T");
				Layout layout{ sizeof(T) * numElements, byteAlignment };
				T* bufferMemory = (T*)mallocImpl(layout);
				gk_assertm(bufferMemory != nullptr, "Allocator failed to malloc");
				gk_assertm((size_t(bufferMemory) % alignof(T)) == 0, "Allocator buffer malloc returned a pointer not aligned to the alignment requirements of the type T");
				if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<T>) {
					return bufferMemory;
				}
				for (size_t i = 0; i < numElements; i++) {
					new(bufferMemory + i) T(args...); // placement new construct for each instance.
				}
				return bufferMemory;
			}

			template<typename T>
			void freeObject(T* object) {
				if (object == nullptr) return;
				Layout layout{ sizeof(T), alignof(T) };
				object->~T();
				freeImpl((void*)object, layout);
			}

			template<typename T>
			void freeAlignedObject(T* object, size_t byteAlignment) {
				gk_assertm(alignof(T) <= byteAlignment, "Cannot do aligned malloc with an alignment smaller than the alignment requirements of the type T");
				if (object == nullptr) return;
				Layout layout{ sizeof(T), byteAlignment };

				object->~T();
				freeImpl((void*)object, layout);
			}

			template<typename T>
			void freeBuffer(T* buffer, size_t numElements) {
				gk_assertm((size_t(buffer) % alignof(T)) == 0, "Cannot free buffer that is not aligned to the alignment requirements of the type T");
				if (buffer == nullptr) return;

				for (size_t i = 0; i < numElements; i++) {
					buffer[i].~T();
				}
				Layout layout{ sizeof(T) * numElements, alignof(T) };
				freeImpl((void*)buffer, layout);
			}

			template<typename T>
			void freeAlignedBuffer(T* buffer, size_t numElements, size_t byteAlignment) {
				gk_assertm((size_t(buffer) % byteAlignment) == 0, "Cannot free buffer that is not aligned to the alignment requirements of the type T");
				if (buffer == nullptr) return;

				for (size_t i = 0; i < numElements; i++) {
					buffer[i].~T();
				}
				Layout layout(sizeof(T) * numElements, byteAlignment);
			}

		private:

			virtual void* mallocImpl(Layout layout) = 0;

			virtual void freeImpl(void* buffer, Layout layout) = 0;

		};


	}
} // namespace gk