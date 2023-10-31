#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Thread/RwLock.h"
#include "../../GkTypesLib/GkTypes/Hash/Hashmap.h"
#include "../../GkTypesLib/GkTypes/String/String.h"
#include "../GkTest.h"
#include <thread>

void readValueInRwLockSingleThread(gk::RwLock<int>* rwlock) {
	auto lock = rwlock->read();
	EXPECT_EQ((*lock.get()), 100);
}

void incrementValueInRwLockSingleThread(gk::RwLock<int>* rwlock) {
	for (int i = 0; i < 100; i++) {
		auto lock = rwlock->write();
		(*lock.get())++;
	}
}

void readValueInRwLockMultiThread(gk::RwLock<int>* rwlock) {
	for (int i = 0; i < 1000; i++) {
		auto lock = rwlock->read();
		EXPECT_EQ((*lock.get()), 100);
	}
}

void readValueWhileWriteOccurring(gk::RwLock<gk::HashMap<int, gk::String>>* rwlock) {
	for (int i = 0; i < 100; i++) {
		auto lock = rwlock->read();
		EXPECT_FALSE(lock.get()->find(i).none());
	}
}

void writeValuesWhileReadOccurring(gk::RwLock<gk::HashMap<int, gk::String>>* rwlock, int startRange, int endRange) {
	for (int i = startRange; i < endRange; i++) {
		auto lock = rwlock->write();
		lock.get()->insert(i, gk::String::from(i));
	}
}

void readValueConst(const gk::RwLock<int>* rwlock) {
	auto lock = rwlock->read();
	EXPECT_EQ((*lock.get()), 100);
}

void spinReadValueInRwLockSingleThread(gk::RwLock<int>* rwlock) {
	auto lock = rwlock->spinRead();
	EXPECT_EQ((*lock.get()), 100);
}

void spinIncrementValueInRwLockSingleThread(gk::RwLock<int>* rwlock) {
	for (int i = 0; i < 100; i++) {
		auto lock = rwlock->spinWrite();
		(*lock.get())++;
	}
}

void spinReadValueInRwLockMultiThread(gk::RwLock<int>* rwlock) {
	for (int i = 0; i < 1000; i++) {
		auto lock = rwlock->spinRead();
		EXPECT_EQ((*lock.get()), 100);
	}
}

void spinReadValueWhileWriteOccurring(gk::RwLock<gk::HashMap<int, gk::String>>* rwlock) {
	for (int i = 0; i < 100; i++) {
		auto lock = rwlock->spinRead();
		EXPECT_FALSE(lock.get()->find(i).none());
	}
}

void spinWriteValuesWhileReadOccurring(gk::RwLock<gk::HashMap<int, gk::String>>* rwlock, int startRange, int endRange) {
	for (int i = startRange; i < endRange; i++) {
		auto lock = rwlock->spinWrite();
		lock.get()->insert(i, gk::String::from(i));
	}
}

void spinReadValueConst(const gk::RwLock<int>* rwlock) {
	auto lock = rwlock->spinRead();
	EXPECT_EQ((*lock.get()), 100);
}

namespace UnitTests
{
	TEST(RwLock, SingleThreadRead) {
		gk::RwLock<int> rwlock = 100;

		std::thread t{readValueInRwLockSingleThread, & rwlock};
		t.join();
	}

	TEST(RwLock, SingleThreadWrite) {
		gk::RwLock<int> rwlock = 0; 

		std::thread t{incrementValueInRwLockSingleThread, & rwlock};
		t.join();

		EXPECT_EQ((*rwlock.getDataNoLock()), 100);
	}

	TEST(RwLock, ManyThreadsRead) {
		gk::RwLock<int> rwlock = 100;

		std::thread t1{readValueInRwLockSingleThread, & rwlock};
		std::thread t2{readValueInRwLockSingleThread, & rwlock};
		std::thread t3{readValueInRwLockSingleThread, & rwlock};
		std::thread t4{readValueInRwLockSingleThread, & rwlock};

		t1.join();
		t2.join();
		t3.join();
		t4.join();
	}

	TEST(RwLock, ManyThreadsWrite) {
		gk::RwLock<int> rwlock = 0;

		std::thread t1{incrementValueInRwLockSingleThread, & rwlock};
		std::thread t2{incrementValueInRwLockSingleThread, & rwlock};
		std::thread t3{incrementValueInRwLockSingleThread, & rwlock};
		std::thread t4{incrementValueInRwLockSingleThread, & rwlock};

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		EXPECT_EQ((*rwlock.getDataNoLock()), 400);
	}

