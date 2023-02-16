#include <GkTypes/Array/DynamicArray.h>
#include "ConstexprTestUnitTest.h"


#if RUN_TESTS == true
#define test(test, message) \
PRAGMA_MESSAGE([Dynamic Array Unit Test]: test); \
static_assert(test(), "[Dynamic Array Unit Test]: " #test "... " message)
#else
#define test(test, message)
#endif

#pragma region Class

static_assert(sizeof(gk::darray<int>) == 16, "Size of darray with T = int is not 16 bytes");

#pragma endregion

#pragma region Default_Constructor

consteval bool DefaultConstructValidPointer() {
	gk::darray<int> a;
	return a.Data() != nullptr;
}
test(DefaultConstructValidPointer, "Default constructed darray has a null data pointer");

consteval bool DefaultConstructEmpty() {
	gk::darray<int> a;
	return a.Size() == 0;
}
test(DefaultConstructEmpty, "Default constructed darray is not empty");

consteval bool DefaultConstructCapacity() {
	gk::darray<int> a;
	return a.Capacity() == a.DEFAULT_CAPACITY;
}
test(DefaultConstructCapacity, "Default constructed darray's capacity is non-default");

consteval bool DefaultConstructUnique() {
	gk::darray<int> a;
	gk::darray<int> b;
	return a.Data() != b.Data();
}
test(DefaultConstructUnique, "Default constructed darrays share the same data pointer");

#pragma endregion

#pragma region Add_Primitive() 

consteval bool AddPrimitiveSingleSizeMove() {
	gk::darray<int> a;
	a.Add(25);
	return a.Size() == 1;
}
test(AddPrimitiveSingleSizeMove, "Adding a single primitive element by move has the wrong size");

consteval bool AddPrimitiveSingleSizeCopy() {
	gk::darray<int> a;
	int num = 25;
	a.Add(num);
	return a.Size() == 1;
}
test(AddPrimitiveSingleSizeCopy, "Adding a single primitive element by copy has the wrong size");

consteval bool AddPrimitiveSingleCapacityMove() {
	gk::darray<int> a;
	a.Add(25);
	return a.Capacity() == 1;
}
test(AddPrimitiveSingleCapacityMove, "Adding a single primitive eleement by move has a capacity not equal to 1");

consteval bool AddPrimitiveSingleCapacityCopy() {
	gk::darray<int> a;
	int num = 25;
	a.Add(num);
	return a.Capacity() == 1;
}
test(AddPrimitiveSingleCapacityCopy, "Adding a single primitive eleement by copy has a capacity not equal to 1");

consteval bool AddPrimitiveSingleCheckMove() {
	gk::darray<int> a;
	a.Add(25);
	return a.Data()[0] == 25;
}
test(AddPrimitiveSingleCheckMove, "Adding a single primitive element by move does not have the correct element at index 0");

consteval bool AddPrimitiveSingleCheckCopy() {
	gk::darray<int> a;
	int num = 25;
	a.Add(num);
	return a.Data()[0] == 25;
}
test(AddPrimitiveSingleCheckCopy, "Adding a single primitive element by move does not have the correct element at index 0");

#pragma endregion

#pragma region Add_Complex()

consteval bool AddComplexSingleSizeMove() {
	gk::darray<darrayComplexElement> a;
	a.Add(darrayComplexElement());
	return a.Size() == 1;
}
test(AddComplexSingleSizeMove, "Adding a single complex element by move has the wrong size");

consteval bool AddComplexSingleSizeCopy() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	a.Add(elem);
	return a.Size() == 1;
}
test(AddComplexSingleSizeCopy, "Adding a single complex element by copy has the wrong size");

consteval bool AddComplexSingleCapacityMove() {
	gk::darray<darrayComplexElement> a;
	a.Add(darrayComplexElement());
	return a.Capacity() == 1;
}
test(AddComplexSingleCapacityMove, "Adding a single complex eleement by move has a capacity not equal to 1");

consteval bool AddComplexSingleCapacityCopy() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	a.Add(elem);
	return a.Capacity() == 1;
}
test(AddComplexSingleCapacityCopy, "Adding a single complex eleement by copy has a capacity not equal to 1");

consteval bool AddComplexSingleCheckMoveDefault() {
	gk::darray<darrayComplexElement> a;
	a.Add(darrayComplexElement());
	return a.Data()[0] == darrayComplexElement();
}
test(AddComplexSingleCheckMoveDefault, "Adding a single default constructed complex element by move does not have the correct element at index 0");

consteval bool AddComplexSingleCheckCopyDefault() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	a.Add(elem);
	return a.Data()[0] == darrayComplexElement();
}
test(AddComplexSingleCheckCopyDefault, "Adding a single default constructed complex element by move does not have the correct element at index 0");

