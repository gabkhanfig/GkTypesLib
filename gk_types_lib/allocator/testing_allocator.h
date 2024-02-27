#pragma once

#include "allocator.h"
#include "../hash/hashmap.h"

namespace gk {
	/// The testing allocator. Tracks memory allocations asserting the following conditions:
	/// - No memory leaks
	/// - No double frees
	/// - Reference lifetimes do not exceed the `TestingAllocator` object lifetime
	/// This allocator should be used in testing where correct memory usage needs to be verified.
	class TestingAllocator : public gk::IAllocator {
	public:

		virtual ~TestingAllocator() override;

	private:

		virtual Result<void*, AllocError> mallocImpl(usize numBytes, usize alignment) override;

		virtual void freeImpl(void* buffer, usize numBytes, usize alignment) override;

		virtual bool trackRefCount() const override { return true; }

		virtual void incrementRefCount() override;

		virtual void decrementRefCount() override;

	private:

		struct SizeAlignTracker {
			usize size;
			usize align;
		};

		HashMap<void*, SizeAlignTracker> allocTracker;
		HashMap<void*, SizeAlignTracker> freeTracker;
		usize refCount = 0;
	};
} // namespace gk

