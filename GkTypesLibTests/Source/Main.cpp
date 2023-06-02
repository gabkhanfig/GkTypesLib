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


#define flt(n) gk::string::FromFloat(n)
int main() {

	double num = 1.010111;
	print(gk::string::FromFloat(num));

}