#include "hashmap.h"
#include <intrin.h>
#include "../cpu_features/cpu_feature_detector.h"

using gk::Option;
using gk::usize;
using gk::u64;

Option<usize> gk::internal::firstAvailableGroupSlot16(const i8* buffer, usize capacity)
{
	// can use 128bit SIMD instructions
	check_message(capacity % 16 == 0, "capacity must be a multiple of 16");
	check_message(usize(buffer) % 16 == 0, "buffer must be 16 byte aligned");

	const __m128i zeroVec = _mm_set1_epi8(0);
	const __m128i* bufferVec = reinterpret_cast<const __m128i*>(buffer);

	const usize iterationCount = capacity / 16;
	for (usize i = 0; i < iterationCount; i++) {
		u64 bitmask = _mm_cmpeq_epi8_mask(zeroVec, bufferVec[i]);

		Option<usize> index = bitscanForwardNext(&bitmask);
		if (index.none()) {
			continue;
		}
		return Option<usize>(index.someCopy() + (i * 16));
	}
	return Option<usize>();
}

Option<usize> gk::internal::firstAvailableGroupSlot32(const i8* buffer, usize capacity)
{
	// can use 256bit AVX2 instructions
	check_message(capacity % 32 == 0, "capacity must be a multiple of 32");
	check_message(usize(buffer) % 32 == 0, "buffer must be 32 byte aligned");

	const __m256i zeroVec = _mm256_set1_epi8(0);
	const __m256i* bufferVec = reinterpret_cast<const __m256i*>(buffer);

	const usize iterationCount = capacity / 32;
	for (usize i = 0; i < iterationCount; i++) {
		u64 bitmask = _mm256_cmpeq_epi8_mask(zeroVec, bufferVec[i]);

		Option<usize> index = bitscanForwardNext(&bitmask);
		if (index.none()) {
			continue;
		}
		return Option<usize>(index.someCopy() + (i * 32));
	}
	return Option<usize>();
}

static Option<usize> hashAvx512FindFirstAvailableGroupSlot64(const gk::i8* buffer, gk::usize capacity)
{
	using namespace gk;

	const __m512i zeroVec = _mm512_set1_epi8(0);
	const __m512i* bufferVec = reinterpret_cast<const __m512i*>(buffer);

	const usize iterationCount = capacity / 32;
	for (usize i = 0; i < iterationCount; i++) {
		u64 bitmask = _mm512_cmpeq_epi8_mask(zeroVec, bufferVec[i]);

		Option<usize> index = bitscanForwardNext(&bitmask);
		if (index.none()) {
			continue;
		}
		return Option<usize>(index.someCopy() + (i * 64));
	}
	return Option<usize>();
}

Option<usize> gk::internal::firstAvailableGroupSlot64(const i8* buffer, usize capacity)
{
	// dynamic dispatch for either 512bit AVX512, or 256bit AVX2 instructions
	check_message(capacity % 64 == 0, "capacity must be a multiple of 64");
	check_message(usize(buffer) % 64 == 0, "buffer must be 64 byte aligned");

	typedef Option<usize>(*HashFuncFindFirstZeroSlot64Byte)(const i8*, const usize);

	HashFuncFindFirstZeroSlot64Byte func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (true) {
				std::cout << "[gk::HashMap function loader]: Using AVX-512 hash empty find\n";
			}

			return hashAvx512FindFirstAvailableGroupSlot64;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (true) {
				std::cout << "[gk::HashMap function loader]: Using AVX-2 hash empty find\n";
			}
			return firstAvailableGroupSlot32;
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load hashmap hash empty find function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(buffer, capacity);
}

u64 gk::internal::simdFindEqualHashCodeBitmaskIteration16(const i8* bufferOffsetIter, HashMapPairBitmask hashBits)
{
	check_message(usize(bufferOffsetIter) % 16 == 0, "buffer must be 16 byte aligned");

	const __m128i hashCodeVec = _mm_set1_epi8(hashBits.value);
	const __m128i bufferVec = *reinterpret_cast<const __m128i*>(bufferOffsetIter);
	return _mm_cmpeq_epi8_mask(hashCodeVec, bufferVec);
}

