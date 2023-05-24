#include "GkTypes/GKTypesLib.h"
#include "GkTypes/File/FileLoader.h"
#include "GkTypes/Thread/ThreadPool.h"

#include <iostream>
#include <string>
#include "ConstexprTests/ConstexprTestUnitTest.h"

#include "GkTypes/Hashmap/Hashmap.h"
#include "GkTypes/Bitset/Bitset.h"

#include <immintrin.h>

#include <numeric>

#define print(msg) std::cout << msg << '\n'


void TestFunc() {
	print("thread: " << std::this_thread::get_id());
	Sleep(2000);
	return;
}


int main() {
	gk::bitset<8> b;
	b.SetBit(1, true);
	print(gk::string::FromBool(b.GetBit(0)));

}