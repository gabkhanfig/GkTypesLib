#pragma once

#include "allocator.h"

namespace gk
{
	class HeapAllocator : public IAllocator {
		virtual Result<void*, AllocError> mallocImpl(MemoryLayout layout);

		virtual void freeImpl(void* buffer, MemoryLayout layout);
	};

	static Allocator* globalHeapAllocator() {
		static Allocator global = Allocator::makeShared<HeapAllocator>();
		return &global;
	}
}