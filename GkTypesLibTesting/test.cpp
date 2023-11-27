#include "pch.h"
#include "../GkTypesLib/GkTypes/GkTypesLib.h"
#include "Utility/EnglishWords.h"
#include "Utility/Benchmark.h"
#include <unordered_map>
#include <xmmintrin.h>
#include <intrin.h>
#include <vector>
#include <compare>
#include "GkTest.h"
#include "../GkTypesLib/GkTypes/Job/JobThread.h"

#include "../GkTypesLib/GkTypes/Allocator/Allocator.h"
#include "../GkTypesLib/GkTypes/Allocator/HeapAllocator.h"

#include "../GkTypesLib/GkTypes/Array/ArrayList.h"

#include <utility>
#include "../GkTypesLib/GkTypes/Job/JobFuture.h"
#include "../GkTypesLib/GkTypes/Job/JobSystem.h"

int main(int argc, char** argv) {
	//runGkTests(argc, argv);

	gk::ArrayList<int8> a;
	for (int i = 0; i < 100; i++) {
		a.push(i);
	}
	auto opt = a.find(80);
	auto ind = opt.some();
	std::cout << ind << std::endl;
}