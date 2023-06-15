#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Array/DynamicArray.h"

struct darrayComplexElement
{
	int* data;
	int count;

	darrayComplexElement() {
		data = new int[1];
		data[0] = 0;
		count = 1;
	}

	darrayComplexElement(const darrayComplexElement& other) {
		data = new int[other.count];
		count = other.count;
		for (int i = 0; i < count; i++) {
			data[i] = other.data[i];
		}
	}

	darrayComplexElement(darrayComplexElement&& other) noexcept {
		data = other.data;
		count = other.count;
		other.data = nullptr;
		other.count = 0;
	}

	~darrayComplexElement() {
		if (data) delete[] data;
	}

	bool operator == (const darrayComplexElement& other) {
		if (count != other.count) {
			return false;
		}

		for (int i = 0; i < count; i++) {
			if (data[i] != other.data[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator != (const darrayComplexElement& other) {
		return !(*this == other);
	}

	void operator = (const darrayComplexElement& other) {
		if (data) delete[] data;
		data = new int[other.count];
		count = other.count;
		for (int i = 0; i < count; i++) {
			data[i] = other.data[i];
		}
	}

	void operator = (darrayComplexElement&& other) noexcept {
		if (data) delete[] data;
		data = other.data;
		count = other.count;
		other.data = nullptr;
		other.count = 0;
	}
};

namespace UnitTests {
#pragma region Class

	static_assert(sizeof(gk::darray<int>) == 16, "Size of darray with T = int is not 16 bytes");

#pragma endregion

#pragma region Default_Constructor

	TEST(DynamicArray, DefaultConstructValidPointer) {
		gk::darray<int> a;
		ASSERT_NE(a.Data(), nullptr);
	}

	TEST(DynamicArray, DefaultConstructEmpty) {
		gk::darray<int> a;
		ASSERT_EQ(a.Size(), 0);
	}

	TEST(DynamicArray, DefaultConstructCapacity) {
		gk::darray<int> a;
		ASSERT_EQ(a.Capacity(), a.DEFAULT_CAPACITY);
	}



	TEST(DynamicArray, DefaultConstructUnique) {
		gk::darray<int> a;
		gk::darray<int> b;
		ASSERT_NE(a.Data(), b.Data());
	}

#pragma endregion

#pragma region Add_Primitive() 

	TEST(DynamicArray, AddPrimitiveSingleSizeMove) {
		gk::darray<int> a;
		a.Add(25);
		ASSERT_EQ(a.Size(), 1);
	}

	TEST(DynamicArray, AddPrimitiveSingleSizeCopy) {
		gk::darray<int> a;
		int num = 25;
		a.Add(num);
		ASSERT_EQ(a.Size(), 1);
	}

	TEST(DynamicArray, AddPrimitiveSingleCapacityMove) {
		gk::darray<int> a;
		a.Add(25);
		ASSERT_EQ(a.Capacity(), 1);
	}

	TEST(DynamicArray, AddPrimitiveSingleCapacityCopy) {
		gk::darray<int> a;
		int num = 25;
		a.Add(num);
		ASSERT_EQ(a.Capacity(), 1);
	}

	TEST(DynamicArray, AddPrimitiveSingleCheckMove) {
		gk::darray<int> a;
		a.Add(25);
		ASSERT_EQ(a.Data()[0], 25);
	}

	TEST(DynamicArray, AddPrimitiveSingleCheckCopy) {
		gk::darray<int> a;
		int num = 25;
		a.Add(num);
		ASSERT_EQ(a.Data()[0], 25);
	}

#pragma endregion

#pragma region Add_Complex()

	TEST(DynamicArray, AddComplexSingleSizeMove) {
		gk::darray<darrayComplexElement> a;
		a.Add(darrayComplexElement());
		ASSERT_EQ(a.Size(), 1);
	}

	TEST(DynamicArray, AddComplexSingleSizeCopy) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		a.Add(elem);
		ASSERT_EQ(a.Size(), 1);
	}

	TEST(DynamicArray, AddComplexSingleCapacityMove) {
		gk::darray<darrayComplexElement> a;
		a.Add(darrayComplexElement());
		ASSERT_EQ(a.Capacity(), 1);
	}

	TEST(DynamicArray, AddComplexSingleCapacityCopy) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		a.Add(elem);
		ASSERT_EQ(a.Capacity(), 1);
	}

	TEST(DynamicArray, AddComplexSingleCheckMoveDefault) {
		gk::darray<darrayComplexElement> a;
		a.Add(darrayComplexElement());
		ASSERT_TRUE(a.Data()[0] == darrayComplexElement());
	}

	TEST(DynamicArray, AddComplexSingleCheckCopyDefault) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		a.Add(elem);
		ASSERT_TRUE(a.Data()[0] == darrayComplexElement());
	}

	TEST(DynamicArray, AddComplexSingleCheckMoveEdit) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		elem.data[0] = 15;
		a.Add(std::move(elem));
		darrayComplexElement elem2;
		elem2.data[0] = 15;
		ASSERT_TRUE(a.Data()[0] == elem2);
	}

