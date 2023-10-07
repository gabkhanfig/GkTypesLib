
#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/Queue/RingQueue.h"
#include "../GkTest.h"

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

namespace UnitTests
{
	TEST(RingQueue, ConstructWithCapacity) {
		gk::RingQueue<int> q = gk::RingQueue<int>(2);
	}

	TEST(RingQueue, ConstructCapacityCorrect) {
		gk::RingQueue<int> q = gk::RingQueue<int>(6);
		EXPECT_EQ(q.capacity(), 6);
	}

	TEST(RingQueue, ConstructIsEmpty) {
		gk::RingQueue<int> q = gk::RingQueue<int>(6);
		EXPECT_FALSE(q.isFull());
		EXPECT_TRUE(q.isEmpty());
	}

	TEST(RingQueue, ConstructIsEmptyZeroLength) {
		gk::RingQueue<int> q = gk::RingQueue<int>(6);
		EXPECT_EQ(q.len(), 0);
	}

	TEST(RingQueue, AddToQueue) {
		gk::RingQueue<int> q = gk::RingQueue<int>(10);
		q.push(6);
		EXPECT_EQ(q.len(), 1);
	}

	TEST(RingQueue, AddMultipleToQueue) {
		gk::RingQueue<int> q = gk::RingQueue<int>(10);
		q.push(6);
		q.push(6);
		EXPECT_EQ(q.len(), 2);
	}

	TEST(RingQueue, RemoveFromQueue) {
		gk::RingQueue<int> q = gk::RingQueue<int>(10);
		q.push(6);
		int a = q.pop();
		EXPECT_EQ(a, 6);
		EXPECT_EQ(q.len(), 0);
	}

	TEST(RingQueue, WrapAround) {
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
		EXPECT_EQ(q.len(), 8);
		EXPECT_FALSE(q.isEmpty());
		EXPECT_FALSE(q.isFull());
	}

	TEST(RingQueue, Full) {
		gk::RingQueue<int> q = gk::RingQueue<int>(10);
		for (int i = 0; i < 10; i++) {
			q.push(i);
		}
		EXPECT_TRUE(q.isFull());
	}

	TEST(RingQueue, Iterator) {
		gk::RingQueue<int> q = gk::RingQueue<int>(10);
		for (int i = 0; i < 10; i++) {
			q.push(i);
		}
		int val = 0;
		for (int num : q) {
			EXPECT_EQ(num, val);
			val++;
		}
		EXPECT_EQ(q.len(), 0);
	}

	TEST(RingQueue, PopMove) {
		gk::RingQueue<RingQueueTestClass> q = gk::RingQueue<RingQueueTestClass>(10);
		RingQueueTestClass obj;
		int* ptr = obj.ptr;
		q.push(std::move(obj));
		EXPECT_EQ(obj.ptr, nullptr);
		RingQueueTestClass obj2 = q.pop();
		EXPECT_EQ(obj2.ptr, ptr);
	}

}