#pragma once

#include "Allocator.h"

namespace gk
{
	namespace allocator
	{
		class HeapAllocator : public Allocator {
		private:

			virtual void* mallocImpl(Layout layout) override {
				std::cout << "heap allocator malloc\n";
				if (layout.alignment > 8) {
					return _aligned_malloc(layout.size, layout.alignment);
				}
				else {
					return std::malloc(layout.size);
				}
			}

			virtual void freeImpl(void* buffer, Layout layout) override {
				std::cout << "heap allocator free\n";
				if (layout.alignment > 8) {
					return _aligned_free(buffer);
				}
				else {
					return std::free(buffer);
				}
			}
		};

		static HeapAllocator* globalHeap() {
			static HeapAllocator* globalHeapAllocator = new HeapAllocator();
			return globalHeapAllocator;
		}
	}
	
}