	TEST(DynamicArray, AddComplexSingleCheckCopyEdit) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		elem.data[0] = 15;
		a.Add(elem);
		ASSERT_EQ(a.Data()[0].data[0], 15);
	}

#pragma endregion

#pragma region Reallocate_Primitive

	TEST(DynamicArray, ReallocatePrimitiveDifferentCapacity) {
		gk::darray<int> a;
		const unsigned int initialCapacity = a.Capacity();
		for (int i = 0; i < 20; i++) {
			a.Add(i);
		}
		ASSERT_NE(a.Capacity(), initialCapacity);
	}

	TEST(DynamicArray, ReallocatePrimitiveCheckElements) {
		gk::darray<int> a;
		for (int i = 0; i < 20; i++) {
			a.Add(i);
		}
		for (int i = 0; i < 20; i++) {
			EXPECT_EQ(a.Data()[i], i);
		}
	}

#pragma endregion

#pragma region Reallocate_Complex

	TEST(DynamicArray, ReallocateComplexDifferentCapacity) {
		gk::darray<darrayComplexElement> a;
		const unsigned int initialCapacity = a.Capacity();
		for (int i = 0; i < 20; i++) {
			a.Add(darrayComplexElement());
		}
		ASSERT_NE(a.Capacity(), initialCapacity);
	}

	TEST(DynamicArray, ReallocateComplexCheckElements) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		elem.data[0] = 25;
		for (int i = 0; i < 20; i++) {
			a.Add(elem);
		}
		for (int i = 0; i < 20; i++) {
			EXPECT_TRUE(a.Data()[i] == elem);
		}
	}

#pragma endregion

#pragma region Reserve_Primitive

	TEST(DynamicArray, ReservePrimitive) {
		gk::darray<int> a;
		a.Reserve(100);
		ASSERT_EQ(a.Capacity(), 100);
	}

	TEST(DynamicArray, ReservePrimitiveSmaller) {
		gk::darray<int> a;
		for (int i = 0; i < 100; i++) {
			a.Add(i);
		}
		a.Reserve(50);
		ASSERT_NE(a.Capacity(), 50);
	}

	TEST(DynamicArray, ReservePrimitiveSameElement) {
		gk::darray<int> a;
		a.Add(1);
		a.Reserve(1000);
		ASSERT_EQ(a[0], 1);
	}

#pragma endregion

