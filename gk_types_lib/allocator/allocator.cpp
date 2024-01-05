#include "allocator.h"

using gk::Result;
using gk::AllocError;
using gk::Allocator;
using gk::IAllocator;

Result<void*, AllocError> gk::malloc(usize numBytes, usize alignment)
{
	void* mem;
	if (alignment > alignof(usize)) {
		mem = _aligned_malloc(numBytes, alignment);
	}
	else {
		mem = std::malloc(numBytes);
	}
	if (mem) {
		return ResultOk<void*>(mem);
	}
	else {
		return ResultErr<AllocError>(AllocError::OutOfMemory);
	}
}

void gk::free(void* memory, usize numBytes, usize alignment)
{
	if (alignment > alignof(usize)) {
		return _aligned_free(memory);
	}
	else {
		return std::free(memory);
	}
}

Result<void*, AllocError> gk::Allocator::mallocImpl(MemoryLayout layout)
{
  if (inner == nullptr) {
		return gk::malloc(layout.size, layout.alignment);
  }
  else {
		return inner->mallocImpl(layout);
  }
}

void gk::Allocator::freeImpl(void* buffer, MemoryLayout layout)
{
	return gk::free(buffer, layout.size, layout.alignment);
}

Allocator gk::globalHeapAllocator() {
	return Allocator();
}
