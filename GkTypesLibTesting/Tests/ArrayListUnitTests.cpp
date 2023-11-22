#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Array/ArrayList.h"
#include "../GkTest.h"
#include <string>



template<typename T>
using ArrayList = gk::ArrayList<T>;

using gk::globalHeapAllocator;
using gk::Allocator;


namespace UnitTests
{
	TEST(ArrayList, DefaultConstruct) {
		ArrayList<int> a;
		EXPECT_EQ(a.len(), 0);
		EXPECT_EQ(a.capacity(), 0);
		EXPECT_EQ(a.data(), nullptr); // ensure no allocation when not necessary
		EXPECT_EQ(a.allocator(), *globalHeapAllocator());
	}

	COMPTIME_TEST(ArrayList, DefaultConstruct, {
		ArrayList<int> a;
		comptimeAssertEq(a.len(), 0);
		comptimeAssertEq(a.capacity(), 0);
		comptimeAssertEq(a.data(), nullptr);
		comptimeAssertEq(a.allocator(), Allocator()); // compile time does not use an allocator
	});

	TEST(ArrayList, CopyConstruct) {
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = a;
		EXPECT_EQ(b[0], 1);
		EXPECT_EQ(b.len(), 1);
		EXPECT_TRUE(b.capacity() > 0);
	}

	COMPTIME_TEST(ArrayList, CopyConstruct, {
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = a;
		comptimeAssertEq(b[0], 1);
		comptimeAssertEq(b.len(), 1);
		comptimeAssert(b.capacity() > 0);
	});

	TEST(ArrayList, MoveConstruct) {
		ArrayList<int> a;
		a.push(1);
		const int* oldPtr = a.data();
		ArrayList<int> b = std::move(a);
		EXPECT_EQ(b[0], 1);
		EXPECT_EQ(b.len(), 1);
		EXPECT_TRUE(b.capacity() > 0);
		EXPECT_EQ(b.data(), oldPtr);
	}

	COMPTIME_TEST(ArrayList, MoveConstruct, {
		ArrayList<int> a;
		a.push(1);
		const int* oldPtr = a.data();
		ArrayList<int> b = std::move(a);
		comptimeAssertEq(b[0], 1);
		comptimeAssertEq(b.len(), 1);
		comptimeAssert(b.capacity() > 0);
		comptimeAssertEq(b.data(), oldPtr); // Ensure pointer is moved
	});

	TEST(ArrayList, CopyAssign) {
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = a;
		EXPECT_EQ(b[0], 1);
		EXPECT_EQ(b.len(), 1);
		EXPECT_TRUE(b.capacity() > 0);
	}

	COMPTIME_TEST(ArrayList, CopyAssign, {
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = a;
		comptimeAssertEq(b[0], 1);
		comptimeAssertEq(b.len(), 1);
		comptimeAssert(b.capacity() > 0);
	});

	TEST(ArrayList, MoveAssign) {
		ArrayList<int> a;
		a.push(1);
		const int* oldPtr = a.data();
		ArrayList<int> b;
		b.push(5);
		b = std::move(a);
		EXPECT_EQ(b[0], 1);
		EXPECT_EQ(b.len(), 1);
		EXPECT_TRUE(b.capacity() > 0);
		EXPECT_EQ(b.data(), oldPtr);
	}

	COMPTIME_TEST(ArrayList, MoveAssign, {
		ArrayList<int> a;
		a.push(1);
		const int* oldPtr = a.data();
		ArrayList<int> b;
		b.push(5);
		b	= std::move(a);
		comptimeAssertEq(b[0], 1);
		comptimeAssertEq(b.len(), 1);
		comptimeAssert(b.capacity() > 0);
		comptimeAssertEq(b.data(), oldPtr); // Ensure pointer is moved
	});

	TEST(ArrayList, InitWithAllocator) {
		ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator()->clone());
		a.push(1);
		EXPECT_EQ(a[0], 1);
	}

	TEST(ArrayList, InitWithAllocatorAndCopy) {
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = ArrayList<int>::init(globalHeapAllocator()->clone(), a);
		EXPECT_EQ(b[0], 1);
	}

	TEST(ArrayList, InitWithAllocatorAndInitializerList) {
		ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator()->clone(), {0, 1, 2});
		EXPECT_EQ(a[0], 0);
		EXPECT_EQ(a[1], 1);
		EXPECT_EQ(a[2], 2);
	}

	TEST(ArrayList, InitWithAllocatorAndPtr) {
		int buf[] = { 0, 1, 2 };
		ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator()->clone(), buf, 3);
		EXPECT_EQ(a[0], 0);
		EXPECT_EQ(a[1], 1);
		EXPECT_EQ(a[2], 2);
	}

	TEST(ArrayList, WithCapacityWithAllocator) {
		ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10);
		a.push(1);
		EXPECT_TRUE(a.capacity() >= 10);
		EXPECT_EQ(a[0], 1);
	}

	TEST(ArrayList, WithCapacityWithAllocatorAndCopy) {
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10, a);
		EXPECT_TRUE(a.capacity() >= 10);
		EXPECT_EQ(b[0], 1);
	}

	TEST(ArrayList, WithCapacityWithAllocatorAndInitializerList) {
		ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10, {0, 1, 2});
		EXPECT_TRUE(a.capacity() >= 10);
		EXPECT_EQ(a[0], 0);
		EXPECT_EQ(a[1], 1);
		EXPECT_EQ(a[2], 2);
	}

	TEST(ArrayList, WithCapacityWithAllocatorAndPtr) {
		int buf[] = { 0, 1, 2 };
		ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10, buf, 3);
		EXPECT_TRUE(a.capacity() >= 10);
		EXPECT_EQ(a[0], 0);
		EXPECT_EQ(a[1], 1);
		EXPECT_EQ(a[2], 2);
	}

	TEST(ArrayList, PushElements) {
		ArrayList<std::string> a;
		const std::string first = "hello";
		a.push(first);
		a.push("world");
		a.push("it");
		a.push("is");
		a.push("me");
		a.push("how");
		a.push("are");
		a.push("you");
		EXPECT_EQ(a[0], first);
		EXPECT_EQ(a[1], "world");
		EXPECT_EQ(a[2], "it");
		EXPECT_EQ(a[3], "is");
		EXPECT_EQ(a[4], "me");
		EXPECT_EQ(a[5], "how");
		EXPECT_EQ(a[6], "are");
		EXPECT_EQ(a[7], "you");
	}

	COMPTIME_TEST(ArrayList, PushElements, {
		ArrayList<std::string> a;
		const std::string first = "hello";
		a.push(first);
		a.push("world");
		a.push("it");
		a.push("is");
		a.push("me");
		a.push("how");
		a.push("are");
		a.push("you");
		comptimeAssertEq(a[0], first);
		comptimeAssertEq(a[1], "world");
		comptimeAssertEq(a[2], "it");
		comptimeAssertEq(a[3], "is");
		comptimeAssertEq(a[4], "me");
		comptimeAssertEq(a[5], "how");
		comptimeAssertEq(a[6], "are");
		comptimeAssertEq(a[7], "you");
	});
}