#pragma region Reserve_Complex

	TEST(DynamicArray, ReserveComplex) {
		gk::darray<darrayComplexElement> a;
		a.Reserve(100);
		ASSERT_EQ(a.Capacity(), 100);
	}

	TEST(DynamicArray, ReserveComplexSmaller) {
		gk::darray<darrayComplexElement> a;
		for (int i = 0; i < 100; i++) {
			a.Add(darrayComplexElement());
		}
		a.Reserve(50);
		ASSERT_NE(a.Capacity(), 50);
	}

	TEST(DynamicArray, ReserveComplexSameElement) {
		gk::darray<darrayComplexElement> a;
		darrayComplexElement elem;
		elem.data[0] = 8;
		a.Add(elem);
		a.Reserve(1000);
		ASSERT_TRUE(a[0] == elem);
	}

#pragma endregion

#pragma region Copy_Construct

	TEST(DynamicArray, CopyConstructValidPointer) {
		const gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(a);
		ASSERT_NE(b.Data(), nullptr);
	}

	TEST(DynamicArray, CopyConstructEmpty) {
		const gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(a);
		ASSERT_EQ(b.Size(), 0);
	}

	TEST(DynamicArray, CopyConstructCapacity) {
		const gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(a);
		ASSERT_EQ(b.Capacity(), b.DEFAULT_CAPACITY);
	}

	TEST(DynamicArray, CopyConstructUnique) {
		const gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(a);
		ASSERT_NE(a.Data(), b.Data());
	}

	TEST(DynamicArray, CopyConstructSinglePrimitiveElement) {
		gk::darray<int> a;
		a.Add(1);
		gk::darray<int> b = gk::darray<int>(a);
		ASSERT_EQ(b[0], 1);
	}

	TEST(DynamicArray, CopyConstructMultiplePrimitiveElement) {
		gk::darray<int> a;
		a.Add(1);
		a.Add(1);
		gk::darray<int> b = gk::darray<int>(a);
		ASSERT_EQ(b[0], 1);
		ASSERT_EQ(b[1], 1);
	}

#pragma endregion

#pragma region Move_Construct

	TEST(DynamicArray, MoveConstructValidPointer) {
		gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_NE(b.Data(), nullptr);
	}

	TEST(DynamicArray, MoveConstructEmpty) {
		gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_EQ(b.Size(), 0);
	}

	TEST(DynamicArray, MoveConstructCapacity) {
		gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_EQ(b.Capacity(), b.DEFAULT_CAPACITY);
	}

	TEST(DynamicArray, MoveConstructInvalidFirst) {
		gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_EQ(a.Data(), nullptr);
	}

	TEST(DynamicArray, MoveConstructValidSecond) {
		gk::darray<int> a;
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_NE(b.Data(), nullptr);
	}

	TEST(DynamicArray, MoveConstructSinglePrimitiveElement) {
		gk::darray<int> a;
		a.Add(1);
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_EQ(b[0], 1);
	}

	TEST(DynamicArray, MoveConstructMultiplePrimitiveElement) {
		gk::darray<int> a;
		a.Add(1);
		a.Add(1);
		gk::darray<int> b = gk::darray<int>(std::move(a));
		ASSERT_EQ(b[0], 1);
		ASSERT_EQ(b[1], 1);
	}

#pragma endregion

#pragma region Index

	TEST(DynamicArray, IndexCheckElement) {
		gk::darray<int> a;
		a.Add(10);
		ASSERT_EQ(a[0], 10);
	}

	TEST(DynamicArray, IndexCheckMultipleElements) {
		gk::darray<int> a;
		for (int i = 0; i < 100; i++) {
			a.Add(i);
			EXPECT_EQ(a[i], i);
		}

		for (int i = 0; i < 100; i++) {
			EXPECT_EQ(a[i], i);
		}
	}

	TEST(DynamicArray, IndexCheckComplexElement) {
		gk::darray<darrayComplexElement> a;
		a.Add(darrayComplexElement());
		ASSERT_TRUE(a[0] == darrayComplexElement());
	}

	TEST(DynamicArray, IndexCheckMultipleElementsComplex) {
		gk::darray<darrayComplexElement> a;
		for (int i = 0; i < 100; i++) {
			a.Add(darrayComplexElement());
			EXPECT_TRUE(a[i] == darrayComplexElement());
		}

		for (int i = 0; i < 100; i++) {
			EXPECT_TRUE(a[i] == darrayComplexElement());
		}
	}

