#include "pch.h"
#include "../GkTypesLib/GkTypes/GkTypesLib.h"
#include "Utility/EnglishWords.h"
#include <unordered_map>

void MeasureDifferentHashesNoMap(const gk::darray<gk::string>& words) {
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

void BenchmarkHashmap(const gk::darray<gk::string>& words) {
  std::unordered_map<gk::string, int> map;
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


int main(int argc, char** argv) {

  const gk::darray<gk::string> words = EnglishWords::LoadAllEnglishWordsToStrings();
  MeasureDifferentHashesNoMap(words);

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 100; i++) {
    BenchmarkHashmap(words);
  }
  auto end = std::chrono::steady_clock::now();
  long long msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "hashmap benchmark " << msElapsed << "ms\n";


  //::testing::InitGoogleTest(&argc, argv);
  //return RUN_ALL_TESTS();
}