	TEST(RwLock, ManyThreadsReadAndWrite) {
		gk::RwLock<gk::HashMap<int, gk::String>> rwlock;

		for (int i = 0; i < 100; i++) {
			rwlock.getDataNoLock()->insert(i, gk::String::from(i));
		}

		std::thread t1{writeValuesWhileReadOccurring, & rwlock, 100, 200};
		std::thread t2{writeValuesWhileReadOccurring, & rwlock, 200, 400};

		std::thread t3{readValueWhileWriteOccurring, & rwlock};
		std::thread t4{readValueWhileWriteOccurring, & rwlock};

		std::thread t5{writeValuesWhileReadOccurring, & rwlock, 400, 450};
		std::thread t6{writeValuesWhileReadOccurring, & rwlock, 450, 500};

		std::thread t7{readValueWhileWriteOccurring, & rwlock};
		std::thread t8{readValueWhileWriteOccurring, & rwlock};

		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
		t6.join();
		t7.join();
		t8.join();

		EXPECT_EQ(rwlock.getDataNoLock()->size(), 500);
	}

	TEST(RwLock, ConstRead) {
		gk::RwLock<int> rwlock = 100;
		readValueConst(&rwlock);
	}

	TEST(RwLock, SpinSingleThreadRead) {
		gk::RwLock<int> rwlock = 100;

		std::thread t{spinReadValueInRwLockSingleThread, & rwlock};
		t.join();
	}

	TEST(RwLock, SpinSingleThreadWrite) {
		gk::RwLock<int> rwlock = 0; 

		std::thread t{spinIncrementValueInRwLockSingleThread, & rwlock};
		t.join();

		EXPECT_EQ((*rwlock.getDataNoLock()), 100);
	}

	TEST(RwLock, SpinManyThreadsRead) {
		gk::RwLock<int> rwlock = 100;

		std::thread t1{spinReadValueInRwLockSingleThread, & rwlock};
		std::thread t2{spinReadValueInRwLockSingleThread, & rwlock};
		std::thread t3{spinReadValueInRwLockSingleThread, & rwlock};
		std::thread t4{spinReadValueInRwLockSingleThread, & rwlock};

		t1.join();
		t2.join();
		t3.join();
		t4.join();
	}

	TEST(RwLock, SpinManyThreadsWrite) {
		gk::RwLock<int> rwlock = 0;

		std::thread t1{spinIncrementValueInRwLockSingleThread, & rwlock};
		std::thread t2{spinIncrementValueInRwLockSingleThread, & rwlock};
		std::thread t3{spinIncrementValueInRwLockSingleThread, & rwlock};
		std::thread t4{spinIncrementValueInRwLockSingleThread, & rwlock};

		t1.join();
		t2.join();
		t3.join();
		t4.join();

		EXPECT_EQ((*rwlock.getDataNoLock()), 400);
	}

	TEST(RwLock, SpinManyThreadsReadAndWrite) {
		gk::RwLock<gk::HashMap<int, gk::String>> rwlock;

		for (int i = 0; i < 100; i++) {
			rwlock.getDataNoLock()->insert(i, gk::String::from(i));
		}

		std::thread t1{spinWriteValuesWhileReadOccurring, & rwlock, 100, 200};
		std::thread t2{spinWriteValuesWhileReadOccurring, & rwlock, 200, 400};

		std::thread t3{spinReadValueWhileWriteOccurring, & rwlock};
		std::thread t4{spinReadValueWhileWriteOccurring, & rwlock};

		std::thread t5{spinWriteValuesWhileReadOccurring, & rwlock, 400, 450};
		std::thread t6{spinWriteValuesWhileReadOccurring, & rwlock, 450, 500};

		std::thread t7{spinReadValueWhileWriteOccurring, & rwlock};
		std::thread t8{spinReadValueWhileWriteOccurring, & rwlock};

		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
		t6.join();
		t7.join();
		t8.join();

		EXPECT_EQ(rwlock.getDataNoLock()->size(), 500);
	}

	TEST(RwLock, SpinConstRead) {
		gk::RwLock<int> rwlock = 100;
		spinReadValueConst(&rwlock);
	}



}