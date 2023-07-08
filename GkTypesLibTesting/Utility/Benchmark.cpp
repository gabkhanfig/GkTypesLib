#include "Benchmark.h"
#include <iostream>

Benchmark::Benchmark(const char* benchmarkName)
{
	name = benchmarkName;
	start = std::chrono::high_resolution_clock::now();
}

void Benchmark::End(TimeUnit timeUnit)
{
	std::chrono::steady_clock::time_point end = std::chrono::high_resolution_clock::now();
	auto duration = end - start;
	std::cout << "[Benchmark]: " << name << ' ';
	switch (timeUnit) {
	case TimeUnit::ns:
		std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() << "ns\n";
		break;
	case TimeUnit::us:
		std::cout << std::chrono::duration_cast<std::chrono::microseconds>(duration).count() << "us\n";
		break;
	case TimeUnit::ms:
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms\n";
		break;
	case TimeUnit::s:
		std::cout << std::chrono::duration_cast<std::chrono::seconds>(duration).count() << "s\n";
		break;
	}
}
