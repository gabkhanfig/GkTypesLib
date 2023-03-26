#include "GkTypes/GKTypesLib.h"
#include "GkTypes/File/FileLoader.h"
#include "GkTypes/Thread/ThreadPool.h"

#include <iostream>
#include <string>
#include "ConstexprTests/ConstexprTestUnitTest.h"

#include "GkTypes/Hashmap/Hashmap.h"

#include <immintrin.h>

#include <numeric>

#define print(msg) std::cout << msg << '\n'


void TestFunc() {
	print("thread: " << std::this_thread::get_id());
	Sleep(2000);
	return;
}


int main() {
	gk::ThreadPool* threadPool = new gk::ThreadPool(gk::ThreadPool::SystemThreadCount() - 1);
	for (int i = 0; i < 10; i++) {
		threadPool->AddFunctionToQueue(TestFunc);
	}
	threadPool->ExecuteQueue();
}