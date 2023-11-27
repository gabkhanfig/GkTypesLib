#include "rw_lock.h"

#if GK_TYPES_LIB_TEST
#include <thread>
#include <unordered_map>
#include <string>

namespace gk
{
	namespace unitTests
	{
		void readValueInRwLockSingleThread(gk::RwLock<int>* rwlock) {
			auto lock = rwlock->read();
			check_eq((*lock.get()), 100);
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
				check_eq((*lock.get()), 100);
			}
		}

		void readValueWhileWriteOccurring(gk::RwLock<std::unordered_map<int, std::string>>* rwlock) {
			for (int i = 0; i < 100; i++) {
				auto lock = rwlock->read();
				auto map = lock.get();
				check_ne(map->find(i), map->end());
			}
		}

		void writeValuesWhileReadOccurring(gk::RwLock<std::unordered_map<int, std::string>>* rwlock, int startRange, int endRange) {
			for (int i = startRange; i < endRange; i++) {
				auto lock = rwlock->write();
				lock.get()->insert({ i, std::to_string(i) });
			}
		}

		void readValueConst(const gk::RwLock<int>* rwlock) {
			auto lock = rwlock->read();
			check_eq((*lock.get()), 100);
		}

		struct NoDefaultConstructRwLock {
			int a;
			int b;

			NoDefaultConstructRwLock() = delete;
			NoDefaultConstructRwLock(int inA, int inB) : a(inA), b(inB) {}
			NoDefaultConstructRwLock(const NoDefaultConstructRwLock&) = delete;
			NoDefaultConstructRwLock(NoDefaultConstructRwLock&&) = delete;

		};
	}
}

using gk::unitTests::readValueInRwLockSingleThread;
using gk::unitTests::incrementValueInRwLockSingleThread;
using gk::unitTests::readValueInRwLockMultiThread;
using gk::unitTests::readValueWhileWriteOccurring;
using gk::unitTests::writeValuesWhileReadOccurring;
using gk::unitTests::readValueConst;
using gk::unitTests::NoDefaultConstructRwLock;

test_case("SingleThreadRead") {
	gk::RwLock<int> rwlock = 100;

	std::thread t{readValueInRwLockSingleThread, & rwlock};
	t.join();
}

test_case("SingleThreadWrite") {
	gk::RwLock<int> rwlock = 0;

	std::thread t{incrementValueInRwLockSingleThread, & rwlock};
	t.join();

	check_eq((*rwlock.unsafeGetDataNoLock()), 100);
}

test_case("ManyThreadsRead") {
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

test_case("ManyThreadsWrite") {
	gk::RwLock<int> rwlock = 0;

	std::thread t1{incrementValueInRwLockSingleThread, & rwlock};
	std::thread t2{incrementValueInRwLockSingleThread, & rwlock};
	std::thread t3{incrementValueInRwLockSingleThread, & rwlock};
	std::thread t4{incrementValueInRwLockSingleThread, & rwlock};

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	check_eq((*rwlock.unsafeGetDataNoLock()), 400);
}

test_case("ManyThreadsReadAndWrite") {
	gk::RwLock<std::unordered_map<int, std::string>> rwlock;

	for (int i = 0; i < 100; i++) {
		rwlock.unsafeGetDataNoLock()->insert({ i, std::to_string(i) });
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

	check_eq(rwlock.unsafeGetDataNoLock()->size(), 500);
}

test_case("ConstRead") {
	gk::RwLock<int> rwlock = 100;
	readValueConst(&rwlock);
}


#endif