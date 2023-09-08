#include "pch.h"
#include "../GkTypesLib/GkTypes/GkTypesLib.h"
#include "Utility/EnglishWords.h"
#include "Utility/Benchmark.h"
#include <unordered_map>
#include <xmmintrin.h>
#include <intrin.h>
#include "../GkTypesLib/GkTypes/Event/Event.h"

const int iterations = 1;

void MeasureDifferentHashesNoMap(const gk::darray<gk::String>& words) {
  auto start1 = std::chrono::steady_clock::now();
  for (int i = 0; i < words.Size(); i++) {
    size_t num = words[i].ComputeHash();
  }
  auto end1 = std::chrono::steady_clock::now();
  long long msElapsed = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();
  std::cout << "Normal hash took " << msElapsed << "us\n";
  auto start2 = std::chrono::steady_clock::now();
  for (int i = 0; i < words.Size(); i++) {
    size_t num = words[i].ComputeHash();
  }
  auto end2 = std::chrono::steady_clock::now();
  msElapsed = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();
  std::cout << "Murmur hash took " << msElapsed << "us\n";
}

void BenchmarkHashmap(const gk::darray<gk::String>& words) {
  std::unordered_map<gk::String, int> map;
  for (int i = 0; i < words.Size(); i++) {
    map.insert({ words[i], 0 });
  }
  for (int i = 0; i < words.Size(); i++) {
    auto found = map.find(words[i]);
    if (found->second != 0) {
      std::cout << "panic\n";
    }
  }
}

template<typename T> 
gk::darray<T> ConvertToOtherStringArray(const gk::darray<gk::String>& words) {
  gk::darray<T> strs;
  for (const gk::String& word : words) {
    T asStr = T(word.CStr());
    const uint32 element = strs.Add(asStr);
    //std::cout << asStr << std::endl;
    //std::cout << strs.At(element);
    //std::cout << "\n\n";
  }
  return strs;
}

template <typename T>
void RunStringBenchmark(const char* benchmarkName, const gk::darray<T>& words) {
  Benchmark benchmark{ benchmarkName };
  uint64 found = 0;
  for (int i = 0; i < 1; i++) {
    for (const T& word : words) {  
      if (words.Contains(word)) found++;
    }
  }
  benchmark.End(Benchmark::TimeUnit::ms);
  std::cout << found << std::endl;
}

void RunStdStringBenchmark(const gk::darray<std::string>& words) {
  RunStringBenchmark<std::string>("std string", words);
}

void RunNormalStringBenchmark(const gk::darray<gk::String>& words) {
  RunStringBenchmark<gk::String>("normal string", words);
}

#define CONST_CHAR_ITERATIONS words.Size() // Breaking at 262144, power of 2. Why?
// nonseasonableness
// nonseasonably


void RunStdStringConstCharBenchmark(const char* benchmarkName, const gk::darray<std::string>& words, const gk::darray<const char*>& chars) {
  Benchmark benchmark{ benchmarkName };
  uint64 found = 0;
  for (int i = 0; i < iterations; i++) {
    for (uint64 i = 0; i < CONST_CHAR_ITERATIONS; i++) {
      for (const char* str : chars) {
        if (words[i] == str) {
          found++;
          break;
        }
      }
    }
  }
  benchmark.End(Benchmark::TimeUnit::ms);
  std::cout << found << std::endl;
}

void RunGkStringConstCharBenchmark(const char* benchmarkName, const gk::darray<gk::String>& words, const gk::darray<const char*>& chars) {
  Benchmark benchmark{ benchmarkName };
  uint64 found = 0;
  for (int i = 0; i < iterations; i++) {
    for (uint64 i = 0; i < CONST_CHAR_ITERATIONS; i++) {
      for (const char* str : chars) {
        if (words[i] == str) {
          found++;
          break;
        }
      }
    }
  }
  benchmark.End(Benchmark::TimeUnit::ms);
  std::cout << found << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}