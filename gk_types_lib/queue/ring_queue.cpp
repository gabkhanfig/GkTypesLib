#include "ring_queue.h"

#if GK_TYPES_LIB_TEST
namespace gk
{
	namespace unitTests
	{
		class RingQueueTestClass {
		public:
			int* ptr;
			RingQueueTestClass() {
				ptr = new int;
				*ptr = 0;
			}

			RingQueueTestClass(const RingQueueTestClass& other) {
				ptr = new int;
				*ptr = *other.ptr;
			}

			RingQueueTestClass(RingQueueTestClass&& other) noexcept {
				ptr = other.ptr;
				other.ptr = nullptr;
			}

			~RingQueueTestClass() {
				if (ptr) delete ptr;
			}

			RingQueueTestClass& operator = (const RingQueueTestClass& other) {
				if (ptr) delete ptr;
				ptr = new int;
				*ptr = *other.ptr;
				return *this;
			}

			RingQueueTestClass& operator = (RingQueueTestClass&& other) {
				if (ptr) delete ptr;
				ptr = other.ptr;
				other.ptr = nullptr;
				return *this;
			}
		};
	}
}

using gk::unitTests::RingQueueTestClass;

test_case("ConstructWithCapacity") {
	gk::RingQueue<int> q = gk::RingQueue<int>(2);
}

test_case("ConstructCapacityCorrect") {
	gk::RingQueue<int> q = gk::RingQueue<int>(6);
	check_eq(q.capacity(), 6);
}

test_case("ConstructIsEmpty") {
	gk::RingQueue<int> q = gk::RingQueue<int>(6);
	check_not(q.isFull());
	check(q.isEmpty());
}

test_case("ConstructIsEmptyZeroLength") {
	gk::RingQueue<int> q = gk::RingQueue<int>(6);
	check_eq(q.len(), 0);
}

test_case("AddToQueue") {
	gk::RingQueue<int> q = gk::RingQueue<int>(10);
	q.push(6);
	check_eq(q.len(), 1);
}

test_case("AddMultipleToQueue") {
	gk::RingQueue<int> q = gk::RingQueue<int>(10);
	q.push(6);
	q.push(6);
	check_eq(q.len(), 2);
}

test_case("RemoveFromQueue") {
	gk::RingQueue<int> q = gk::RingQueue<int>(10);
	q.push(6);
	int a = q.pop();
	check_eq(a, 6);
	check_eq(q.len(), 0);
}

test_case("WrapAround") {
	gk::RingQueue<int> q = gk::RingQueue<int>(10);
	for (int i = 0; i < 8; i++) {
		q.push(i);
	}
	for (int i = 0; i < 8; i++) {
		int a = q.pop();
	}
	for (int i = 0; i < 8; i++) {
		q.push(i);
	}
	check_eq(q.len(), 8);
	check_not(q.isEmpty());
	check_not(q.isFull());
}

test_case("Full") {
	gk::RingQueue<int> q = gk::RingQueue<int>(10);
	for (int i = 0; i < 10; i++) {
		q.push(i);
	}
	check(q.isFull());
}

test_case("Iterator") {
	gk::RingQueue<int> q = gk::RingQueue<int>(10);
	for (int i = 0; i < 10; i++) {
		q.push(i);
	}
	int val = 0;
	for (int num : q) {
		check_eq(num, val);
		val++;
	}
	check_eq(q.len(), 0);
}

test_case("PopMove") {
	gk::RingQueue<RingQueueTestClass> q = gk::RingQueue<RingQueueTestClass>(10);
	RingQueueTestClass obj;
	int* ptr = obj.ptr;
	q.push(std::move(obj));
	check_eq(obj.ptr, nullptr);
	RingQueueTestClass obj2 = q.pop();
	check_eq(obj2.ptr, ptr);
}

#endif