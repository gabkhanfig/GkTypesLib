#include "../pch.h"
#include "../GkTest.h"
#include "../../GkTypesLib/GkTypes/Option/Option.h"
#include <vector>

struct ExampleOptionT
{
	size_t value;
};

struct ExampleComplexOptionT
{
	int* ptr;

	ExampleComplexOptionT() { ptr = new int; }
	ExampleComplexOptionT(const ExampleComplexOptionT& other) {
		ptr = new int;
		*ptr = *other.ptr;
	}
	ExampleComplexOptionT(ExampleComplexOptionT&& other) noexcept {
		ptr = other.ptr;
		other.ptr = nullptr;
	}
	~ExampleComplexOptionT() {
		if (ptr) delete ptr;
	}
};

template<>
struct gk::Option<ExampleOptionT>
{
	Option() { _value.value = 0; }
	Option(const ExampleOptionT value) : _value(value) {}
	Option(const Option<ExampleOptionT>& other) : _value(other._value) {}
	void operator = (const ExampleOptionT value) { _value = value; }
	void operator = (const Option<ExampleOptionT>& other) { _value = other._value; }

	[[nodiscard]] size_t some() const {
		gk_assertm(!none(), "Cannot get optional example option value if its none");
		return _value.value;
	}

	[[nodiscard]] bool none() const {
		return _value.value == 0;
	}

private:

	ExampleOptionT _value;

};

static_assert(sizeof(gk::Option<void*>) == sizeof(void*), "Pointer option type should only occupy the size of a pointer, due to using nullptr as the none value");
static_assert(sizeof(gk::Option<ExampleOptionT>) == sizeof(ExampleOptionT), "Custom option should occupy only as much memory as required");

namespace UnitTests
{
	TEST(Option, PointerDefaultNone) {
		gk::Option<int*> a;
		ASSERT_TRUE(a.none());
	}

	TEST(Option, PointerPassInNullptr) {
		gk::Option<int*> a = nullptr;
		ASSERT_TRUE(a.none());
	}

	TEST(Option, PointerPassValidPointerNotNone) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		ASSERT_FALSE(a.none());
	}

	TEST(Option, PointerPassValidPointerSome) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		ASSERT_EQ(a.some(), ptr);
	}

	TEST(Option, PointerCopyNone) {
		gk::Option<int*> a;
		gk::Option<int*> b = a;
		ASSERT_TRUE(b.none());
	}

	TEST(Option, PointerCopyValid) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		gk::Option<int*> b = a;
		ASSERT_FALSE(b.none());
	}

	TEST(Option, PointerCopySamePointer) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		gk::Option<int*> b = a;
		ASSERT_EQ(a.some(), b.some());
	}

	TEST(Option, PointerMoveNone) {
		gk::Option<int*> a;
		gk::Option<int*> b = std::move(a);
		ASSERT_TRUE(b.none());
	}

	TEST(Option, PointerMoveValid) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		gk::Option<int*> b = std::move(a);
		ASSERT_FALSE(b.none());
	}

	TEST(Option, PointerMoveSamePointerAsNew) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		gk::Option<int*> b = std::move(a);
		ASSERT_EQ(b.some(), ptr);
	}

#if GK_CHECK
	TEST(Option, PointerMoveInvalidFirstInDebug) {
		int* ptr = new int;
		gk::Option<int*> a = ptr;
		EXPECT_FALSE(a.none());
		gk::Option<int*> b = std::move(a);
		ASSERT_TRUE(a.none());
	}
#endif

	TEST(Option, NonPointerDefaultNone) {
		gk::Option<int> a;
		ASSERT_TRUE(a.none());
	}

	TEST(Option, NonPointerValid) {
		gk::Option<int> a = 1;
		ASSERT_FALSE(a.none());
	}

	TEST(Option, NonPointerSome) {
		gk::Option<int> a = 5;
		ASSERT_EQ(a.some(), 5);
	}

	TEST(Option, NonPointerCopyNone) {
		gk::Option<int> a;
		gk::Option<int> b = a;
		ASSERT_TRUE(b.none());
	}

	TEST(Option, NonPointerCopyValid) {
		gk::Option<int> a = 1;
		gk::Option<int> b;
		ASSERT_TRUE(b.none());
	}

	TEST(Option, NonPointerCopySome) {
		gk::Option<int> a = 5;
		gk::Option<int> b = a;
		ASSERT_EQ(b.some(), 5);
	}

	TEST(Option, NonPointerCopyEqualSome) {
		gk::Option<int> a = 5;
		gk::Option<int> b = a;
		ASSERT_EQ(a.some(), b.some());
	}

	TEST(Option, NonPointerCopyEqualSomeSanity) {
		int num = 5;
		gk::Option<int> a = num;
		gk::Option<int> b = a;
		ASSERT_EQ(a.some(), b.some());
	}

	TEST(Option, NonPointerMoveNone) {
		gk::Option<int> a;
		ASSERT_TRUE(a.none());
		gk::Option<int> b = std::move(a);
		ASSERT_TRUE(b.none());
	}

	TEST(Option, NonPointerMoveValid) {
		gk::Option<int> a = 1;
		gk::Option<int> b = std::move(a);
		ASSERT_FALSE(b.none());
	}

	TEST(Option, NonPointerMoveSome) {
		gk::Option<int> a = 5;
		gk::Option<int> b = std::move(a);
		ASSERT_FALSE(b.none());
	}

#if GK_CHECK
	TEST(Option, NonPointerMoveInvalidOtherDebug) {
		gk::Option<int> a = 5;
		EXPECT_FALSE(a.none());
		gk::Option<int> b = std::move(a);
		ASSERT_TRUE(a.none());
	}
#endif

	TEST(Option, NonPointerSomeComplexValue) {
		std::vector<int> vec = { 0, 1, 2, 3, 4, 5 };
		gk::Option<std::vector<int>> a = vec;
		std::vector<int> copy = a.some();
		for (size_t i = 0; i < 6; i++) {
			EXPECT_EQ(vec[i], copy[i]);
		}
	}

	TEST(Option, NonPointerMoveHeldComplexValue) {
		std::vector<int> vec = { 0, 1, 2, 3, 4, 5 };
		gk::Option<std::vector<int>> a = vec;
		std::vector<int> moved = std::move(a.someMove());
		for (size_t i = 0; i < 6; i++) {
			EXPECT_EQ(moved[i], i);
		}
	}	

#if GK_CHECK
	TEST(Option, NonPointerMoveHeldComplexValueInvalidateOptionDebug) {
		std::vector<int> vec = { 0, 1, 2, 3, 4, 5 };
		gk::Option<std::vector<int>> a = vec;
		ASSERT_FALSE(a.none());
		std::vector<int> moved = std::move(a.someMove());
		ASSERT_TRUE(a.none());
	}
#endif

	TEST(Option, NonPointerCopyComplexToOption) {
		ExampleComplexOptionT obj;
		*obj.ptr = 10; 
		gk::Option<ExampleComplexOptionT> a = obj;
		EXPECT_NE(obj.ptr, nullptr);
		EXPECT_EQ(*a.some().ptr, 10);
		EXPECT_NE(a.some().ptr, obj.ptr);
	}

	TEST(Option, NonPointerMoveComplexToOption) {
		ExampleComplexOptionT obj;
		*obj.ptr = 10;
		gk::Option<ExampleComplexOptionT> a = std::move(obj);
		EXPECT_EQ(obj.ptr, nullptr);
		EXPECT_EQ(*a.some().ptr, 10);
	}

}