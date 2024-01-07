#include "array_list.h"
#include "../cpu_features/cpu_feature_detector.h"
#include <intrin.h>

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanForward)

constexpr bool SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS = false;

template<typename T>
using Option = gk::Option<T>;
using gk::usize;
using gk::i8;
using gk::i16;
using gk::i32;
using gk::i64;
using gk::u8;
using gk::u16;
using gk::u32;
using gk::u64;

using Find1ByteInArrayListFunc = gk::Option<usize>(*)(const i8*, usize, i8);
using Find2ByteInArrayListFunc = gk::Option<usize>(*)(const i16*, usize, i16);
using Find4ByteInArrayListFunc = gk::Option<usize>(*)(const i32*, usize, i32);
using Find8ByteInArrayListFunc = gk::Option<usize>(*)(const i64*, usize, i64);

static gk::Option<usize> avx512Find1ByteInArrayList(const i8* arrayListData, usize length, i8 toFind) {
	constexpr usize NUM_PER_VEC = 64;

	const __m512i findVec = _mm512_set1_epi8(toFind);
	const __m512i* arrayListVec = reinterpret_cast<const __m512i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u64 bitmask = _mm512_cmpeq_epi8_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward64(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx2Find1ByteInArrayList(const i8* arrayListData, usize length, i8 toFind) {
	constexpr usize NUM_PER_VEC = 32;

	const __m256i findVec = _mm256_set1_epi8(toFind);
	const __m256i* arrayListVec = reinterpret_cast<const __m256i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm256_cmpeq_epi8_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx512Find2ByteInArrayList(const i16* arrayListData, usize length, i16 toFind) {
	constexpr usize NUM_PER_VEC = 32;

	const __m512i findVec = _mm512_set1_epi16(toFind);
	const __m512i* arrayListVec = reinterpret_cast<const __m512i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm512_cmpeq_epi16_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx2Find2ByteInArrayList(const i16* arrayListData, usize length, i16 toFind) {
	constexpr usize NUM_PER_VEC = 16;

	const __m256i findVec = _mm256_set1_epi16(toFind);
	const __m256i* arrayListVec = reinterpret_cast<const __m256i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm256_cmpeq_epi16_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx512Find4ByteInArrayList(const i32* arrayListData, usize length, i32 toFind) {
	constexpr usize NUM_PER_VEC = 16;

	const __m512i findVec = _mm512_set1_epi32(toFind);
	const __m512i* arrayListVec = reinterpret_cast<const __m512i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm512_cmpeq_epi32_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx2Find4ByteInArrayList(const i32* arrayListData, usize length, i32 toFind) {
	constexpr usize NUM_PER_VEC = 8;

	const __m256i findVec = _mm256_set1_epi32(toFind);
	const __m256i* arrayListVec = reinterpret_cast<const __m256i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm256_cmpeq_epi32_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx512Find8ByteInArrayList(const i64* arrayListData, usize length, i64 toFind) {
	constexpr usize NUM_PER_VEC = 8;

	const __m512i findVec = _mm512_set1_epi64(toFind);
	const __m512i* arrayListVec = reinterpret_cast<const __m512i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm512_cmpeq_epi64_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

static gk::Option<usize> avx2Find8ByteInArrayList(const i64* arrayListData, usize length, i64 toFind) {
	constexpr usize NUM_PER_VEC = 4;

	const __m256i findVec = _mm256_set1_epi64x(toFind);
	const __m256i* arrayListVec = reinterpret_cast<const __m256i*>(arrayListData);
	const usize iterationToDo = (length % NUM_PER_VEC == 0 ? length : length + (NUM_PER_VEC - (length % NUM_PER_VEC))) / NUM_PER_VEC;
	for (usize i = 0; i < iterationToDo; i++) {
		const u32 bitmask = _mm256_cmpeq_epi64_mask(findVec, arrayListVec[i]);

		unsigned long index;
		if (_BitScanForward(&index, bitmask) == 0) {
			continue;
		}
		const usize actualIndex = (i * NUM_PER_VEC) + index;
		if (index >= length) {
			return gk::Option<usize>(); // out of range
		}
		return gk::Option<usize>(actualIndex);
	}
	return gk::Option<usize>(); // didnt find
}

Option<usize> gk::internal::doSimdArrayElementFind1Byte(const i8* arrayListData, usize length, i8 toFind)
{
	static Find1ByteInArrayListFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-512 1 byte find\n";
			}
			return avx512Find1ByteInArrayList;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-2 1 byte find\n";
			}
			return avx2Find1ByteInArrayList; // CHANGE THIS
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load array list 1 byte find function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(arrayListData, length, toFind);
}

Option<usize> gk::internal::doSimdArrayElementFind2Byte(const i16* arrayListData, usize length, i16 toFind)
{
	static Find2ByteInArrayListFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-512 2 byte find\n";
			}
			return avx512Find2ByteInArrayList;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-2 2 byte find\n";
			}
			return avx2Find2ByteInArrayList; // CHANGE THIS
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load array list 2 byte find function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(arrayListData, length, toFind);
}

Option<usize> gk::internal::doSimdArrayElementFind4Byte(const i32* arrayListData, usize length, i32 toFind)
{
	static Find4ByteInArrayListFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-512 4 byte find\n";
			}
			return avx512Find4ByteInArrayList;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-2 4 byte find\n";
			}
			return avx2Find4ByteInArrayList; // CHANGE THIS
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load array list 4 byte find function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(arrayListData, length, toFind);
}

Option<usize> gk::internal::doSimdArrayElementFind8Byte(const i64* arrayListData, usize length, i64 toFind)
{
	static Find8ByteInArrayListFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-512 8 byte find\n";
			}
			return avx512Find8ByteInArrayList;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (SHOULD_LOG_ARRAYLIST_DYNAMICALLY_LOADED_FUNCTIONS) {
				std::cout << "[gk::ArrayList function loader]: Using AVX-2 8 byte find\n";
			}
			return avx2Find8ByteInArrayList; // CHANGE THIS
		}
		else {
			std::cout << "[gk::HashMap function loader]: ERROR\nCannot load array list 8 byte find function if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(arrayListData, length, toFind);
}

#if GK_TYPES_LIB_TEST
#include <string>

template<typename T>
using ArrayList = gk::ArrayList<T>;

using gk::globalHeapAllocator;
using gk::AllocatorRef;

test_case("Default Construct") {
	ArrayList<int> a;
	check_eq(a.len(), 0);
	check_eq(a.capacity(), 0);
	check_eq(a.data(), nullptr); // ensure no allocation when not necessary
	check_eq(a.allocator(), globalHeapAllocator());
}

comptime_test_case(default_construct, {
	ArrayList<int> a;
	check_eq(a.len(), 0);
	check_eq(a.capacity(), 0);
	check_eq(a.data(), nullptr); // ensure no allocation when not necessary
})

test_case("Copy Construct") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = a;
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
}

