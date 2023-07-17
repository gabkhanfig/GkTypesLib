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

class Test {
public:
  int a;

  Test(int _a) {
    a = _a;
  }

  void DoSomething(int num, float num2) {
    a = num * num2;
    std::cout << a << std::endl;
  }

  int GetNum() const {
    return a;
  }

  int GetNumAdded(int b) const {
    return a + b;
  }
};


void SomeRandomFunc(bool b, int num) {
  if (b) {
    std::cout << "true... " << num << std::endl;
  }
  else {
    std::cout << "false... " << num << std::endl;
  }
}

double Multiply(double a, double b) {
  return a * b;
}

//typedef void (Test::*TestMemberFunc)(int);


int main(int argc, char** argv) {

  //TestMemberFunc fn = &Test::DoSomething;
  //Test* obj = new Test(0);

  //Test* obj2 = new Test(14);
  //(obj->*fn)(5);


  long long num = -static_cast<long long>(bool(false));
  long long other = ~static_cast<long long>(!bool(false));
  std::cout << num << std::endl;
  std::cout << other << std::endl;


  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();

  //auto e = gk::Event<void, int, float>::Create(obj, &Test::DoSomething);
  //auto p = gk::Event<void, bool, int>::Create(SomeRandomFunc);
  //auto ret = gk::Event<double, double, double>::Create(Multiply);
  //auto memFuncReturn = gk::Event<int, int>::Create(obj2, &Test::GetNumAdded);
  //auto memFuncReturnNoArgs = gk::Event<int>::Create(obj2, &Test::GetNum);
  //e->Invoke(4, 5);
  //p->Invoke(false, 10);
  //double c = ret->Invoke(10, 8);
  //std::cout << c << std::endl;
  //int number = memFuncReturn->Invoke(14);
  //std::cout << number << std::endl;

}