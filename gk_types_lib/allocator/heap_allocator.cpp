#include "heap_allocator.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using gk::Result;
using gk::AllocError;
using gk::MemoryLayout;

Result<void*, AllocError> gk::HeapAllocator::mallocImpl(MemoryLayout layout)
{
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

void gk::HeapAllocator::freeImpl(void* buffer, MemoryLayout layout)
{
	if (layout.alignment > 8) {
		return _aligned_free(buffer);
	}
	else {
		return std::free(buffer);
	}
}
