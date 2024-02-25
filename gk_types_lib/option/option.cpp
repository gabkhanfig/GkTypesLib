#include "option.h"

#if GK_TYPES_LIB_TEST
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
		check_message(!none(), "Cannot get optional example option value if its none");
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

test_case("PointerDefaultNone") {
	gk::Option<int*> a;
	check(a.none());
}

test_case("PointerPassInNullptr") {
	gk::Option<int*> a = nullptr;
	check(a.none());
}

test_case("PointerPassValidPointerNotNone") {
	int* ptr = new int;
	gk::Option<int*> a = ptr;
	check_not(a.none());
}

test_case("PointerPassValidPointerSome") {
	int* ptr = new int;
	gk::Option<int*> a = ptr;
	check_eq(a.some(), ptr);
}

test_case("PointerMoveNone") {
	gk::Option<int*> a;
	gk::Option<int*> b = std::move(a);
	check(b.none());
}

test_case("PointerMoveValid") {
	int* ptr = new int;
	gk::Option<int*> a = ptr;
	gk::Option<int*> b = std::move(a);
	check_not(b.none());
}

test_case("PointerMoveSamePointerAsNew") {
	int* ptr = new int;
	gk::Option<int*> a = ptr;
	gk::Option<int*> b = std::move(a);
	check_eq(b.some(), ptr);
}

test_case("PointerMove") {
	int* ptr = new int;
	gk::Option<int*> a = ptr;
	check_not(a.none());
	gk::Option<int*> b = std::move(a);
}

test_case("NonPointerDefaultNone") {
	gk::Option<int> a;
	check(a.none());
}

test_case("NonPointerValid") {
	gk::Option<int> a = 1;
	check_not(a.none());
}

test_case("NonPointerSome") {
	gk::Option<int> a = 5;
	check_eq(a.some(), 5);
}

test_case("NonPointerMoveNone") {
	gk::Option<int> a;
	check(a.none());
	gk::Option<int> b = std::move(a);
	check(b.none());
}

test_case("NonPointerMoveValid") {
	gk::Option<int> a = 1;
	gk::Option<int> b = std::move(a);
	check_not(b.none());
}

test_case("NonPointerMoveSome") {
	gk::Option<int> a = 5;
	gk::Option<int> b = std::move(a);
	check_not(b.none());
}

test_case("NonPointerSomeComplexValue") {
	std::vector<int> vec = { 0, 1, 2, 3, 4, 5 };
	gk::Option<std::vector<int>> a = vec;
	std::vector<int> copy = a.some();
	for (size_t i = 0; i < 6; i++) {
		check_eq(vec[i], copy[i]);
	}
}

test_case("NonPointerMoveHeldComplexValue") {
	std::vector<int> vec = { 0, 1, 2, 3, 4, 5 };
	gk::Option<std::vector<int>> a = vec;
	std::vector<int> moved = a.some();
	for (size_t i = 0; i < 6; i++) {
		check_eq(moved[i], i);
	}
}

test_case("NonPointerCopyComplexToOption") {
	ExampleComplexOptionT obj;
	*obj.ptr = 10;
	gk::Option<ExampleComplexOptionT> a = obj;
	check_ne(obj.ptr, nullptr);
	auto moved = a.some();
	int* movedPtr = moved.ptr;
	check_eq(*movedPtr, 10);
	check_ne(movedPtr, obj.ptr);
}

test_case("NonPointerMoveComplexToOption") {
	ExampleComplexOptionT obj;
	*obj.ptr = 10;
	gk::Option<ExampleComplexOptionT> a = std::move(obj);
	check_eq(obj.ptr, nullptr);
	check_eq(*a.some().ptr, 10);
}

comptime_test_case(ref_option, {
	int num = 2;
	{
		gk::Option<int&> r = num;
		check(r.isSome());
		int& numRef = r.some();
		check(r.none());
		check_eq(numRef, 2);
	}
	{
		gk::Option<int&> r;
		check(r.none());
	}
	{
		gk::Option<int&> r;
		r = num;
		check(r.isSome());
		int& numRef = r.some();
		check(r.none());
		check_eq(numRef, 2);
	}
	{
		gk::Option<int&> r1 = num;
		gk::Option<int&> r2 = std::move(r1);
		check(r2.isSome());
		int& numRef = r2.some();
		check(r2.none());
		check_eq(numRef, 2);
	}
});

#endif