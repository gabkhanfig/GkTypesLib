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

/*
#if false
struct MutexTest
{
  static constexpr size_t tagBits = ~(1ULL << 63);
  static constexpr size_t notLockedBit = (1ULL << 63);

  MutexTest() {
    atom.store(false);
  }

  void lock() {
    bool expected = false;
    // While is locked and
    // Fetch the value. If expected equals the value, load desired into the value. Otherwise expected is set to the actual value
    while (
      //(expected & notLockedBit)
      //&& !atom.compare_exchange_weak(expected, (expected + 1) & notLockedBit))
      !atom.compare_exchange_strong(expected, true))
    {
      expected = false;
      //std::cout << "hai\n";
      // pause / yield
    }
  }

  void unlock() {
    atom.store(false);
  }

  std::atomic<bool> atom;
};
#else
struct MutexTest // sorta works sorta doesn't ?
{
  static constexpr size_t tagBits = ~(1ULL << 63);
  static constexpr size_t lockedBit = (1ULL << 63);

  MutexTest() {
    atom.store(0);
  }

  void lock() {
    size_t expected = atom.load() & tagBits;
    // While is locked and
    // Fetch the value. If expected equals the value, load desired into the value. Otherwise expected is set to the actual value
    while (
      //(expected & notLockedBit)
      //&& !atom.compare_exchange_weak(expected, (expected + 1) | notLockedBit))
      !atom.compare_exchange_strong(expected, (expected + 1) | lockedBit))
    {
      expected = expected & tagBits;
      _mm_pause();
    }
  }

  void unlock() {
    size_t value = atom.load();
    atom.store(value & tagBits);
  }
  std::atomic<size_t> atom;
};
#endif
*/

int main(int argc, char** argv) {
  runGkTests(argc, argv);
}