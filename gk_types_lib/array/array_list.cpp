#include "array_list.h"

#if GK_TYPES_LIB_TEST
#include <string>

template<typename T>
using ArrayList = gk::ArrayList<T>;

using gk::globalHeapAllocator;
using gk::Allocator;

test_case("Default Construct") {
	ArrayList<int> a;
	check_eq(a.len(), 0);
	check_eq(a.capacity(), 0);
	check_eq(a.data(), nullptr); // ensure no allocation when not necessary
	check_eq(a.allocator(), *globalHeapAllocator());
}

test_case("Copy Construct") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = a;
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
}

test_case("Move Construct") {
	ArrayList<int> a;
	a.push(1);
	const int* oldPtr = a.data();
	ArrayList<int> b = std::move(a);
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
	check_eq(b.data(), oldPtr);
}

test_case("Copy Assign") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = a;
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
}

test_case("Move Assign") {
	ArrayList<int> a;
	a.push(1);
	const int* oldPtr = a.data();
	ArrayList<int> b;
	b.push(5);
	b = std::move(a);
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
	check_eq(b.data(), oldPtr);
}

test_case("Init") {
	ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator()->clone());
	a.push(1);
	check_eq(a[0], 1);
}

test_case("Init And Copy") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = ArrayList<int>::init(globalHeapAllocator()->clone(), a);
	check_eq(b[0], 1);
}

test_case("Init And Initialize List") {
	ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator()->clone(), { 0, 1, 2 });
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("Init And Ptr") {
	int buf[] = { 0, 1, 2 };
	ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator()->clone(), buf, 3);
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("With Capacity") {
	ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10);
	a.push(1);
	check(a.capacity() >= 10);
	check_eq(a[0], 1);
}

test_case("With Capacity Copy") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10, a);
	check(a.capacity() >= 10);
	check_eq(b[0], 1);
}

test_case("With Capacity Initializer List") {
	ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10, { 0, 1, 2 });
	check(a.capacity() >= 10);
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("With Capacity Ptr") {
	int buf[] = { 0, 1, 2 };
	ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator()->clone(), 10, buf, 3);
	check(a.capacity() >= 10);
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("Push std::string's") {
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
	check_eq(a[0], first);
	check_eq(a[1], "world");
	check_eq(a[2], "it");
	check_eq(a[3], "is");
	check_eq(a[4], "me");
	check_eq(a[5], "how");
	check_eq(a[6], "are");
	check_eq(a[7], "you");
}

test_case("Reserve") {
	ArrayList<int> a;
	a.push(0);
	a.reserve(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

test_case("Reserve From Empty") {
	ArrayList<int> a;
	a.reserve(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 0);
}

test_case("Reserve Zero") {
	ArrayList<int> a;
	a.push(0);
	a.reserve(0);
	check(a.capacity() > 0);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

test_case("Reserve Tiny") {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	a.reserve(1);
	check(a.capacity() >= 10);
	check_eq(a.len(), 10);
	check_eq(a[0], 0);
}

test_case("Reserve Exact") {
	ArrayList<int> a;
	a.push(0);
	a.reserveExact(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

test_case("Reserve Exact From Empty") {
	ArrayList<int> a;
	a.reserveExact(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 0);
}

test_case("Reserve Exact Zero") {
	ArrayList<int> a;
	a.push(0);
	a.reserveExact(0);
	check(a.capacity() > 0);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

test_case("Reserve Tiny Exact") {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	a.reserveExact(1);
	check(a.capacity() >= 10);
	check_eq(a.len(), 10);
	check_eq(a[0], 0);
}

#endif