u64 gk::internal::simdFindEqualHashCodeBitmaskIteration32(const i8* bufferOffsetIter, HashMapPairBitmask hashBits)
{
	check_message(usize(bufferOffsetIter) % 32 == 0, "buffer must be 32 byte aligned");

	const __m256i hashCodeVec = _mm256_set1_epi8(hashBits.value);
	const __m256i bufferVec = *reinterpret_cast<const __m256i*>(bufferOffsetIter);
	return _mm256_cmpeq_epi8_mask(hashCodeVec, bufferVec);
}

static u64 avx512FindEqualHashCodeBitmaskIteration64(const gk::i8* bufferOffsetIter, gk::internal::HashMapPairBitmask hashBits) {
	check_message(gk::usize(bufferOffsetIter) % 64 == 0, "buffer must be 64 byte aligned");

	const __m512i hashCodeVec = _mm512_set1_epi8(hashBits.value);
	const __m512i bufferVec = *reinterpret_cast<const __m512i*>(bufferOffsetIter);
	return _mm512_cmpeq_epi8_mask(hashCodeVec, bufferVec);
}

static u64 avx2FindEqualHashCodeBitmaskIteration64(const gk::i8* bufferOffsetIter, gk::internal::HashMapPairBitmask hashBits) {
	check_message(gk::usize(bufferOffsetIter) % 64 == 0, "buffer must be 64 byte aligned");

	const __m256i hashCodeVec = _mm256_set1_epi8(hashBits.value);
	const __m256i* bufferVec = reinterpret_cast<const __m256i*>(bufferOffsetIter);
	return static_cast<gk::u64>(_mm256_cmpeq_epi8_mask(hashCodeVec, bufferVec[0])) |
		(static_cast<gk::u64>(_mm256_cmpeq_epi8_mask(hashCodeVec, bufferVec[1])) << 32);
}


u64 gk::internal::simdFindEqualHashCodeBitmaskIteration64(const i8* bufferOffsetIter, HashMapPairBitmask hashBits)
{
	typedef u64(*HashFuncFindEqualHashBitmaskIteration)(const i8*, HashMapPairBitmask);

	static HashFuncFindEqualHashBitmaskIteration func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (true) {
				std::cout << "[gk::HashMap function loader]: Using AVX-512 hash equality comparison\n";
			}

			return avx512FindEqualHashCodeBitmaskIteration64;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (true) {
				std::cout << "[gk::HashMap function loader]: Using AVX-2 hash equality comparison\n";
			}
			return avx2FindEqualHashCodeBitmaskIteration64;
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load hashmap hash equality comparison function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(bufferOffsetIter, hashBits);
}

#if GK_TYPES_LIB_TEST
#include <string>

template<typename Key, typename Value>
using HashMap = gk::HashMap<Key, Value, 32>;

// Example of making a custom hash
template<>
static size_t gk::hash<std::string>(const std::string& key) {
	std::hash<std::string> hasher;
	return hasher(key);
}

static constexpr void hashMapDefaultConstruct() {
	HashMap<int, int> map;
	check_eq(map.size(), 0);
}

test_case("DefaultConstruct") {
	hashMapDefaultConstruct();
}

comptime_test_case(hashmap, DefaultConstruct, {
	hashMapDefaultConstruct();
});

static constexpr void hashMapInsertIntSize() {
	HashMap<int, int> map;
	map.insert(1, 5);
	check_eq(map.size(), 1);
}

test_case("InsertIntSize") {
	hashMapInsertIntSize();
}

comptime_test_case(hashmap, InsertIntSize, {
	hashMapInsertIntSize();
});

static constexpr void hashMapInsertMultipleIntsSize() {
	HashMap<int, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(i, 100);
	}
	check_eq(map.size(), 20);
}

test_case("InsertMultipleIntsSize") {
	hashMapInsertMultipleIntsSize();
}

comptime_test_case(hashmap, InsertMultipleIntsSize, {
	hashMapInsertMultipleIntsSize();
});

test_case("InsertStringSize") {
	HashMap<std::string, int> map;
	map.insert(std::string("hello"), 5);
	check_eq(map.size(), 1);
}

test_case("InsertMultipleStringsSize") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	check_eq(map.size(), 20);
}

test_case("SanityCheckCopyKeyAndValueSize") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		std::string numStr = std::to_string(i);
		int value = 100;
		map.insert(numStr, value);
	}
	check_eq(map.size(), 20);
}