consteval bool AddComplexSingleCheckMoveEdit() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	elem.data[0] = 15;
	a.Add(std::move(elem));
	darrayComplexElement elem2;
	elem2.data[0] = 15;
	return a.Data()[0] == elem2;
}
test(AddComplexSingleCheckMoveEdit, "Adding a single edited complex element by move does not have the correct element at index 0");

consteval bool AddComplexSingleCheckCopyEdit() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	elem.data[0] = 15;
	a.Add(elem);
	return a.Data()[0].data[0] == 15;
}
test(AddComplexSingleCheckCopyEdit, "Adding a single edited constructed complex element by move does not have the correct element at index 0");

#pragma endregion

#pragma region Reallocate_Primitive

consteval bool ReallocatePrimitiveDifferentCapacity() {
	gk::darray<int> a;
	const unsigned int initialCapacity = a.Capacity();
	for (int i = 0; i < 20; i++) {
		a.Add(i);
	}
	return a.Capacity() != initialCapacity;
}
test(ReallocatePrimitiveDifferentCapacity, "Darray has the same capacity after reallocation with a primitive type");

consteval bool ReallocatePrimitiveCheckElements() {
	gk::darray<int> a;
	for (int i = 0; i < 20; i++) {
		a.Add(i);
	}
	for (int i = 0; i < 20; i++) {
		if (a.Data()[i] != i) return false;
	}
	return true;
}
test(ReallocatePrimitiveCheckElements, "Darray does not have the same primitive elements upon reallocation");

#pragma endregion

#pragma region Reallocate_Complex

consteval bool ReallocateComplexDifferentCapacity() {
	gk::darray<darrayComplexElement> a;
	const unsigned int initialCapacity = a.Capacity();
	for (int i = 0; i < 20; i++) {
		a.Add(darrayComplexElement());
	}
	return a.Capacity() != initialCapacity;
}
test(ReallocateComplexDifferentCapacity, "Darray has the same capacity after reallocation with a complex type");

consteval bool ReallocateComplexCheckElements() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	elem.data[0] = 25;
	for (int i = 0; i < 20; i++) {
		a.Add(elem);
	}
	for (int i = 0; i < 20; i++) {
		if (a.Data()[i] != elem) return false;
	}
	return true;
}
test(ReallocateComplexCheckElements, "Darray does not have the same complex elements upon reallocation");

#pragma endregion

#pragma region Reserve_Primitive

consteval bool ReservePrimitive() {
	gk::darray<int> a;
	a.Reserve(100);
	return a.Capacity() == 100;
}
test(ReservePrimitive, "Reserving capacity in the darray does not set the correct capacity using primitive T");

consteval bool ReservePrimitiveSmaller() {
	gk::darray<int> a; 
	for (int i = 0; i < 100; i++) {
		a.Add(i);
	}
	a.Reserve(50);
	return a.Capacity() != 50;
}
test(ReservePrimitiveSmaller, "Reserving capacity smaller than it's current capacity is actually setting the capacity smaller using primitive T");

consteval bool ReservePrimitiveSameElement() {
	gk::darray<int> a;
	a.Add(1);
	a.Reserve(1000);
	return a[0] == 1;
}
test(ReservePrimitiveSameElement, "Reserving capacity in darray does not retain the same element using primitive T");

#pragma endregion

#pragma region Reserve_Complex

consteval bool ReserveComplex() {
	gk::darray<darrayComplexElement> a;
	a.Reserve(100);
	return a.Capacity() == 100;
}
test(ReserveComplex, "Reserving capacity in the darray does not set the correct capacity using complex T");

consteval bool ReserveComplexSmaller() {
	gk::darray<darrayComplexElement> a;
	for (int i = 0; i < 100; i++) {
		a.Add(darrayComplexElement());
	}
	a.Reserve(50);
	return a.Capacity() != 50;
}
test(ReserveComplexSmaller, "Reserving capacity smaller than it's current capacity is actually setting the capacity smaller using complex T");

consteval bool ReserveComplexSameElement() {
	gk::darray<darrayComplexElement> a;
	darrayComplexElement elem;
	elem.data[0] = 8;
	a.Add(elem);
	a.Reserve(1000);
	return a[0] == elem;
}
test(ReserveComplexSameElement, "Reserving capacity in darray does not retain the same element using complex T");

#pragma endregion

#pragma region Copy_Construct

consteval bool CopyConstructValidPointer() {
	const gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(a);
	return b.Data() != nullptr;
}
test(CopyConstructValidPointer, "Copy constructed darray has a null data pointer");

