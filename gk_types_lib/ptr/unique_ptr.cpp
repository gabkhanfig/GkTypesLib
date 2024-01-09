#include "unique_ptr.h"
#include <string>

#if GK_TYPES_LIB_TEST

using gk::UniquePtr;
using gk::globalHeapAllocator;

static constexpr void testUniquePtrDefault() {
	auto p = UniquePtr<std::string>::null();
	check_not(p.isValid());
}

test_case("default construct") { testUniquePtrDefault(); }
comptime_test_case(default_construct, { testUniquePtrDefault(); });

static constexpr void testUniquePtrConstructFromPointer() {
	std::string* str = new std::string("hello world!");
	auto p = UniquePtr<std::string>(str);
	check_eq(*p, "hello world!");
	check_eq(p.get(), str);
}

test_case("construct from pointer") { testUniquePtrConstructFromPointer(); }
comptime_test_case(construct_from_pointer, { testUniquePtrConstructFromPointer(); });

static constexpr void testUniquePtrMoveConstruct() {
	std::string* str = new std::string("hello world!");
	auto p1 = UniquePtr<std::string>(str);
	auto p2 = std::move(p1);
	check_eq(*p2, "hello world!");
	check_eq(p2.get(), str);
}

test_case("move construct") { testUniquePtrMoveConstruct(); }
comptime_test_case(move_construct, { testUniquePtrMoveConstruct(); });

static constexpr void testUniquePtrMoveAssign() {
	std::string* str = new std::string("hello world!");
	auto p1 = UniquePtr<std::string>(str);
	auto p2 = UniquePtr<std::string>::create();
	p2 = std::move(p1);
	check_eq(*p2, "hello world!");
	check_eq(p2.get(), str);
}

test_case("move assign") { testUniquePtrMoveAssign(); }
comptime_test_case(move_assign, { testUniquePtrMoveAssign(); });

static constexpr void testUniquePtrCreate() {
	auto p1 = UniquePtr<std::string>::create();
	check_eq(*p1, "");
	const auto p2 = UniquePtr<std::string>::create();
	check_eq(*p2, "");
}

test_case("create") { testUniquePtrCreate(); }
comptime_test_case(create, { testUniquePtrCreate(); });

static constexpr void testUniquePtrCreateWithArgs() {
	auto p1 = UniquePtr<std::string>::create("hello world!");
	check_eq(*p1, "hello world!");
	const auto p2 = UniquePtr<std::string>::create("hello world!");
	check_eq(*p2, "hello world!");
}

test_case("create with args") { testUniquePtrCreateWithArgs(); }
comptime_test_case(create_with_args, { testUniquePtrCreateWithArgs(); });

test_case("init deinit custom allocator") {
	auto p = UniquePtr<std::string>::init(globalHeapAllocator());
	check_eq(*p, "");
	p.deinit(globalHeapAllocator());
}

test_case("init deinit custom allocator with args") {
	auto p = UniquePtr<std::string>::init(globalHeapAllocator(), "hello world!");
	check_eq(*p, "hello world!");
	p.deinit(globalHeapAllocator());
}

static constexpr void testUniquePtrOperatorBool() {
	auto p1 = UniquePtr<std::string>::create();
	check(p1);
	auto p2 = UniquePtr<std::string>::null();
	check_not(p2);
}

test_case("operator bool") { testUniquePtrOperatorBool(); }
comptime_test_case(operator_bool, { testUniquePtrOperatorBool(); });

static constexpr void testUniquePtrLogicalNot() {
	auto p1 = UniquePtr<std::string>::create();
	check_not(!p1);
	auto p2 = UniquePtr<std::string>::null();
	check(!p2);
}

test_case("logical not") { testUniquePtrLogicalNot(); }
comptime_test_case(logical_not, { testUniquePtrLogicalNot(); });

static constexpr void testUniquePtrIndirection() {
	auto p = UniquePtr<std::string>::create();
	check_eq(p->size(), 0);
}

test_case("indirection") { testUniquePtrIndirection(); }
comptime_test_case(indirection, { testUniquePtrIndirection(); });

static constexpr void testUniquePtrConstIndirection() {
	const auto p = UniquePtr<std::string>::create();
	check_eq(p->size(), 0);
}

test_case("const indirection") { testUniquePtrConstIndirection(); }
comptime_test_case(const_indirection, { testUniquePtrConstIndirection(); });

static constexpr void testUniquePtrGet() {
	auto p1 = UniquePtr<std::string>::create();
	std::string* ptr1 = p1.get();
	check_eq(ptr1->size(), 0);

	const auto p2 = UniquePtr<std::string>::create();
	const std::string* ptr2 = p2.get();
	check_eq(ptr2->size(), 0);
}

test_case("get") { testUniquePtrGet(); }
comptime_test_case(get, { testUniquePtrGet(); });

static constexpr void testUniquePtrRelease() {
	auto p = UniquePtr<std::string>::create("hello world!");
	check(p.isValid());

	std::string* ptr = p.release();
	check_not(p.isValid());

	check_eq(*ptr, "hello world!");

	delete ptr;
}

test_case("release") { testUniquePtrRelease(); }
comptime_test_case(release, { testUniquePtrRelease(); });

static constexpr void testUniquePtrSwap() {
	auto p1 = UniquePtr<std::string>::create("hello world!");
	auto p2 = UniquePtr<std::string>::create("goodbye world!");

	p1.swap(p2);
	check_eq(*p1, "goodbye world!");
	check_eq(*p2, "hello world!");
}
test_case("swap") { testUniquePtrSwap(); }
comptime_test_case(swap, { testUniquePtrSwap(); });

static constexpr void testUniquePtrEqual() {
	auto p = UniquePtr<std::string>::create();
	std::string* ptr = p.get();
	check_eq(p, ptr);
}
test_case("equal") { testUniquePtrEqual(); }
comptime_test_case(equal, { testUniquePtrEqual(); });

#endif