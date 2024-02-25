#include "mutex.h"

#if GK_TYPES_LIB_TEST
#include <thread>

namespace gk
{
	namespace unitTests
	{
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
	}
}

using gk::unitTests::incrementValue;
using gk::unitTests::runIncrement;
using gk::unitTests::NoDefaultConstructMutex;

test_case("SingleThreadsAccess") {
	gk::Mutex<int> mutex = 0;

	std::thread t1{runIncrement, & mutex};

	// If this test isn't completing, something is wrong.

	t1.join();

	check_eq(*mutex.getDataNoLock(), 100);
}

test_case("MultipleThreadsAccess") {
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

	check_eq(*mutex.getDataNoLock(), 400);
}

test_case("ConstructWithTypeConstructor") {
	gk::Mutex<NoDefaultConstructMutex> mutex{5, 9};
	check_eq((*mutex.getDataNoLock()).a, 5);
	check_eq((*mutex.getDataNoLock()).b, 9);
}

test_case("indirection") {
	gk::Mutex<NoDefaultConstructMutex> mutex = { 5, 9 };
	auto lock = mutex.lock();
	check_eq(lock->a, 5);
	check_eq(lock->b, 9);
}
#endif