consteval bool CopyConstructEmpty() {
	const gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(a);
	return b.Size() == 0;
}
test(CopyConstructEmpty, "Copy constructed darray is not empty");

consteval bool CopyConstructCapacity() {
	const gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(a);
	return b.Capacity() == b.DEFAULT_CAPACITY;
}
test(CopyConstructCapacity, "Copy constructed darray's capacity is non-default");

consteval bool CopyConstructUnique() {
	const gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(a);
	return a.Data() != b.Data();
}
test(CopyConstructUnique, "Copy constructed darrays share the same data pointer");

consteval bool CopyConstructSinglePrimitiveElement() {
	gk::darray<int> a;
	a.Add(1);
	gk::darray<int> b = gk::darray<int>(a);
	return b[0] == 1;
}
test(CopyConstructSinglePrimitiveElement, "Copy constructed darray does not share the same single primitive element");

consteval bool CopyConstructMultiplePrimitiveElement() {
	gk::darray<int> a;
	a.Add(1);
	a.Add(1);
	gk::darray<int> b = gk::darray<int>(a);
	return b[0] == 1 && b[1] == 1;
}
test(CopyConstructMultiplePrimitiveElement, "Copy constructed darray does not share the same multiple primitive elements");

#pragma endregion

#pragma region Move_Construct

consteval bool MoveConstructValidPointer() {
	gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return b.Data() != nullptr;
}
test(MoveConstructValidPointer, "Move constructed darray has a null data pointer");

consteval bool MoveConstructEmpty() {
	gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return b.Size() == 0;
}
test(MoveConstructEmpty, "Move constructed darray is not empty");

consteval bool MoveConstructCapacity() {
	gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return b.Capacity() == b.DEFAULT_CAPACITY;
}
test(MoveConstructCapacity, "Move constructed darray's capacity is non-default");

consteval bool MoveConstructInvalidFirst() {
	gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return a.Data() == nullptr;
}
test(MoveConstructInvalidFirst, "Moved darray caused by move construction has a non-null original data pointer");

consteval bool MoveConstructValidSecond() {
	gk::darray<int> a;
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return b.Data() != nullptr;
}
test(MoveConstructValidSecond, "Move constructed darray has an null data pointer");

consteval bool MoveConstructSinglePrimitiveElement() {
	gk::darray<int> a;
	a.Add(1);
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return b[0] == 1;
}
test(MoveConstructSinglePrimitiveElement, "Move constructed darray does not share the same single primitive element");

consteval bool MoveConstructMultiplePrimitiveElement() {
	gk::darray<int> a;
	a.Add(1);
	a.Add(1);
	gk::darray<int> b = gk::darray<int>(std::move(a));
	return b[0] == 1 && b[1] == 1;
}
test(MoveConstructMultiplePrimitiveElement, "Move constructed darray does not share the same multiple primitive elements");

#pragma endregion

#pragma region Reserve

consteval bool ReserveHigherCapacity() {
	gk::darray<int> a;
	a.Reserve(100);
	return a.Capacity() == 100;
}
test(ReserveHigherCapacity, "Reserving a higher capacity than the current in the darray does not set the capacity to the correct value");

consteval bool ReserveLowerCapacity() {
	gk::darray<int> a;
	for (int i = 0; i < 100; i++) {
		a.Add(0);
	}

	const ArrSizeT cap = a.Capacity();
	a.Reserve(50);
	return a.Capacity() == cap;
}
test(ReserveLowerCapacity, "Reserving a lower capacity is unintendedly shrinking the array");

consteval bool ReserveTestElement() {
	gk::darray<int> a;
	a.Reserve(100);
	a.Data()[99] = 1;
	return a.Data()[99] == 1;
}
test(ReserveTestElement, "Cannot assign value to reserved darray index");

#pragma endregion

#pragma region Index

consteval bool IndexCheckElement() {
	gk::darray<int> a;
	a.Add(10);
	return a[0] == 10;
}
test(IndexCheckElement, "Index 0 of darray is not correct");

consteval bool IndexCheckMultipleElements() {
	gk::darray<int> a;
	for (int i = 0; i < 100; i++) {
		a.Add(i);
		if (a[i] != i) {
			return false;
		}
	}

	for (int i = 0; i < 100; i++) {
		if (a[i] != i) {
			return false;
		}
	}
	return true;
}
test(IndexCheckMultipleElements, "Darray indices do not retain their values");

consteval bool IndexCheckComplexElement() {
	gk::darray<darrayComplexElement> a;
	a.Add(darrayComplexElement());
	return a[0] == darrayComplexElement();
}
test(IndexCheckComplexElement, "Index 0 of darray using complex element is not correct");

