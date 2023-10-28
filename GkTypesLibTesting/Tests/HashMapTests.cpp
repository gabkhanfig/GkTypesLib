#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/Hash/Hashmap.h"
#include "../../GkTypesLib/GkTypes/String/String.h"
#include "../GkTest.h"

namespace UnitTests
{
	TEST(HashMap, DefaultConstruct) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		EXPECT_EQ(map.size(), 0);
	}

	TEST(HashMap, InsertIntSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		map.insert(1, 5);
		EXPECT_EQ(map.size(), 1);
	}

	TEST(HashMap, InsertMultipleIntsSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(i, 100);
		}
		EXPECT_EQ(map.size(), 20);
	}

	TEST(HashMap, InsertStringSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		map.insert(gk::String("hello"_str), 5);
		EXPECT_EQ(map.size(), 1);
	}

	TEST(HashMap, InsertMultipleStringsSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(gk::String::from(i), 100);
		}
		EXPECT_EQ(map.size(), 20);
	}

	TEST(HashMap, SanityCheckCopyKeyAndValueSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			gk::String numStr = gk::String::from(i);
			int value = 100;
			map.insert(numStr, value);
		}
		EXPECT_EQ(map.size(), 20);
	}

	TEST(HashMap, SanityCheckCopyKeyAndMoveValueSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			gk::String numStr = gk::String::from(i);
			map.insert(numStr, 100);
		}
		EXPECT_EQ(map.size(), 20);
	}

	TEST(HashMap, SanityCheckMoveKeyAndCopyValueSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			int value = 100;
			map.insert(gk::String::from(i), value);
		}
		EXPECT_EQ(map.size(), 20);
	}

	TEST(HashMap, InsertMultipleWithDuplicateKeysSize) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			int value = 100;
			map.insert(gk::String::from(i), value);
		}
		for (int i = 0; i < 20; i++) {
			map.insert(gk::String::from(i), 100);
		}
		EXPECT_EQ(map.size(), 20);
	}

	TEST(HashMap, FindIntSize1) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		map.insert(5, 100);
		auto found = map.find(5);
		EXPECT_EQ(*found.some(), 100);
	}

	TEST(HashMap, DontFindIntSize1) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		map.insert(5, 100);
		auto found = map.find(13);
		EXPECT_TRUE(found.none());
	}

	TEST(HashMap, FindIntSizeMultiple) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(i, 100);
		}
		auto found = map.find(5);
		EXPECT_EQ(*found.some(), 100);
	}

	TEST(HashMap, DontFindIntSizeMultiple) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<int, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(i, 100);
		}
		auto found = map.find(21);
		EXPECT_TRUE(found.none());
	}

	TEST(HashMap, FindStringSize1) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		map.insert(gk::String::from(5), 100);
		auto found = map.find('5');
		EXPECT_EQ(*found.some(), 100);
	}

	TEST(HashMap, DontFindStringSize1) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		map.insert(gk::String::from(5), 100);
		auto found = map.find("13"_str);
		EXPECT_TRUE(found.none());
	}

	TEST(HashMap, FindStringSizeMultiple) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(gk::String::from(i), 100);
		}
		auto found = map.find("5"_str);
		EXPECT_EQ(*found.some(), 100);
	}

	TEST(HashMap, DontFindStringSizeMultiple) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(gk::String::from(i), 100);
		}
		auto found = map.find("21"_str);
		EXPECT_TRUE(found.none());
	}

	TEST(HashMap, ConstFindStringSize1) {
		MemoryLeakDetector leakDetector;
		const gk::HashMap<gk::String, int> map = []() {
			gk::HashMap<gk::String, int> outMap;
			outMap.insert(gk::String::from(5), 100);
			return outMap;
		}();
		auto found = map.find('5');
		EXPECT_EQ(*found.some(), 100);
	}

	TEST(HashMap, ConstDontFindStringSize1) {
		MemoryLeakDetector leakDetector;
		const gk::HashMap<gk::String, int> map = []() {
			gk::HashMap<gk::String, int> outMap;
			outMap.insert(gk::String::from(5), 100);
			return outMap;
		}();
		auto found = map.find("13"_str);
		EXPECT_TRUE(found.none());
	}

	TEST(HashMap, ConstFindStringSizeMultiple) {
		MemoryLeakDetector leakDetector;
		const gk::HashMap<gk::String, int> map = []() {
			gk::HashMap<gk::String, int> outMap;
			for (int i = 0; i < 20; i++) {
				outMap.insert(gk::String::from(i), 100);
			}
			return outMap;
		}();
		auto found = map.find("5"_str);
		EXPECT_EQ(*found.some(), 100);
	}

	TEST(HashMap, ConstDontFindStringSizeMultiple) {
		MemoryLeakDetector leakDetector;
		const gk::HashMap<gk::String, int> map = []() {
			gk::HashMap<gk::String, int> outMap;
			for (int i = 0; i < 20; i++) {
				outMap.insert(gk::String::from(i), 100);
			}
			return outMap;
		}();
		auto found = map.find("21"_str);
		EXPECT_TRUE(found.none());
	}

	TEST(HashMap, EraseSingleElement) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		map.insert(gk::String::from(5), 100);
		map.erase(gk::String::from(5));
		auto found = map.find("5"_str);
		EXPECT_TRUE(found.none());
		EXPECT_EQ(map.size(), 0);
	}

	TEST(HashMap, EraseSingleElementFromMultiple) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(gk::String::from(i), 100);
		}
		map.erase(gk::String::from(5));
		auto found = map.find("5"_str);
		EXPECT_TRUE(found.none());
		EXPECT_EQ(map.size(), 19);
	}

	TEST(HashMap, EraseAllElements) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 20; i++) {
			map.insert(gk::String::from(i), 100);
		}
		for (int i = 0; i < 20; i++) {
			gk::String numStr = gk::String::from(i);
			auto found = map.find(numStr);
			EXPECT_EQ(*found.some(), 100);
			map.erase(gk::String::from(i));
			auto found2 = map.find(numStr);
			EXPECT_TRUE(found2.none());
		}
		for (int i = 0; i < 20; i++) {
			auto found = map.find(gk::String::from(i));
			EXPECT_TRUE(found.none());
		}
		EXPECT_EQ(map.size(), 0);
	}

	TEST(HashMap, Reserve) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		map.reserve(100);
		EXPECT_EQ(map.size(), 0);
		for (int i = 0; i < 100; i++) {
			map.insert(gk::String::from(i), -1);
		}
		EXPECT_EQ(map.size(), 100);
	}
	
	TEST(HashMap, Iterator) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 100; i++) {
			map.insert(gk::String::from(i), -1);
		}
		int iterCount = 0;
		for (auto pair : map) {
			iterCount++;
			EXPECT_EQ(pair->value, -1);
		}
		EXPECT_EQ(iterCount, 100);
	}
	
	TEST(HashMap, ConstIterator) {
		MemoryLeakDetector leakDetector;
		const gk::HashMap<gk::String, int> map = []() {
			gk::HashMap<gk::String, int> mapOut;
			for (int i = 0; i < 100; i++) {
				mapOut.insert(gk::String::from(i), -1);
			}
			return mapOut;
		}();
		int iterCount = 0;
		for (auto pair : map) {
			iterCount++;
			EXPECT_EQ(pair->value, -1);
		}
		EXPECT_EQ(iterCount, 100);
	}

	TEST(HashMap, CopyConstruct) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 100; i++) {
			map.insert(gk::String::from(i), -1);
		}
		gk::HashMap<gk::String, int> otherMap = map;
		EXPECT_EQ(otherMap.size(), 100);
		int iterCount = 0;
		for (auto pair : otherMap) {
			iterCount++;
			EXPECT_EQ(pair->value, -1);
		}
		EXPECT_EQ(iterCount, 100);
	}

	TEST(HashMap, MoveConstruct) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 100; i++) {
			map.insert(gk::String::from(i), -1);
		}
		gk::HashMap<gk::String, int> otherMap = std::move(map);
		EXPECT_EQ(otherMap.size(), 100);
		int iterCount = 0;
		for (auto pair : otherMap) {
			iterCount++;
			EXPECT_EQ(pair->value, -1);
		}
		EXPECT_EQ(iterCount, 100);
	}

	TEST(HashMap, CopyAssign) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 100; i++) {
			map.insert(gk::String::from(i), -1);
		}
		gk::HashMap<gk::String, int> otherMap;
		otherMap.insert("hi"_str, 1);
		otherMap = map;
		EXPECT_EQ(otherMap.size(), 100);
		int iterCount = 0;
		for (auto pair : otherMap) {
			iterCount++;
			EXPECT_EQ(pair->value, -1);
		}
		EXPECT_EQ(iterCount, 100);
	}

	TEST(HashMap, MoveAssign) {
		MemoryLeakDetector leakDetector;
		gk::HashMap<gk::String, int> map;
		for (int i = 0; i < 100; i++) {
			map.insert(gk::String::from(i), -1);
		}
		gk::HashMap<gk::String, int> otherMap;
		otherMap.insert("hi"_str, 1);
		otherMap = std::move(map);
		EXPECT_EQ(otherMap.size(), 100);
		int iterCount = 0;
		for (auto pair : otherMap) {
			iterCount++;
			EXPECT_EQ(pair->value, -1);
		}
		EXPECT_EQ(iterCount, 100);
	}
}