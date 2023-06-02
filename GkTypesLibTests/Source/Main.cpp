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
#include <cmath>

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

	print(gk::string::FromFloat(-0.001));
	//gk::bitset<8> b;
	//b.SetBit(1, true);
	//print(gk::string::FromBool(b.GetBit(0)));

	//gk::string something = "aaaa" + gk::string("balls");

	//Example t{ 1, 2 };
	//print(gk::string::From<Example>(t));

	//std::to_string(1.123456789);
	//gk::string numStr = gk::string::FromFloat(1.123456789);
	//print(numStr);
}