consteval bool IndexCheckMultipleElementsComplex() {
	gk::darray<darrayComplexElement> a;
	for (int i = 0; i < 100; i++) {
		a.Add(darrayComplexElement());
		if (a[i] != darrayComplexElement()) {
			return false;
		}
	}

	for (int i = 0; i < 100; i++) {
		if (a[i] != darrayComplexElement()) {
			return false;
		}
	}
	return true;
}
test(IndexCheckMultipleElementsComplex, "Darray indices with T as a complex type do not retain their values");

#pragma endregion

#pragma region Contains

consteval bool ContainsFirstIndex() {
	gk::darray<int> a;
	a.Add(10);
	return a.Contains(10);
}
test(ContainsFirstIndex, "Darray should contain element at first index");

consteval bool ContainsSecondIndex() {
	gk::darray<int> a;
	a.Add(20);
	a.Add(15);
	return a.Contains(15);
}
test(ContainsSecondIndex, "Darray should contain element at second index");

consteval bool DoesntContain() {
	gk::darray<int> a;
	a.Add(20);
	a.Add(15);
	return !a.Contains(10);
}
test(DoesntContain, "Darray is not supposed to contain element");

consteval bool ContainsComplexElement() {
	gk::darray<darrayComplexElement> a;
	a.Add(darrayComplexElement());
	darrayComplexElement modified = darrayComplexElement();
	modified.data[0] = 15;
	a.Add(modified);
	return a.Contains(modified);
}
test(ContainsComplexElement, "Darray should contain complex element");

consteval bool DoesntContainComplexElement() {
	gk::darray<darrayComplexElement> a;
	a.Add(darrayComplexElement());
	darrayComplexElement modified = darrayComplexElement();
	modified.data[0] = 15;
	a.Add(modified);
	modified.data[0] = 20;
	return !a.Contains(modified);
}
test(DoesntContainComplexElement, "Darray shouldn't contain modified complex element");

#pragma endregion

#pragma region Empty

consteval bool EmptyNewPointer() {
	gk::darray<int> a;
	int* old = a.Data();
	a.Add(10);
	a.Add(100);
	a.Add(15);
	a.Add(1010101);
	a.Add(5);
	a.Empty();
	return old != a.Data();
}
test(EmptyNewPointer, "Emptied darray should use a new pointer");

consteval bool EmptySize() {
	gk::darray<int> a;
	a.Add(10);
	a.Empty();
	return a.Size() == 0;
}
test(EmptySize, "Emptied darray should have a size of 0");

consteval bool EmptyCapacity() {
	gk::darray<int> a;
	a.Add(10);
	a.Add(10);
	a.Add(10);
	a.Add(10);
	a.Empty();
	return a.Capacity() == a.DEFAULT_CAPACITY;
}
test(EmptyCapacity, "Emptied darray should have it's capacity set to the default");

consteval bool EmptyValidPointer() {
	gk::darray<int> a;
	a.Add(11);
	a.Add(10);
	a.Empty();
	a.Data()[0] = 5;
	return a.Data()[0] == 5;
}
test(EmptyValidPointer, "Emptied darray has a valid data pointer");

#pragma endregion

#pragma region Find

consteval bool FindFirst() {
	gk::darray<int> a;
	a.Add(10);
	return a.Find(10) == 0;
}
test(FindFirst, "Could not find correct element in darray at first index");

consteval bool FindOffset() {
	gk::darray<int> a;
	a.Add(10);
	a.Add(10);
	a.Add(11);
	return a.Find(11) == 2;
}
test(FindOffset, "Could not find correct element in the darray at an offset index");

consteval bool FindNone() {
	gk::darray<int> a;
	a.Add(10);
	a.Add(11);
	return a.Find(12) == a.INDEX_NONE;
}
test(FindNone, "Darray found an element that doesn't exist");

consteval bool FindLastFirst() {
	gk::darray<int> a;
	a.Add(10);
	return a.FindLast(10) == 0;
}
test(FindLastFirst, "Darray could not find the correct element starting from the back");

consteval bool FindLastOffset() {
	gk::darray<int> a = { 10, 10, 11 };
	return a.FindLast(10) == 1;
}
test(FindLastOffset, "Darray could not find the correct element starting from the back");

#pragma endregion

#pragma region Iterator

consteval bool IterateElements() {
	gk::darray<int> arr;
	for (int i = 0; i < 10; i++) {
		arr.Add(i);
	}

	int index = 0;
	for (auto a : arr) {
		if (a != index) return false;
		index++;
	}
	return true;
}
test(IterateElements, "Darray iterator does not iterate in the right order");

#pragma endregion