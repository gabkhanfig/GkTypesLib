#include "hashmap.h"
#include "../cpu_features/cpu_feature_detector.h"
#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

using gk::i8;
using gk::u64;
using gk::internal::PairHashBits;
using gk::Option;
using gk::usize;


constexpr bool SHOULD_LOG_HASHMAP_DYNAMICALLY_LOADED_FUNCTIONS = false;


typedef u64 (*FuncFindEqualHashCodeBitmask)(const i8*, PairHashBits);
typedef Option<i8>(*FuncFindFirstZeroSlot)(const i8*);


static u64 avx512FindHashCode(const i8* buffer, PairHashBits hashCode) {
	const __m512i bufferVec = *reinterpret_cast<const __m512i*>(buffer);
	const __m512i hashCodeVec = _mm512_set1_epi8(hashCode.value);
	return _mm512_cmpeq_epi8_mask(bufferVec, hashCodeVec);
}

static u64 avx2FindHashCode(const i8* buffer, PairHashBits hashCode) {
	const __m256i* bufferVec = reinterpret_cast<const __m256i*>(buffer);
	const __m256i hashCodeVec = _mm256_set1_epi8(hashCode.value);
	const u64 out =
		static_cast<u64>(_mm256_cmpeq_epi8_mask(bufferVec[0], hashCodeVec))
		| (static_cast<u64>(_mm256_cmpeq_epi8_mask(bufferVec[1], hashCodeVec)) << 32);
	return out;
}

static Option<i8> avx512FindFirstZeroSlot(const i8* buffer) {
	const __m512i bufferVec = *reinterpret_cast<const __m512i*>(buffer);
	const __m512i zeroVec = _mm512_set1_epi8(0);
	const u64 result = _mm512_cmpeq_epi8_mask(bufferVec, zeroVec);
	unsigned long index;
	if (!_BitScanForward64(&index, result)) return Option<i8>();
	return Option<i8>(static_cast<i8>(index));
}

static Option<i8> avx2FindFirstZeroSlot(const i8* buffer) {
	const __m256i* bufferVec = reinterpret_cast<const __m256i*>(buffer);
	const __m256i zeroVec = _mm256_set1_epi8(0);
	const u64 result =
		static_cast<u64>(_mm256_cmpeq_epi8_mask(bufferVec[0], zeroVec))
		| (static_cast<u64>(_mm256_cmpeq_epi8_mask(bufferVec[1], zeroVec)) << 32);
	unsigned long index;
	if (!_BitScanForward64(&index, result)) return Option<i8>();
	return Option<i8>(static_cast<i8>(index));
}

u64 gk::internal::hashMapSimdFindEqualHashCodesBitmask(const i8* buffer, PairHashBits hashCode)
{
	static FuncFindEqualHashCodeBitmask func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_HASHMAP_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::HashMap function loader]: Using AVX-512 hash equality comparison\n";
			}

			return avx512FindHashCode;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_HASHMAP_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::HashMap function loader]: Using AVX-2 hash equality comparison\n";
			}
			return avx2FindHashCode;
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load hashmap hash equality comparison function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();
	return func(buffer, hashCode);
}

Option<i8> gk::internal::firstAvailableGroupSlot(const i8* buffer)
{
	static FuncFindFirstZeroSlot func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_HASHMAP_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::HashMap function loader]: Using AVX-512 empty find check\n";
			}

			return avx512FindFirstZeroSlot;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_HASHMAP_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::HashMap function loader]: Using AVX-2 empty find check\n";
			}
			return avx2FindFirstZeroSlot;
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load hashmap empty find check function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(buffer);
}

Option<usize> gk::internal::bitscanForwardNext(u64* bitmask)
{
	unsigned long index;
	if (!_BitScanForward64(&index, *bitmask)) return Option<usize>();
	*bitmask = (*bitmask & ~(1ULL << index)) & 63;
	return Option<u64>(index);
}


#if GK_TYPES_LIB_TEST
#include <string>

template<typename Key, typename Value>
using HashMap = gk::HashMap<Key, Value>;

// Example of making a custom hash
template<>
size_t gk::hash<std::string>(const std::string& key) {
	std::hash<std::string> hasher;
	return hasher(key);
}

test_case("DefaultConstruct") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	check_eq(map.size(), 0);
}

test_case("InsertIntSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	map.insert(1, 5);
	check_eq(map.size(), 1);
}

test_case("InsertMultipleIntsSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(i, 100);
	}
	check_eq(map.size(), 20);
}

test_case("InsertStringSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	map.insert(std::string("hello"), 5);
	check_eq(map.size(), 1);
}

test_case("InsertMultipleStringsSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	check_eq(map.size(), 20);
}

test_case("SanityCheckCopyKeyAndValueSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		std::string numStr = std::to_string(i);
		int value = 100;
		map.insert(numStr, value);
	}
	check_eq(map.size(), 20);
}

test_case("SanityCheckCopyKeyAndMoveValueSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		std::string numStr = std::to_string(i);
		map.insert(numStr, 100);
	}
	check_eq(map.size(), 20);
}