comptime_test_case(CopyConstruct, {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = a;
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
});

test_case("Move Construct") {
	ArrayList<int> a;
	a.push(1);
	const int* oldPtr = a.data();
	ArrayList<int> b = std::move(a);
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
	check_eq(b.data(), oldPtr);
}

comptime_test_case(MoveConstruct, {
	ArrayList<int> a;
	a.push(1);
	const int* oldPtr = a.data();
	ArrayList<int> b = std::move(a);
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
	check_eq(b.data(), oldPtr);
});

test_case("Copy Assign") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = a;
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
}

comptime_test_case(CopyAssign, {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = a;
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
});

test_case("Move Assign") {
	ArrayList<int> a;
	a.push(1);
	const int* oldPtr = a.data();
	ArrayList<int> b;
	b.push(5);
	b = std::move(a);
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
	check_eq(b.data(), oldPtr);
}

comptime_test_case(MoveAssign, {
	ArrayList<int> a;
	a.push(1);
	const int* oldPtr = a.data();
	ArrayList<int> b;
	b.push(5);
	b = std::move(a);
	check_eq(b[0], 1);
	check_eq(b.len(), 1);
	check(b.capacity() > 0);
	check_eq(b.data(), oldPtr);
});

test_case("Init") {
	ArrayList<int> a = ArrayList<int>::init(globalHeapAllocator());
	a.push(1);
	check_eq(a[0], 1);
}

