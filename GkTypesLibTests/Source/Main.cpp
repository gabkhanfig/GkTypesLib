#include "GkTypes/GKTypesLib.h"
#include "GkTypes/File/FileLoader.h"
#include "GkTypes/Thread/ThreadPool.h"

#include <iostream>
#include <string>
#include "ConstexprTests/ConstexprTestUnitTest.h"

#define print(msg) std::cout << msg << '\n'


int main() {

  gk::ThreadPool* pool = new gk::ThreadPool(gk::ThreadPool::SystemThreadCount() - 1);

  Sleep(10000000);
}