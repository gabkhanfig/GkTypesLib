#pragma once

#include "Allocator.h"

namespace gk
{
	class HeapAllocator : public IAllocator {
		virtual Result<void*, AllocError> mallocImpl(MemoryLayout layout) {
			void* mem;
			if (layout.alignment > 8) {
				mem = _aligned_malloc(layout.size, layout.alignment);
			}
			else {
				mem = std::malloc(layout.size);
			}
			if (mem) {
				return ResultOk<void*>(mem);
			}
			else {
				return ResultErr<AllocError>(AllocError::OutOfMemory);
			}
		}

		virtual void freeImpl(void* buffer, MemoryLayout layout) {
			if (layout.alignment > 8) {
				return _aligned_free(buffer);
			}
			else {
				return std::free(buffer);
			}
		}
	};

	static Allocator* globalHeapAllocator() {
		static Allocator global = Allocator::makeShared<HeapAllocator>();
		return &global;
	}
}