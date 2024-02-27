#include "allocator.h"

using gk::Result;
using gk::AllocError;
using gk::AllocatorRef;
using gk::IAllocator;
using gk::usize;

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

AllocatorRef gk::IAllocator::toRef()
{
	return AllocatorRef(this);
}

Result<void*, AllocError> gk::HeapAllocator::mallocImpl(usize numBytes, usize alignment)
{
	return gk::malloc(numBytes, alignment);
}

void gk::HeapAllocator::freeImpl(void* buffer, usize numBytes, usize alignment)
{
	return gk::free(buffer, numBytes, alignment);
}

constexpr usize ALLOCATOR_USE_REF_COUNT_FLAG = (1ULL << 48);

gk::AllocatorRef::AllocatorRef(IAllocator* inAllocator)
{
	check_ne(inAllocator, nullptr);
	const usize ptrAsUsize = reinterpret_cast<usize>(inAllocator);
	const usize countFlag = static_cast<usize>(inAllocator->trackRefCount()) << 48;
	inner = ptrAsUsize | countFlag;
	if (countFlag) {
		inAllocator->incrementRefCount();
	}
}

bool gk::AllocatorRef::operator==(const IAllocator* other) const
{
	constexpr usize PTR_BITMASK = (1ULL << 48) - 1;
	return reinterpret_cast<const IAllocator*>(inner & PTR_BITMASK) == other;
}

Result<void*, AllocError> gk::AllocatorRef::mallocImpl(usize numBytes, usize alignment)
{
	return getAllocatorObject()->mallocImpl(numBytes, alignment);
}

void gk::AllocatorRef::freeImpl(void* buffer, usize numBytes, usize alignment)
{
	return getAllocatorObject()->freeImpl(buffer, numBytes, alignment);
}

IAllocator* gk::AllocatorRef::getAllocatorObject()
{
	constexpr usize PTR_BITMASK = (1ULL << 48) - 1;
	return reinterpret_cast<IAllocator*>(inner & PTR_BITMASK);
}

void gk::AllocatorRef::destruct()
{
	if (inner == 0) {
		return;
	}

	if (inner & ALLOCATOR_USE_REF_COUNT_FLAG) {
		getAllocatorObject()->decrementRefCount();
	}
	inner = 0;
}

void gk::AllocatorRef::constructCopy(const AllocatorRef& other)
{
	inner = other.inner;
	if (!(inner & ALLOCATOR_USE_REF_COUNT_FLAG)) {
		return;
	}

	getAllocatorObject()->incrementRefCount();
}

void gk::AllocatorRef::assignMove(AllocatorRef&& other) noexcept
{
	if (inner & ALLOCATOR_USE_REF_COUNT_FLAG) { // if the ref is null (0), this if will not execute, thus is safe
		getAllocatorObject()->decrementRefCount();
	}

	inner = other.inner;
	other.inner = 0;
}

void gk::AllocatorRef::assignCopy(const AllocatorRef& other)
{
	if (inner & ALLOCATOR_USE_REF_COUNT_FLAG) { // if the ref is null (0), this if will not execute, thus is safe
		getAllocatorObject()->decrementRefCount();
	}

	inner = other.inner;
	if (inner & ALLOCATOR_USE_REF_COUNT_FLAG) {
		getAllocatorObject()->incrementRefCount();
	}
}

gk::HeapAllocator* gk::globalHeapAllocator() 
{
	static gk::HeapAllocator GLOBAL_HEAP_ALLOCATOR = gk::HeapAllocator();
	return &GLOBAL_HEAP_ALLOCATOR;
}

AllocatorRef gk::globalHeapAllocatorRef()
{
	return gk::HeapAllocator::globalInstance();
}

AllocatorRef gk::HeapAllocator::globalInstance()
{
	static const usize GLOBAL_HEAP_ALLOCATOR_REF = reinterpret_cast<usize>(gk::globalHeapAllocator());
	return AllocatorRef{ GLOBAL_HEAP_ALLOCATOR_REF };
}
