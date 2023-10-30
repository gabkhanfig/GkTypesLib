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

void nestedLock(gk::Mutex<int>* mutex) {
	for (int i = 0; i < 1000; i++) {
		auto lock1 = mutex->lock();
		auto lock2 = mutex->lock();
		auto lock3 = mutex->lock();
		(*lock2.get())++;
	}
}

void incrementValueSpin(gk::Mutex<int>* mutex) {
	auto lock = mutex->spinlock();
	(*lock.get())++;
}

void runIncrementSpin(gk::Mutex<int>* mutex) {
	Sleep(50);
	for (int i = 0; i < 100; i++) {
		incrementValueSpin(mutex);
	}
}

void nestedLockSpin(gk::Mutex<int>* mutex) {
	for (int i = 0; i < 1000; i++) {
		auto lock1 = mutex->spinlock();
		auto lock2 = mutex->spinlock();
		auto lock3 = mutex->spinlock();
		(*lock2.get())++;
	}
}

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

	TEST(Mutex, NestedLock) {
		gk::Mutex<int> mutex = 0;

		std::thread t1{nestedLock, & mutex};
		std::thread t2{nestedLock, & mutex};
		std::thread t3{nestedLock, & mutex};
		std::thread t4{nestedLock, & mutex};

		// If this test isn't completing, something is wrong.

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		EXPECT_EQ(*mutex.getDataNoLock(), 4000);
	}

	TEST(Mutex, SingleThreadsAccessSpin) {
		gk::Mutex<int> mutex = 0;

		std::thread t1{runIncrementSpin, & mutex};

		// If this test isn't completing, something is wrong.

		t1.join();

		EXPECT_EQ(*mutex.getDataNoLock(), 100);
	}

	TEST(Mutex, MultipleThreadsAccessSpin) {
		gk::Mutex<int> mutex = 0;

		std::thread t1{runIncrementSpin, & mutex};
		std::thread t2{runIncrementSpin, & mutex};
		std::thread t3{runIncrementSpin, & mutex};
		std::thread t4{runIncrementSpin, & mutex};

		// If this test isn't completing, something is wrong.

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		EXPECT_EQ(*mutex.getDataNoLock(), 400);
	}

	TEST(Mutex, NestedLockSpin) {
		gk::Mutex<int> mutex = 0;

		std::thread t1{nestedLockSpin, & mutex};
		std::thread t2{nestedLockSpin, & mutex};
		std::thread t3{nestedLockSpin, & mutex};
		std::thread t4{nestedLockSpin, & mutex};

		// If this test isn't completing, something is wrong.

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		EXPECT_EQ(*mutex.getDataNoLock(), 4000);
	}
}