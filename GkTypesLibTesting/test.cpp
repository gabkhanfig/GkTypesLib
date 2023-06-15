#include "pch.h"
#include "../GkTypesLib/GkTypes/GkTypesLib.h"

int main(int argc, char** argv) {
  gk::bitset<66> a;
  //sizeof(gk::bitset<65>)

  gk::bitset<65> bit = std::array<unsigned long long, 2>{0, 1};
  std::cout << bit.GetBit(64) << std::endl; 
  std::cout << bit.GetBit(0) << std::endl;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}