test_case("Init And Copy") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = ArrayList<int>::initCopy(globalHeapAllocator(), a);
	check_eq(b[0], 1);
}

test_case("Init And Initialize List") {
	ArrayList<int> a = ArrayList<int>::initList(globalHeapAllocator(), { 0, 1, 2 });
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("Init And Ptr") {
	int buf[] = { 0, 1, 2 };
	ArrayList<int> a = ArrayList<int>::initBufferCopy(globalHeapAllocator(), buf, 3);
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("With Capacity") {
	ArrayList<int> a = ArrayList<int>::withCapacity(globalHeapAllocator(), 10);
	a.push(1);
	check(a.capacity() >= 10);
	check_eq(a[0], 1);
}

test_case("With Capacity Copy") {
	ArrayList<int> a;
	a.push(1);
	ArrayList<int> b = ArrayList<int>::withCapacityCopy(globalHeapAllocator(), 10, a);
	check(a.capacity() >= 10);
	check_eq(b[0], 1);
}

test_case("With Capacity Initializer List") {
	ArrayList<int> a = ArrayList<int>::withCapacityList(globalHeapAllocator(), 10, { 0, 1, 2 });
	check(a.capacity() >= 10);
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("With Capacity Ptr") {
	int buf[] = { 0, 1, 2 };
	ArrayList<int> a = ArrayList<int>::withCapacityBufferCopy(globalHeapAllocator(), 10, buf, 3);
	check(a.capacity() >= 10);
	check_eq(a[0], 0);
	check_eq(a[1], 1);
	check_eq(a[2], 2);
}

test_case("Push std::strings") {
	ArrayList<std::string> a;
	const std::string first = "hello";
	a.push(first);
	a.push("world");
	a.push("it");
	a.push("is");
	a.push("me");
	a.push("how");
	a.push("are");
	a.push("you");
	check_eq(a[0], first);
	check_eq(a[1], "world");
	check_eq(a[2], "it");
	check_eq(a[3], "is");
	check_eq(a[4], "me");
	check_eq(a[5], "how");
	check_eq(a[6], "are");
	check_eq(a[7], "you");
}

comptime_test_case(PushStdStrings, {
	ArrayList<std::string> a;
	const std::string first = "hello";
	a.push(first);
	a.push("world");
	a.push("it");
	a.push("is");
	a.push("me");
	a.push("how");
	a.push("are");
	a.push("you");
	check_eq(a[0], first);
	check_eq(a[1], "world");
	check_eq(a[2], "it");
	check_eq(a[3], "is");
	check_eq(a[4], "me");
	check_eq(a[5], "how");
	check_eq(a[6], "are");
	check_eq(a[7], "you");
});

test_case("Reserve") {
	ArrayList<int> a;
	a.push(0);
	a.reserve(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

comptime_test_case(Reserve, {
	ArrayList<int> a;
	a.push(0);
	a.reserve(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
});

test_case("Reserve From Empty") {
	ArrayList<int> a;
	a.reserve(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 0);
}

comptime_test_case(ReserveFromEmpty, {
	ArrayList<int> a;
	a.reserve(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 0);
});

test_case("Reserve Zero") {
	ArrayList<int> a;
	a.push(0);
	a.reserve(0);
	check(a.capacity() > 0);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

comptime_test_case(ReserveZero, {
	ArrayList<int> a;
	a.push(0);
	a.reserve(0);
	check(a.capacity() > 0);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
});

test_case("Reserve Tiny") {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	a.reserve(1);
	check(a.capacity() >= 10);
	check_eq(a.len(), 10);
	check_eq(a[0], 0);
}

comptime_test_case(ReserveTiny, {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	a.reserve(1);
	check(a.capacity() >= 10);
	check_eq(a.len(), 10);
	check_eq(a[0], 0);
});

test_case("Reserve Exact") {
	ArrayList<int> a;
	a.push(0);
	a.reserveExact(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

comptime_test_case(ReserveExact, {
	ArrayList<int> a;
	a.push(0);
	a.reserveExact(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
});

test_case("Reserve Exact From Empty") {
	ArrayList<int> a;
	a.reserveExact(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 0);
}

comptime_test_case(ReserveExactFromEmpty, {
	ArrayList<int> a;
	a.reserveExact(100);
	check(a.capacity() >= 100);
	check_eq(a.len(), 0);
});

test_case("Reserve Exact Zero") {
	ArrayList<int> a;
	a.push(0);
	a.reserveExact(0);
	check(a.capacity() > 0);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
}

comptime_test_case(ReserveExactZero, {
	ArrayList<int> a;
	a.push(0);
	a.reserveExact(0);
	check(a.capacity() > 0);
	check_eq(a.len(), 1);
	check_eq(a[0], 0);
});

test_case("Reserve Tiny Exact") {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	a.reserveExact(1);
	check(a.capacity() >= 10);
	check_eq(a.len(), 10);
	check_eq(a[0], 0);
}

comptime_test_case(ReserveTinyExact, {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	a.reserveExact(1);
	check(a.capacity() >= 10);
	check_eq(a.len(), 10);
	check_eq(a[0], 0);
});

test_case("find_empty") {
	ArrayList<int> a;
	check(a.find(1).none());
}

comptime_test_case(find_empty, {
	ArrayList<int> a;
	check(a.find(1).none());
});

test_case("find_at_index_0") {
	ArrayList<int> a;
	a.push(50);
	check(a.find(50).isSome());
	check_eq(a.find(50).some(), 0);
}

comptime_test_case(find_at_index_0, {
	ArrayList<int> a;
	a.push(50);
	check(a.find(50).isSome());
	check_eq(a.find(50).some(), 0);
});

test_case("find_random_location") {
	ArrayList<int> a = ArrayList<int>::initList(gk::globalHeapAllocatorRef(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	check(a.find(5).isSome());
	check_eq(a.find(5).some(), 5);
}

comptime_test_case(find_random_location, {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	check(a.find(5).isSome());
	check_eq(a.find(5).some(), 5);
});

test_case("find_end") {
	ArrayList<int> a = ArrayList<int>::initList(gk::globalHeapAllocatorRef(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	check(a.find(9).isSome());
	check_eq(a.find(9).some(), 9);
}

comptime_test_case(find_end, {
	ArrayList<int> a;
	for (int i = 0; i < 10; i++) {
		a.push(i);
	}
	check(a.find(9).isSome());
	check_eq(a.find(9).some(), 9);
});

static constexpr void testArrayListRemoveContainsOneElement() {
	ArrayList<int> a;
	a.push(50);
	check_eq(a.remove(0), 50);
	check_eq(a.len(), 0);
}

test_case("remove contains one element") {
	testArrayListRemoveContainsOneElement();
}

comptime_test_case(remove_contains_one_element, {
	testArrayListRemoveContainsOneElement();
});

static constexpr void testArrayListRemoveLastElement() {
	ArrayList<int> a;
	a.push(10);
	a.push(20);
	a.push(30);
	a.push(40);
	a.push(50);
	check_eq(a.remove(4), 50);
	check_eq(a.len(), 4);
	check_eq(a[0], 10);
	check_eq(a[1], 20);
	check_eq(a[2], 30);
	check_eq(a[3], 40);
}

test_case("remove last element") {
	testArrayListRemoveLastElement();
}

comptime_test_case(remove_last_element, {
	testArrayListRemoveLastElement();
});

static constexpr void testArrayListRemoveMiddleElement() {
	ArrayList<int> a;
	a.push(10);
	a.push(20);
	a.push(30);
	a.push(40);
	a.push(50);
	check_eq(a.remove(2), 30);
	check_eq(a.len(), 4);
	check_eq(a[0], 10);
	check_eq(a[1], 20);
	check_eq(a[2], 40);
	check_eq(a[3], 50);
}

test_case("remove middle element") {
	testArrayListRemoveMiddleElement();
}

comptime_test_case(remove_middle_element, {
	testArrayListRemoveMiddleElement();
	});

#endif


