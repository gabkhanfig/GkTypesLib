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

const char* test = "ashdgajshdgasd";


void TestFunc() {
	print("thread: " << std::this_thread::get_id());
	Sleep(2000);
	return;
}

struct Example {
	int a;
	int b;
};
template<>
[[nodiscard]] constexpr static gk::string gk::string::From<Example>(const Example& value) {
	return string::FromInt(value.a) + gk::string(", ") + gk::string::FromInt(value.b);
}


int main() {
	gk::bitset<8> b;
	b.SetBit(1, true);
	print(gk::string::FromBool(b.GetBit(0)));


	Example t{ 1, 2 };
	print(gk::string::From<Example>(t));


	gk::string test = "aaaa" + gk::string("aaaa");
}