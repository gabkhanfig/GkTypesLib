#include "GkTypes/GKTypesLib.h"
#include "GkTypes/File/FileLoader.h"
#include "GkTypes/Thread/ThreadPool.h"

#include <iostream>
#include <string>
#include "ConstexprTests/ConstexprTestUnitTest.h"

#define print(msg) std::cout << msg << '\n'


int main() {

  gk::ThreadPool* pool = new gk::ThreadPool(gk::ThreadPool::SystemThreadCount() - 1);

  gk::darray<int> a = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  for (int i = 0; i < a.Size(); i++) {
    std::cout << a[i] << " ";
  }
  std::cout << '\n';
  a.RemoveFirst(4);
  for (int i = 0; i < a.Size(); i++) {
    std::cout << a[i] << " ";
  }

  Sleep(10000000);
}