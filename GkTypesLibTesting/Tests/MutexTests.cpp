#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Thread/Mutex.h"
#include "../GkTest.h"
#include <thread>

void incrementValue(gk::Mutex<int>* mutex) {
	auto lock = mutex->lock();
	(*lock.get())++;
}

void runIncrement(gk::Mutex<int>* mutex) {
	Sleep(50);
	for (int i = 0; i < 100; i++) {
		incrementValue(mutex);
	}
}

struct NoDefaultConstructMutex {
	int a;
	int b;

	NoDefaultConstructMutex() = delete;
	NoDefaultConstructMutex(int inA, int inB) : a(inA), b(inB) {}
	NoDefaultConstructMutex(const NoDefaultConstructMutex&) = delete;
	NoDefaultConstructMutex(NoDefaultConstructMutex&&) = delete;

};

namespace UnitTests
{
	TEST(Mutex, SingleThreadsAccess) {
		gk::Mutex<int> mutex = 0;

		std::thread t1{runIncrement, & mutex};

		// If this test isn't completing, something is wrong.

		t1.join();

		EXPECT_EQ(*mutex.getDataNoLock(), 100);
	}

	TEST(Mutex, MultipleThreadsAccess) {
		gk::Mutex<int> mutex = 0;

		std::thread t1{runIncrement, & mutex};
		std::thread t2{runIncrement, & mutex};
		std::thread t3{runIncrement, & mutex};
		std::thread t4{runIncrement, & mutex};

		// If this test isn't completing, something is wrong.

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		EXPECT_EQ(*mutex.getDataNoLock(), 400);
	}

	TEST(Mutex, ConstructWithTypeConstructor) {
		gk::Mutex<NoDefaultConstructMutex> mutex{5, 9};
		EXPECT_EQ((*mutex.getDataNoLock()).a, 5);
		EXPECT_EQ((*mutex.getDataNoLock()).b, 9);
	}
}