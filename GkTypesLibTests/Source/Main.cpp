#include "GkTypes/GKTypesLib.h"
#include "GkTypes/File/FileLoader.h"
#include "GkTypes/Thread/ThreadPool.h"

#include <iostream>
#include <string>
#include "ConstexprTests/ConstexprTestUnitTest.h"

#define print(msg) std::cout << msg << '\n'

void testFunc() {
	while (true) {
		Sleep(1000);
		print("hello world! " << std::this_thread::get_id());
	}
}


int main() 
{
	//gk::ThreadPool* pool = new gk::ThreadPool(4);
	//for (int i = 0; i < pool->GetThreadCount(); i++) {
	//	pool->AddFunctionToQueue(testFunc);
	//}
	//pool->ExecuteQueue();
	std::unordered_map<gk::GlobalString, int> map;
	map.insert({ "hello world!", 1 });
}