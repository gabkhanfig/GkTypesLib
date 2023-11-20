#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Array/ArrayList.h"
#include "../GkTest.h"



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





}