test_case("SanityCheckCopyKeyAndMoveValueSize") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		std::string numStr = std::to_string(i);
		map.insert(numStr, 100);
	}
	check_eq(map.size(), 20);
}

test_case("SanityCheckMoveKeyAndCopyValueSize") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		int value = 100;
		map.insert(std::to_string(i), value);
	}
	check_eq(map.size(), 20);
}

test_case("InsertMultipleWithDuplicateKeysSize") {
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

static constexpr void testHashMapFindIntSize1() {
	HashMap<int, int> map;
	map.insert(5, 100);
	auto found = map.find(5);
	check_eq(*found.some(), 100);
}

test_case("FindIntSize1") {
	testHashMapFindIntSize1();
}

comptime_test_case(hashmap, FindIntSize1, {
	testHashMapFindIntSize1();
});

static constexpr void testhashMapDontFindIntSize1() {
	HashMap<int, int> map;
	map.insert(5, 100);
	auto found = map.find(13);
	check(found.none());
}

test_case("DontFindIntSize1") {
	testhashMapDontFindIntSize1();
}

comptime_test_case(hashmap, DontFindIntSize1, {
	testhashMapDontFindIntSize1();
});

static constexpr void testHashMapFindIntSizeMultiple() {
	HashMap<int, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(i, 100);
	}
	auto found = map.find(5);
	check_eq(*found.some(), 100);
}

test_case("FindIntSizeMultiple") {
	testHashMapFindIntSizeMultiple();
}

comptime_test_case(hashmap, FindIntSizeMultiple, {
	testHashMapFindIntSizeMultiple();
});

static constexpr void testHashMapDontFindIntSizeMultiple() {
	HashMap<int, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(i, 100);
	}
	auto found = map.find(21);
	check(found.none());
}

test_case("DontFindIntSizeMultiple") {
	testHashMapDontFindIntSizeMultiple();
}

comptime_test_case(hashmap, DontFindIntSizeMultiple, {
	testHashMapFindIntSizeMultiple();
});

test_case("FindStringSize1") {
	HashMap<std::string, int> map;
	map.insert(std::to_string(5), 100);
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("DontFindStringSize1") {
	HashMap<std::string, int> map;
	map.insert(std::to_string(5), 100);
	auto found = map.find("13");
	check(found.none());
}

test_case("FindStringSizeMultiple") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("DontFindStringSizeMultiple") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 20; i++) {
		map.insert(std::to_string(i), 100);
	}
	auto found = map.find("21");
	check(found.none());
}

test_case("ConstFindStringSize1") {
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> outMap;
		outMap.insert(std::to_string(5), 100);
		return outMap;
	}();
	auto found = map.find("5");
	check_eq(*found.some(), 100);
}

test_case("ConstDontFindStringSize1") {
	const HashMap<std::string, int> map = []() {
		HashMap<std::string, int> outMap;
		outMap.insert(std::to_string(5), 100);
		return outMap;
	}();
	auto found = map.find("13");
	check(found.none());
}

test_case("ConstFindStringSizeMultiple") {
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
	HashMap<std::string, int> map;
	map.insert(std::to_string(5), 100);
	map.erase(std::to_string(5));
	auto found = map.find("5");
	check(found.none());
	check_eq(map.size(), 0);
}

test_case("EraseSingleElementFromMultiple") {
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
	HashMap<std::string, int> map;
	map.reserve(100);
	check_eq(map.size(), 0);
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	check_eq(map.size(), 100);
}

test_case("Iterator") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	int iterCount = 0;
	for (auto pair : map) {
		iterCount++;
		check_eq(pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("ConstIterator") {
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
		check_eq(pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("CopyConstruct") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	HashMap<std::string, int> otherMap = map;
	check_eq(otherMap.size(), 100);
	int iterCount = 0;
	for (auto pair : otherMap) {
		iterCount++;
		check_eq(pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("MoveConstruct") {
	HashMap<std::string, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(std::to_string(i), -1);
	}
	HashMap<std::string, int> otherMap = std::move(map);
	check_eq(otherMap.size(), 100);
	int iterCount = 0;
	for (auto pair : otherMap) {
		iterCount++;
		check_eq(pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("CopyAssign") {
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
		check_eq(pair.value, -1);
	}
	check_eq(iterCount, 100);
}

test_case("MoveAssign") {
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
		check_eq(pair.value, -1);
	}
	check_eq(iterCount, 100);
}

#endif