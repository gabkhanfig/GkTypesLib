#include "allocator.h"

using gk::Result;
using gk::AllocError;
using gk::Allocator;
using gk::IAllocator;

Result<void*, AllocError> gk::Allocator::mallocImpl(MemoryLayout layout)
{
  if (inner == nullptr) {
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
  else {
		return inner->mallocImpl(layout);
  }
}

void gk::Allocator::freeImpl(void* buffer, MemoryLayout layout)
{
	if (layout.alignment > 8) {
		return _aligned_free(buffer);
	}
	else {
		return std::free(buffer);
	}
}

Allocator gk::globalHeapAllocator() {
	return Allocator();
}
