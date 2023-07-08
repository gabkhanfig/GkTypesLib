#pragma once

#include <chrono>

class Benchmark 
{
public:

	enum class TimeUnit {
		ns,
		us,
		ms,
		s
	};

	Benchmark(const char* benchmarkName);

	void End(TimeUnit timeUnit);

private:

	const char* name;
	std::chrono::steady_clock::time_point start;
};