#include "testing_allocator.h"
#include <sstream>

using gk::Result;
using gk::AllocError;
using gk::usize;
using gk::Option;

gk::TestingAllocator::~TestingAllocator()
{
	check_message(this->refCount == 0, "References to this TestingAllocator instance still exist. Cannot safely destroy")

	if (this->allocTracker.size() == 0) {
		return;
	}

	usize bytesNotFreed = 0;
	std::cout << "Memory leak caught! Info:\n";
	for (const auto& allocInfo : this->allocTracker) {
		std::cout << "\tAddress: 0x" << allocInfo.key << ", Size: " << allocInfo.value.size << ", Align: " << allocInfo.value.align << '\n';
		bytesNotFreed += allocInfo.value.size;
	}
	std::cout << bytesNotFreed << " bytes leaked!" << std::endl;
	for (const auto& allocInfo : this->allocTracker) {
		gk::free(allocInfo.key, allocInfo.value.size, allocInfo.value.align);
	}
	check_message(false, "Memory leak caught! See stdout for information on memory leak");
}

Result<void*, AllocError> gk::TestingAllocator::mallocImpl(usize numBytes, usize alignment)
{
	auto res = gk::malloc(numBytes, alignment);
	if (res.isOk()) {
		void* allocMem = res.okCopy();
		SizeAlignTracker allocInfo{ .size = numBytes, .align = alignment };
		this->allocTracker.insert(allocMem, allocInfo);
		gk::Option<SizeAlignTracker*> found = this->freeTracker.find(allocMem);
		if (found.isSome()) {
			this->freeTracker.erase(allocMem);
		}
	}
	return res;
}

void gk::TestingAllocator::freeImpl(void* buffer, usize numBytes, usize alignment)
{
	bool failed = false;
	{
		gk::Option<SizeAlignTracker*> found = this->freeTracker.find(buffer);
		if (found.isSome()) {
			failed = true;
			check_message(false, "Attempted to double free memory: " << buffer);
		}
		SizeAlignTracker allocInfo{ .size = numBytes, .align = alignment };
		this->freeTracker.insert(buffer, allocInfo);
	}
	if (failed == false) {
		{
			gk::Option<SizeAlignTracker*> found = this->allocTracker.find(buffer);
			if (found.none()) {
				failed = true;
				check_message(false, "Attempted to free memory that was not allocated by this allocator. Memory: " << buffer);
			}
			this->allocTracker.erase(buffer);
		}
	}
	if (!failed) {
		gk::free(buffer, numBytes, alignment);
	}
}

void gk::TestingAllocator::incrementRefCount()
{
	refCount++;
}

void gk::TestingAllocator::decrementRefCount()
{
	check_gt(refCount, 0);
	refCount--;
}

#if GK_TYPES_LIB_TEST

#include "../array/array_list.h"

using gk::TestingAllocator;
using gk::ArrayList;
using gk::AllocatorRef;

test_case("no leak") {
	TestingAllocator t;
	auto mem = t.mallocObject<int>().ok();
	t.freeObject(mem);
}

// https://github.com/doctest/doctest/blob/master/doc/markdown/testcases.md#decorators

test_case("intentional leak" * doctest::should_fail(true) * doctest::expected_failures(1)) {
	TestingAllocator t;
	(void)t.mallocObject<int>().ok();
}

test_case("intentional double free" * doctest::should_fail(true) * doctest::expected_failures(1)) {
	TestingAllocator t;
	auto mem = t.mallocObject<int>().ok();
	auto memCopy = mem;
	t.freeObject(mem);
	t.freeObject(memCopy);
}

test_case("intentional invalid lifetime" * doctest::should_fail(true) * doctest::expected_failures(2)) {
	TestingAllocator t;
	AllocatorRef* ref1 = t.mallocObject<AllocatorRef>().ok();
	std::construct_at(ref1, t.toRef());
	AllocatorRef* ref2 = t.mallocObject<AllocatorRef>().ok();
	std::construct_at(ref2, t.toRef());
	AllocatorRef* ref3 = t.mallocObject<AllocatorRef>().ok();
	std::construct_at(ref3, t.toRef());

	ref1->~AllocatorRef();
	t.freeObject(ref1);

	// Explicitly don't call destructors to simultate the lifetime still existing, and don't focus on memory leaks
	t.freeObject(ref2);
	t.freeObject(ref3);
}

test_case("in practical use") {
	TestingAllocator t;
	auto a = ArrayList<int>::init(t.toRef());
	for (int i = 0; i < 100; i++) {
		a.reserveExact(i);
	}
}

#endif