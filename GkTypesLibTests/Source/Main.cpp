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

constexpr bool is_digit(char c) {
	return c <= '9' && c >= '0';
}

constexpr int stoi_impl(const char* str, int value = 0) {
	return *str ?
		is_digit(*str) ?
		stoi_impl(str + 1, (*str - '0') + value * 10)
		: throw "compile-time-error: not a digit"
		: value;
}

constexpr int stoi(const char* str) {
	return stoi_impl(str);
}

constinit size_t size = gk::string::FromInt(-3556).Len();


int main() {


}