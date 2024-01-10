#include "shared_ptr.h"

#if GK_TYPES_LIB_TEST

using gk::SharedPtr;
using gk::globalHeapAllocator;

test_case("null") {
	auto p = SharedPtr<std::string>::null();
	check_not(p);
	check(!p);
	check_not(p.isValid());
}

test_case("create") {
	auto p = SharedPtr<std::string>::create();
	check(p);
	check_not(!p);
	check_eq(*p, "");
}

test_case("create with arguments") {
	auto p = SharedPtr<std::string>::create("hello world!");
	check(p);
	check_not(!p);
	check_eq(*p, "hello world!");
}

test_case("move construct") {
	auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = std::move(p1);
	check_eq(*p2, "hello world!");
}

test_case("copy construct") {
	auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = p1;
	check_eq(*p1, "hello world!");
	check_eq(*p2, "hello world!");
	check_eq(p1, p2);
}

test_case("move assign") {
	auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = SharedPtr<std::string>::create("goodbye world!");
	p2 = std::move(p1);
	check_eq(*p2, "hello world!");
}

test_case("copy assign") {
	auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = SharedPtr<std::string>::create("goodbye world!");
	p2 = p1;
	check_eq(*p1, "hello world!");
	check_eq(*p2, "hello world!");
	check_eq(p1, p2);
}

test_case("init deinit custom allocator") {
	auto p = SharedPtr<std::string>::init(globalHeapAllocator());
	check_eq(*p, "");
	p.deinit(globalHeapAllocator());
}

test_case("init deinit custom allocator with arguments") {
	auto p = SharedPtr<std::string>::init(globalHeapAllocator(), "hello world!");
	check_eq(*p, "hello world!");
	p.deinit(globalHeapAllocator());
}

test_case("const copy construct") {
	const auto p1 = SharedPtr<std::string>::create("hello world!");
	const auto p2 = p1;
	check_eq(*p1, "hello world!");
	check_eq(*p2, "hello world!");
	check_eq(p1, p2);
}

test_case("const copy assign") {
	const auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = SharedPtr<std::string>::create("goodbye world!");
	p2 = p1;
	check_eq(*p1, "hello world!");
	check_eq(*p2, "hello world!");
	check_eq(p1, p2);
}

test_case("indirection operator") {
	const auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = p1;
	check_eq(p1->size(), 12);
	check_eq(p2->size(), 12);
}

test_case("get ptr") {
	const auto p1 = SharedPtr<std::string>::create("hello world!");
	auto p2 = p1;
	check_eq(p1.get(), p2.get());
}

#endif