#pragma endregion

#pragma region Contains

	TEST(DynamicArray, ContainsFirstIndex) {
		gk::darray<int> a;
		a.Add(10);
		ASSERT_TRUE(a.Contains(10));
	}

	TEST(DynamicArray, ContainsSecondIndex) {
		gk::darray<int> a;
		a.Add(20);
		a.Add(15);
		ASSERT_TRUE(a.Contains(15));
	}

	TEST(DynamicArray, DoesntContain) {
		gk::darray<int> a;
		a.Add(20);
		a.Add(15);
		ASSERT_FALSE(a.Contains(10));
	}

	TEST(DynamicArray, ContainsComplexElement) {
		gk::darray<darrayComplexElement> a;
		a.Add(darrayComplexElement());
		darrayComplexElement modified = darrayComplexElement();
		modified.data[0] = 15;
		a.Add(modified);
		ASSERT_TRUE(a.Contains(modified));
	}

	TEST(DynamicArray, DoesntContainComplexElement) {
		gk::darray<darrayComplexElement> a;
		a.Add(darrayComplexElement());
		darrayComplexElement modified = darrayComplexElement();
		modified.data[0] = 15;
		a.Add(modified);
		modified.data[0] = 20;
		ASSERT_FALSE(a.Contains(modified));
	}

#pragma endregion

#pragma region Empty

	TEST(DynamicArray, EmptyNewPointer) {
		gk::darray<int> a;
		int* old = a.Data();
		a.Add(10);
		a.Add(100);
		a.Add(15);
		a.Add(1010101);
		a.Add(5);
		a.Empty();
		ASSERT_NE(old, a.Data());
	}

	TEST(DynamicArray, EmptySize) {
		gk::darray<int> a;
		a.Add(10);
		a.Empty();
		ASSERT_EQ(a.Size(), 0);
	}

	TEST(DynamicArray, EmptyCapacity) {
		gk::darray<int> a;
		a.Add(10);
		a.Add(10);
		a.Add(10);
		a.Add(10);
		a.Empty();
		ASSERT_EQ(a.Capacity(), a.DEFAULT_CAPACITY);
	}

	TEST(DynamicArray, EmptyValidPointer) {
		gk::darray<int> a;
		a.Add(11);
		a.Add(10);
		a.Empty();
		a.Data()[0] = 5;
		ASSERT_EQ(a.Data()[0], 5);
	}

#pragma endregion

#pragma region Find

	TEST(DynamicArray, FindFirst) {
		gk::darray<int> a;
		a.Add(10);
		ASSERT_EQ(a.Find(10), 0);
	}

	TEST(DynamicArray, FindOffset) {
		gk::darray<int> a;
		a.Add(10);
		a.Add(10);
		a.Add(11);
		ASSERT_EQ(a.Find(11), 2);
	}

	TEST(DynamicArray, FindNone) {
		gk::darray<int> a;
		a.Add(10);
		a.Add(11);
		ASSERT_EQ(a.Find(12), a.INDEX_NONE);
	}

	TEST(DynamicArray, FindLastFirst) {
		gk::darray<int> a;
		a.Add(10);
		ASSERT_EQ(a.FindLast(10), 0);
	}

	TEST(DynamicArray, FindLastOffset) {
		gk::darray<int> a = { 10, 10, 11 };
		ASSERT_EQ(a.FindLast(10), 1);
	}

#pragma endregion

#pragma region Iterator

	TEST(DynamicArray, IterateElements) {
		gk::darray<int> arr;
		for (int i = 0; i < 10; i++) {
			arr.Add(i);
		}

		int index = 0;
		for (auto a : arr) {
			EXPECT_EQ(a, index);
			index++;
		}
	}

#pragma endregion
}