test_case("SanityCheckMoveKeyAndCopyValueSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		int value = 100;
		map.insert(std::to_string(i), value);
	}
	check_eq(map.size(), 20);
}

test_case("InsertMultipleWithDuplicateKeysSize") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		int value = 100;
		map.insert(std::to_string(i), value);
	}
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	check_eq(map.size(), 20);
}

test_case("FindIntSize1") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	map.insert(5, 100);
	auto found = map.find(5);
	check_eq(*found.some(), 100);
}

test_case("DontFindIntSize1") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	map.insert(5, 100);
	auto found = map.find(13);
	check(found.none());
}

test_case("FindIntSizeMultiple") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(i, 100);
	}
	auto found = map.find(5);
	check_eq(*found.some(), 100);
}

test_case("DontFindIntSizeMultiple") {
	//MemoryLeakDetector leakDetector;
	HashMap<int, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(i, 100);
	}
	auto found = map.find(21);
	check(found.none());
}

test_case("FindStringSize1") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	map.insert(std::to_string(5), 100);
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("DontFindStringSize1") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	map.insert(std::to_string(5), 100);
	auto found = map.find("13");
	check(found.none());
}

test_case("FindStringSizeMultiple") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("DontFindStringSizeMultiple") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	auto found = map.find("21");
	check(found.none());
}

test_case("ConstFindStringSize1") {
	//MemoryLeakDetector leakDetector;
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> outMap;
		outMap.insert(std::to_string(5), 100);
		return outMap;
	}();
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("ConstDontFindStringSize1") {
	//MemoryLeakDetector leakDetector;
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> outMap;
		outMap.insert(std::to_string(5), 100);
		return outMap;
	}();
	auto found = map.find("13");
	check(found.none());
}

test_case("ConstFindStringSizeMultiple") {
	//MemoryLeakDetector leakDetector;
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> outMap;
		for (int i = 0; i < 20; i++) {
			outMap.insert(std::to_string(i), 100);
		}
		return outMap;
	}();
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("ConstDontFindStringSizeMultiple") {
	//MemoryLeakDetector leakDetector;
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> outMap;
		for (int i = 0; i < 20; i++) {
			outMap.insert(std::to_string(i), 100);
		}
		return outMap;
	}();
	auto found = map.find("21");
	check(found.none());
}

test_case("EraseSingleElement") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	map.insert(std::to_string(5), 100);
	map.erase(std::to_string(5));
	auto found = map.find("5");
	check(found.none());
	check_eq(map.size(), 0);
}

test_case("EraseSingleElementFromMultiple") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	map.erase(std::to_string(5));
	auto found = map.find("5");
	check(found.none());
	check_eq(map.size(), 19);
}

test_case("EraseAllElements") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	for (int i = 0; i < 20; i++) {
		std::string numStr = std::to_string(i);
		auto found = map.find(numStr);
		check_eq(*found.some(), 100);
		map.erase(std::to_string(i));
		auto found2 = map.find(numStr);
		check(found2.none());
	}
	for (int i = 0; i < 20; i++) {
		auto found = map.find(std::to_string(i));
		check(found.none());
	}
	check_eq(map.size(), 0);
}

test_case("Reserve") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	map.reserve(100);
	check_eq(map.size(), 0);
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	check_eq(map.size(), 100);
}

test_case("Iterator") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	int iterCount = 0;
	for (auto pair : map) {
		iterCount++;
		check_eq(*pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("ConstIterator") {
	//MemoryLeakDetector leakDetector;
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> mapOut;
		for (int i = 0; i < 100; i++) {
			mapOut.insert(std::to_string(i), -1);
		}
		return mapOut;
	}();
	int iterCount = 0;
	for (auto pair : map) {
		iterCount++;
		check_eq(*pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("CopyConstruct") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	HashMap<std::string, int> otherMap = map;
	check_eq(otherMap.size(), 100);
	int iterCount = 0;
	for (auto pair : otherMap) {
		iterCount++;
		check_eq(*pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("MoveConstruct") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	HashMap<std::string, int> otherMap = std::move(map);
	check_eq(otherMap.size(), 100);
	int iterCount = 0;
	for (auto pair : otherMap) {
		iterCount++;
		check_eq(*pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("CopyAssign") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	HashMap<std::string, int> otherMap;
	otherMap.insert("hi", 1);
	otherMap = map;
	check_eq(otherMap.size(), 100);
	int iterCount = 0;
	for (auto pair : otherMap) {
		iterCount++;
		check_eq(*pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("MoveAssign") {
	//MemoryLeakDetector leakDetector;
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	HashMap<std::string, int> otherMap;
	otherMap.insert("hi", 1);
	otherMap = std::move(map);
	check_eq(otherMap.size(), 100);
	int iterCount = 0;
	for (auto pair : otherMap) {
		iterCount++;
		check_eq(*pair.value, -1);
	}
	check_eq(iterCount, 100);
}

#endif


