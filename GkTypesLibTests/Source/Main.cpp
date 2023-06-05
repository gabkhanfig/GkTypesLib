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

int main() {

	double x = -14.332202462295177;
	double y = 8.4711759959645860;
	double z = 0.062521121093571233;

	//gk::string long1 = "123456789012345678901234567890";
	//gk::string short1 = "aaaa";
	//gk::string test2 = long1 + short1;

	gk::string test = gk::string("x: ") + gk::string::FromFloat(x) + gk::string(", y: ") + gk::string::FromFloat(y) + gk::string(", z: ");// +gk::string::FromFloat(z);
	gk::string zstr = gk::string::FromFloat(z);
	//print((zstr == gk::string::FromFloat(z))); // From float returns an incorrect length?
	gk::string combined = test + zstr;
	print(combined);
	//double num = 1.010111;
	//print(gk::string::FromFloat(num));
	//gk::string zstr = gk::string::FromFloat(z);
	//print(zstr);

}