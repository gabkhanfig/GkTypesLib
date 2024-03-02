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
#include "../allocator/testing_allocator.h"
#include "../ptr/unique_ptr.h"

using gk::ArrayList;
using gk::ArrayListUnmanaged;

using gk::IAllocator;
using gk::globalHeapAllocator;
using gk::AllocatorRef;
using gk::TestingAllocator;

using gk::UniquePtr;

/// Makes an instance of TestingAllocator to use for runtime, or nullptr for compile time tests.
static constexpr UniquePtr<IAllocator> makeTestingAllocator() {
	if (std::is_constant_evaluated()) {
		return UniquePtr<IAllocator>::null();
	}
	else {
		TestingAllocator* newAllocator = new TestingAllocator();
		return UniquePtr<IAllocator>(newAllocator);
	}
}

static constexpr void defaultConstruct() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		ArrayListUnmanaged<int> a;
		check_eq(a.len(), 0);
		check_eq(a.capacity(), 0);
		check_eq(a.data(), nullptr); // ensure no allocation when not necessary
	}
	{
		{
			ArrayList<int> a;
			check_eq(a.len(), 0);
			check_eq(a.capacity(), 0);
			check_eq(a.data(), nullptr); // ensure no allocation when not necessary
			if (!std::is_constant_evaluated()) {
				check_eq(a.allocator(), globalHeapAllocator());
			}
		}
		if (!std::is_constant_evaluated()) {
			ArrayList<int> a = ArrayList<int>::init(allocator->toRef()); check_eq(a.len(), 0);
			check_eq(a.capacity(), 0);
			check_eq(a.data(), nullptr); // ensure no allocation when not necessary
			check_eq(a.allocator(), allocator.get());
		}	
	}
}

test_case("default construct") { defaultConstruct(); }
comptime_test_case(default_construct, { defaultConstruct(); })

static constexpr void pushOneElement() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		ArrayListUnmanaged<int> a;
		auto res = a.push(allocator.get(), 0);
		check(res.isOk());
		check_eq(a[0], 0);
		check_eq(a.len(), 1);
		check_ne(a.capacity(), 0);
		check_ne(a.data(), nullptr);
		a.deinit(allocator.get());
	}
	{
		{
			ArrayList<int> a;
			a.push(0);
			check_eq(a[0], 0);
			check_eq(a.len(), 1);
			check_ne(a.capacity(), 0);
			check_ne(a.data(), nullptr);
		}
		if (!std::is_constant_evaluated()) {
			ArrayList<int> a = ArrayList<int>::init(allocator->toRef());
			a.push(0);
			check_eq(a[0], 0);
			check_eq(a.len(), 1);
			check_ne(a.capacity(), 0);
			check_ne(a.data(), nullptr);
		}
	}
}

test_case("push one element") { pushOneElement(); }
comptime_test_case(push_one_element, { pushOneElement(); })

static constexpr void moveSemantics() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		{
			ArrayListUnmanaged<int> a;
			(void)a.push(allocator.get(), 5);
			const int* oldPtr = a.data();

			ArrayListUnmanaged<int> b = std::move(a);

			check_eq(b.data(), oldPtr);
			check_eq(b[0], 5);
			check_eq(b.len(), 1);
			check_ne(b.capacity(), 0);

			b.deinit(allocator.get());
		}
		{
			ArrayListUnmanaged<int> a;
			(void)a.push(allocator.get(), 5);
			const int* oldPtr = a.data();

			ArrayListUnmanaged<int> b;
			(void)b.push(allocator.get(), 5);
			b.deinit(allocator.get());
			b = std::move(a);

			check_eq(b.data(), oldPtr);
			check_eq(b[0], 5);
			check_eq(b.len(), 1);
			check_ne(b.capacity(), 0);

			b.deinit(allocator.get());
		}
	}
	{
		{
			ArrayList<int> a;
			a.push(1);
			const int* oldPtr = a.data();

			ArrayList<int> b = std::move(a);

			check_eq(b[0], 1);
			check_eq(b.len(), 1);
			check(b.capacity() > 0);
			check_eq(b.data(), oldPtr);
		}
		{
			ArrayList<int> a;
			a.push(1);
			const int* oldPtr = a.data();

			ArrayList<int> b;
			b.push(1);
			b = std::move(a);

			check_eq(b[0], 1);
			check_eq(b.len(), 1);
			check(b.capacity() > 0);
			check_eq(b.data(), oldPtr);
		}
		if (!std::is_constant_evaluated()) {
			{
				ArrayList<int> a = ArrayList<int>::init(allocator->toRef());
				a.push(1);
				const int* oldPtr = a.data();

				ArrayList<int> b = std::move(a);

				check_eq(b[0], 1);
				check_eq(b.len(), 1);
				check(b.capacity() > 0);
				check_eq(b.data(), oldPtr);
			}
			{
				ArrayList<int> a = ArrayList<int>::init(allocator->toRef());
				a.push(1);
				const int* oldPtr = a.data();

				ArrayList<int> b;
				b.push(1);
				b = std::move(a);

				check_eq(b[0], 1);
				check_eq(b.len(), 1);
				check(b.capacity() > 0);
				check_eq(b.data(), oldPtr);
			}
		}
	}
}

test_case("move semantics") { moveSemantics(); }
comptime_test_case(move_semantics, { moveSemantics(); })

static constexpr void copySemantics() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		ArrayListUnmanaged<int> a;
		(void)a.push(allocator.get(), 5);
		const int* aPtr = a.data();

		ArrayListUnmanaged<int> b = a.clone(allocator.get()).ok();
		const int* bPtr = b.data();

		check_ne(aPtr, bPtr);
		check_eq(a[0], b[0]);

		a.deinit(allocator.get());
		b.deinit(allocator.get());
	}
	{
		ArrayListUnmanaged<int> a;
		(void)a.push(allocator.get(), 5);
		const int* aPtr = a.data();

		ArrayListUnmanaged<int> b;
		b.push(allocator.get(), 5).ok();
		b.deinit(allocator.get());
		b = a.clone(allocator.get()).ok(); // basically just move assignment
		const int* bPtr = b.data();

		check_ne(aPtr, bPtr);
		check_eq(a[0], b[0]);

		a.deinit(allocator.get());
		b.deinit(allocator.get());
	}
	{
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b = a;
		check_eq(b[0], 1);
		check_eq(b.len(), 1);
		check(b.capacity() > 0);
	}
	{
		ArrayList<int> a;
		a.push(1);
		ArrayList<int> b;
		b = a;
		check_eq(b[0], 1);
		check_eq(b.len(), 1);
		check(b.capacity() > 0);
	}
	{
		if (!std::is_constant_evaluated()) {
			ArrayList<int> a;
			a.push(1);
			ArrayList<int> b = ArrayList<int>::initCopy(allocator->toRef(), a);
			check_eq(b[0], 1);
		}
	}
}

test_case("copy semantics") { copySemantics(); }
comptime_test_case(copy_semantics, { copySemantics(); })

static constexpr void withCapacity() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		ArrayListUnmanaged<int> a = ArrayListUnmanaged<int>::withCapacity(allocator.get(), 20).ok();
		check_ge(a.capacity(), 20);
		check_eq(a.len(), 0);
		a.deinit(allocator.get());
	}
	{
		ArrayListUnmanaged<int> a = ArrayListUnmanaged<int>::withCapacity(allocator.get(), 20).ok();
		a.push(allocator.get(), 5);
		check_ge(a.capacity(), 20);
		check_eq(a.len(), 1);
		a.deinit(allocator.get());
	}
	if (!std::is_constant_evaluated()) {
		{
			ArrayList<int> a = ArrayList<int>::withCapacity(allocator->toRef(), 10);
			check_ge(a.capacity(), 10);
			check_eq(a.len(), 0);
		}
		{
			ArrayList<int> a = ArrayList<int>::withCapacity(allocator->toRef(), 10);
			a.push(1);
			check(a.capacity() >= 10);
			check_eq(a.len(), 1);
			check_eq(a[0], 1);
		}
	}
}

test_case("with capacity") { withCapacity(); }
comptime_test_case(with_capacity, { withCapacity(); })

static constexpr void cloneWithCapacity() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		ArrayListUnmanaged<int> a;
		a.push(allocator.get(), 5).ok();

		ArrayListUnmanaged<int> b = a.cloneWithCapacity(allocator.get(), 20).ok();
		check_ge(b.capacity(), 20);
		check_eq(b.len(), 1);
		check_eq(b[0], 5);

		a.deinit(allocator.get());
		b.deinit(allocator.get());
	}
}

test_case("clone with capacity") { cloneWithCapacity(); }
comptime_test_case(clone_with_capacity, { cloneWithCapacity(); })

static constexpr void initInitializerList() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	{
		ArrayListUnmanaged<int> a = ArrayListUnmanaged<int>::initList(allocator.get(), { 0, 1 }).ok();
		check_eq(a[0], 0);
		check_eq(a[1], 1);
		a.deinit(allocator.get());
	}
	{
		ArrayListUnmanaged<int> a = ArrayListUnmanaged<int>::withCapacityList(allocator.get(), 20, { 0, 1 }).ok();
		check_eq(a[0], 0);
		check_eq(a[1], 1);
		check_ge(a.capacity(), 20);
		a.deinit(allocator.get());
	}
	if (!std::is_constant_evaluated()) {
		ArrayList<int> a = ArrayList<int>::initList(globalHeapAllocator(), { 0, 1 });
		check_eq(a[0], 0);
		check_eq(a[1], 1);	
	}
}

test_case("init initializer list") { initInitializerList(); }
comptime_test_case(init_initializer_list, { initInitializerList(); })

static constexpr void initBufferCopy() {
	UniquePtr<IAllocator> allocator = makeTestingAllocator();
	int buf[] = { 0, 1, 2 };
	{
		ArrayListUnmanaged<int> a = ArrayListUnmanaged<int>::initBufferCopy(allocator.get(), buf, 3).ok();
		check_eq(a[0], 0);
		check_eq(a[1], 1);
		check_eq(a[2], 2);
		a.deinit(allocator.get());
	}
	{
		ArrayListUnmanaged<int> a = ArrayListUnmanaged<int>::withCapacityBufferCopy(allocator.get(), 20, buf, 3).ok();
		check_eq(a[0], 0);
		check_eq(a[1], 1);
		check_eq(a[2], 2);
		check_ge(a.capacity(), 20);
		a.deinit(allocator.get());
	}
	if (!std::is_constant_evaluated()) {
		ArrayList<int> a = ArrayList<int>::initBufferCopy(globalHeapAllocator(), buf, 3);
		check_eq(a[0], 0);
		check_eq(a[1], 1);
		check_eq(a[2], 2);
	}
}

test_case("init buffer copy") { initBufferCopy(); }
comptime_test_case(init_buffer_copy, { initBufferCopy(); })



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

static constexpr void testArrayListRemoveStringContainsOneElement() {
	ArrayList<std::string> a;
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	check_eq(a.remove(0), "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	check_eq(a.len(), 0);
}

test_case("remove string contains one element") {
	testArrayListRemoveStringContainsOneElement();
}

comptime_test_case(remove_string_contains_one_element, {
	testArrayListRemoveStringContainsOneElement();
});

static constexpr void testArrayListStringRemoveLastElement() {
	ArrayList<std::string> a;
	a.push("10");
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	a.push("hello world!");
	a.push("whoa");
	a.push("12w3er467891234567892w3456789234567890");
	check_eq(a.remove(4), "12w3er467891234567892w3456789234567890");
	check_eq(a.len(), 4);
	check_eq(a[0], "10");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	check_eq(a[2], "hello world!");
	check_eq(a[3], "whoa");
}

test_case("remove string last element") {
	testArrayListStringRemoveLastElement();
}

comptime_test_case(remove_string_last_element, {
	testArrayListStringRemoveLastElement();
});

static constexpr void testArrayListRemoveStringMiddleElement() {
	ArrayList<std::string> a;
	a.push("10");
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("hello world!");
	a.push("12w3er467891234567892w3456789234567890");
	check_eq(a.remove(2), "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a.len(), 4);
	check_eq(a[0], "10");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	check_eq(a[2], "hello world!");
	check_eq(a[3], "12w3er467891234567892w3456789234567890");
}

test_case("remove string middle element") {
	testArrayListRemoveStringMiddleElement();
}

comptime_test_case(remove_string_middle_element, {
	testArrayListRemoveStringMiddleElement();
});

static constexpr void testArrayListRemoveSwapContainsOneElement() {
	ArrayList<int> a;
	a.push(50);
	check_eq(a.removeSwap(0), 50);
	check_eq(a.len(), 0);
}

test_case("remove swap contains one element") {
	testArrayListRemoveSwapContainsOneElement();
}

comptime_test_case(remove_swap_contains_one_element, {
	testArrayListRemoveSwapContainsOneElement();
});

static constexpr void testArrayListStringRemoveSwapContainsOneElement() {
	ArrayList<std::string> a;
	a.push("hello world! holy canoly lalalalalalalalalalalala");
	check_eq(a.removeSwap(0), "hello world! holy canoly lalalalalalalalalalalala");
	check_eq(a.len(), 0);
}

test_case("remove swap string contains one element") {
	testArrayListStringRemoveSwapContainsOneElement();
}

comptime_test_case(remove_swap_string_contains_one_element, {
	testArrayListStringRemoveSwapContainsOneElement();
});

static constexpr void testArrayListRemoveSwapLastElement() {
	ArrayList<int> a;
	a.push(10);
	a.push(20);
	a.push(30);
	a.push(40);
	a.push(50);
	check_eq(a.removeSwap(4), 50);
	check_eq(a.len(), 4);
	check_eq(a[0], 10);
	check_eq(a[1], 20);
	check_eq(a[2], 30);
	check_eq(a[3], 40);
}

test_case("remove swap last element") {
	testArrayListRemoveSwapLastElement();
}

comptime_test_case(remove_swap_last_element, {
	testArrayListRemoveSwapLastElement();
	});

static constexpr void testArrayListStringRemoveSwapLastElement() {
	ArrayList<std::string> a;
	a.push("10");
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	a.push("hello world!");
	a.push("whoa");
	a.push("12w3er467891234567892w3456789234567890");
	check_eq(a.removeSwap(4), "12w3er467891234567892w3456789234567890");
	check_eq(a.len(), 4);
	check_eq(a[0], "10");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	check_eq(a[2], "hello world!");
	check_eq(a[3], "whoa");
}

test_case("remove swap string last element") {
	testArrayListStringRemoveSwapLastElement();
}

comptime_test_case(remove_swap_string_last_element, {
	testArrayListStringRemoveSwapLastElement();
	});

static constexpr void testArrayListRemoveSwapMiddleElement() {
	ArrayList<int> a;
	a.push(10);
	a.push(20);
	a.push(30);
	a.push(40);
	a.push(50);
	check_eq(a.removeSwap(2), 30);
	check_eq(a.len(), 4);
	check_eq(a[0], 10);
	check_eq(a[1], 20);
	check_eq(a[2], 50);
	check_eq(a[3], 40);
}

test_case("remove swap middle element") {
	testArrayListRemoveSwapMiddleElement();
}

comptime_test_case(remove_swap_middle_element, {
	testArrayListRemoveSwapMiddleElement();
	});

static constexpr void testArrayListRemoveSwapStringMiddleElement() {
	ArrayList<std::string> a;
	a.push("10");
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	a.push("asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("hello world!");
	a.push("12w3er467891234567892w3456789234567890");
	check_eq(a.removeSwap(2), "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a.len(), 4);
	check_eq(a[0], "10");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh");
	check_eq(a[2], "12w3er467891234567892w3456789234567890");
	check_eq(a[3], "hello world!");
}

test_case("remove swap string middle element") {
	testArrayListRemoveSwapStringMiddleElement();
}

comptime_test_case(remove_swap_string_middle_element, {
	testArrayListRemoveSwapStringMiddleElement();
	});

static constexpr void testArrayListInsertEmpty() {
	ArrayList<std::string> a;
	a.insert(0, "hello asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456!");
	check_eq(a.len(), 1);
	check_eq(a[0], "hello asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456!");
}

test_case("insert empty") {
	testArrayListInsertEmpty();
}

comptime_test_case(insert_empty, {
	testArrayListInsertEmpty();
});

static constexpr void testArrayListInsertEmptySanity() {
	ArrayList<std::string> a;
	const std::string str = "hello asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456!";
	a.insert(0, str);
	check_eq(a.len(), 1);
	check_eq(a[0], str);
}

test_case("insert empty sanity") {
	testArrayListInsertEmptySanity();
}

comptime_test_case(insert_empty_sanity, {
	testArrayListInsertEmptySanity();
});

static constexpr void testArrayListInsertEnd() {
	ArrayList<std::string> a;
	a.push("lol");
	a.push("13");
	a.insert(2, "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456"); // move
	check_eq(a.len(), 3);
	check_eq(a[0], "lol");
	check_eq(a[1], "13");
	check_eq(a[2], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	const std::string another = "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456";
	a.insert(3, another); // copy
	check_eq(a.len(), 4);
	check_eq(a[0], "lol");
	check_eq(a[1], "13");
	check_eq(a[2], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[3], "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
}

test_case("insert end") {
	testArrayListInsertEnd();
}

comptime_test_case(insert_end, {
	testArrayListInsertEnd();
});

static constexpr void testArrayListInsertMiddle() {
	ArrayList<std::string> a;
	a.push("lol");
	a.push("13");
	a.push("whoa!");
	a.insert(1, "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456"); // move
	check_eq(a.len(), 4);
	check_eq(a[0], "lol");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "13");
	check_eq(a[3], "whoa!");
	const std::string another = "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456";
	a.insert(1, another); // copy
	check_eq(a.len(), 5);
	check_eq(a[0], "lol");
	check_eq(a[1], "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[3], "13");
	check_eq(a[4], "whoa!");
}

test_case("insert middle") {
	testArrayListInsertMiddle();
}

comptime_test_case(insert_middle, {
	testArrayListInsertMiddle();
});

static constexpr void testArrayListInsertSwapEmpty() {
	ArrayList<std::string> a;
	a.insertSwap(0, "hello asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456!");
	check_eq(a.len(), 1);
	check_eq(a[0], "hello asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456!");
}

test_case("insert swap empty") {
	testArrayListInsertSwapEmpty();
}

comptime_test_case(insert_swap_empty, {
	testArrayListInsertSwapEmpty();
	});

static constexpr void testArrayListInsertSwapEmptySanity() {
	ArrayList<std::string> a;
	const std::string str = "hello asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456!";
	a.insertSwap(0, str);
	check_eq(a.len(), 1);
	check_eq(a[0], str);
}

test_case("insert swap empty sanity") {
	testArrayListInsertSwapEmptySanity();
}

comptime_test_case(insert_swap_empty_sanity, {
	testArrayListInsertSwapEmptySanity();
	});

static constexpr void testArrayListInsertSwapEnd() {
	ArrayList<std::string> a;
	a.push("lol");
	a.push("13");
	a.push("whoa!");
	a.insertSwap(1, "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456"); // move
	check_eq(a.len(), 4);
	check_eq(a[0], "lol");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "whoa!");
	check_eq(a[3], "13");
	const std::string another = "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456";
	a.insertSwap(1, another); // copy
	check_eq(a.len(), 5);
	check_eq(a[0], "lol");
	check_eq(a[1], "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "whoa!");
	check_eq(a[3], "13");
	check_eq(a[4], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
}

test_case("insert swap end") {
	testArrayListInsertSwapEnd();
}

comptime_test_case(insert_swap_end, {
	testArrayListInsertSwapEnd();
});

static constexpr void testArrayListInsertSwapMiddle() {
	ArrayList<std::string> a;
	a.push("lol");
	a.push("13");
	a.push("whoa!");
	a.insertSwap(1, "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456"); // move
	check_eq(a.len(), 4);
	check_eq(a[0], "lol");
	check_eq(a[1], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "whoa!");
	check_eq(a[3], "13");
	const std::string another = "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456";
	a.insertSwap(1, another); // copy
	check_eq(a.len(), 5);
	check_eq(a[0], "lol");
	check_eq(a[1], "!!asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "whoa!");
	check_eq(a[3], "13");
	check_eq(a[4], "asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
}

test_case("insert swap middle") {
	testArrayListInsertSwapMiddle();
}

comptime_test_case(insert_swap_middle, {
	testArrayListInsertSwapMiddle();
});

static constexpr void testArrayListShrinkToFit() {
	ArrayList<std::string> a;
	a.reserve(20);
	a.push("hello world!");
	check_ge(a.capacity(), 20);
	a.shrinkToFit();
	check_lt(a.capacity(), 20);
}

test_case("shrink to fit") {
	testArrayListShrinkToFit();
}

comptime_test_case(shrink_to_fit, {
	testArrayListShrinkToFit();
});

static constexpr void testArrayListShrink() {
	ArrayList<std::string> a;
	a.reserve(20);
	a.push("hello world!");
	check_ge(a.capacity(), 20);
	a.shrinkTo(10);
	check_lt(a.capacity(), 20);
	check_ge(a.capacity(), 10);
}

test_case("shrink to") {
	testArrayListShrink();
}

comptime_test_case(shrink_to, {
	testArrayListShrink();
});

static constexpr void testArrayListTruncate() {
	ArrayList<std::string> a;
	a.push("0 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("1 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("2 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("3 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("4 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("5 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.truncate(3);
	check_eq(a.len(), 3);
	check_eq(a[0], "0 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[1], "1 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "2 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
}

test_case("truncate") {
	testArrayListTruncate();
}

comptime_test_case(truncate, {
	testArrayListTruncate();
});

static constexpr void testArrayListTruncateGreaterThanLength() {
	ArrayList<std::string> a;
	a.push("0 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("1 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("2 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.truncate(5);
	check_eq(a.len(), 3);
	check_eq(a[0], "0 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[1], "1 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	check_eq(a[2], "2 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
}

test_case("truncate greater than length") {
	testArrayListTruncateGreaterThanLength();
}

comptime_test_case(truncate_greater_than_length, {
	testArrayListTruncateGreaterThanLength();
});

static constexpr void testArrayListAppendCopy() {
	ArrayList<std::string> a;
	a.push("0");
	a.push("1");
	ArrayList<std::string> b;
	b.push("2");
	b.push("3");

	a.appendCopy(b);
	check_eq(a.len(), 4);
	check_eq(a[0], "0");
	check_eq(a[1], "1");
	check_eq(a[2], "2");
	check_eq(a[3], "3");
}

test_case("append copy") {
	testArrayListAppendCopy();
}

comptime_test_case(append_copy, {
	testArrayListAppendCopy();
});

static constexpr void testArrayListAppendInitializerList() {
	ArrayList<std::string> a;
	a.push("0");
	a.push("1");
	a.appendList({ "2", "3" });
	check_eq(a.len(), 4);
	check_eq(a[0], "0");
	check_eq(a[1], "1");
	check_eq(a[2], "2");
	check_eq(a[3], "3");
}

test_case("append initializer list") {
	testArrayListAppendInitializerList();
}

comptime_test_case(append_initializer_list, {
	testArrayListAppendInitializerList();
});

static constexpr void testArrayListAppendBufferCopy() {
	ArrayList<std::string> a;
	a.push("0");
	a.push("1");

	std::string b[] = { "2", "3" };
	a.appendBufferCopy(b, 2);
	check_eq(a.len(), 4);
	check_eq(a[0], "0");
	check_eq(a[1], "1");
	check_eq(a[2], "2");
	check_eq(a[3], "3");
}

test_case("append buffer copy") {
	testArrayListAppendBufferCopy();
}

comptime_test_case(append_buffer_copy, {
	testArrayListAppendBufferCopy();
});

static constexpr void testArrayListResizeEmptyToEmpty() {
	ArrayList<std::string> a;
	a.resize(0, "hello world!");
	check_eq(a.len(), 0);
}

test_case("resize empty to empty") {
	testArrayListResizeEmptyToEmpty();
}

comptime_test_case(resize_empty_to_empty, {
	testArrayListResizeEmptyToEmpty();
});

static constexpr void testArrayListResizeEmptyFill() {
	ArrayList<std::string> a;
	a.resize(2, "hello world!");
	check_eq(a.len(), 2);
	check_eq(a[0], "hello world!");
	check_eq(a[1], "hello world!");
}

test_case("resize empty fill") {
	testArrayListResizeEmptyFill();
}

comptime_test_case(resize_empty_fill, {
	testArrayListResizeEmptyFill();
});

static constexpr void testArrayListResizeHasElementFill() {
	ArrayList<std::string> a;
	a.push("hmm");
	a.resize(3, "hello world!");
	check_eq(a.len(), 3);
	check_eq(a[0], "hmm");
	check_eq(a[1], "hello world!");
	check_eq(a[2], "hello world!");
}

test_case("resize has element fill") {
	testArrayListResizeHasElementFill();
}

comptime_test_case(resize_has_element_fill, {
	testArrayListResizeHasElementFill();
});

static constexpr void testArrayListResizeHasElementSameResizeLength() {
	ArrayList<std::string> a;
	a.push("hmm");
	a.resize(1, "hello world!");
	check_eq(a.len(), 1);
	check_eq(a[0], "hmm");
}

test_case("resize has element same resize length") {
	testArrayListResizeHasElementSameResizeLength();
}

comptime_test_case(resize_has_element_same_resize_length, {
	testArrayListResizeHasElementSameResizeLength();
});

static constexpr void testArrayListResizeTruncate() {
	ArrayList<std::string> a;
	a.push("0");
	a.push("1 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("2 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.push("3 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
	a.resize(2, "");
	check_eq(a.len(), 2);
	check_eq(a[0], "0");
	check_eq(a[1], "1 asdliajshdlkajshdlakjshdlakjshdlakjshdlakjsdh123456");
}

test_case("resize truncate") {
	testArrayListResizeTruncate();
}

comptime_test_case(resize_truncate, {
	testArrayListResizeTruncate();
});

static constexpr void testArrayListIteratorEmpty() {
	ArrayList<std::string> a;
	usize i = 0;
	for (auto& elem : a) {
		i++;
	}
	check_eq(i, 0);
}

test_case("iterator empty") { testArrayListIteratorEmpty(); }
comptime_test_case(iterator_empty, { testArrayListIteratorEmpty(); });

static constexpr void testArrayListIteratorNotEmpty() {
	ArrayList<std::string> a;
	a.push(":D");
	a.push(":D");
	usize i = 0;
	for (auto& elem : a) {
		i++;
		check_eq(elem, ":D");
	}
	check_eq(i, 2);
}

test_case("iterator not empty") { testArrayListIteratorNotEmpty(); }
comptime_test_case(iterator_not_empty, { testArrayListIteratorNotEmpty(); });

static constexpr void testArrayListConstIteratorEmpty() {
	const ArrayList<std::string> a;
	usize i = 0;
	for (auto& elem : a) {
		i++;
	}
	check_eq(i, 0);
}

test_case("const iterator empty") { testArrayListConstIteratorEmpty(); }
comptime_test_case(const_iterator_empty, { testArrayListConstIteratorEmpty(); });

static constexpr void testArrayListConstIteratorNotEmpty() {
	const ArrayList<std::string> a = []() {
		ArrayList<std::string> out;
		out.push(":D");
		out.push(":D");
		return out;
	}();
	
	usize i = 0;
	for (auto& elem : a) {
		i++;
		check_eq(elem, ":D");
	}
	check_eq(i, 2);
}

test_case("const iterator not empty") { testArrayListConstIteratorNotEmpty(); }
comptime_test_case(const_iterator_not_empty, { testArrayListConstIteratorNotEmpty(); });




static constexpr void testArrayListReverseIteratorEmpty() {
	ArrayList<std::string> a;
	usize i = 0;
	for (auto rit = a.rbegin(); rit != a.rend(); ++rit) {
		auto& elem = *rit;
		i++;
	}
	check_eq(i, 0);
}

test_case("reverse iterator empty") { testArrayListReverseIteratorEmpty(); }
comptime_test_case(reverse_iterator_empty, { testArrayListReverseIteratorEmpty(); });

static constexpr void testArrayListReverseIteratorNotEmpty() {
	ArrayList<std::string> a;
	a.push(":D");
	a.push(":D");
	usize i = 0;
	for (auto rit = a.rbegin(); rit != a.rend(); ++rit) {
		auto& elem = *rit;
		i++;
		check_eq(elem, ":D");
	}
	check_eq(i, 2);
}

test_case("reverse iterator not empty") { testArrayListReverseIteratorNotEmpty(); }
comptime_test_case(reverse_iterator_not_empty, { testArrayListReverseIteratorNotEmpty(); });

static constexpr void testArrayListReverseConstIteratorEmpty() {
	const ArrayList<std::string> a;
	usize i = 0;
	for (auto rit = a.rbegin(); rit != a.rend(); ++rit) {
		auto& elem = *rit;
		i++;
	}
	check_eq(i, 0);
}

test_case("reverse const iterator empty") { testArrayListReverseConstIteratorEmpty(); }
comptime_test_case(reverse_const_iterator_empty, { testArrayListReverseConstIteratorEmpty(); });

static constexpr void testArrayListReverseConstIteratorNotEmpty() {
	const ArrayList<std::string> a = []() {
		ArrayList<std::string> out;
		out.push(":D");
		out.push(":D");
		return out;
	}();
	
	usize i = 0;
	for (auto rit = a.rbegin(); rit != a.rend(); ++rit) {
		auto& elem = *rit;
		i++;
		check_eq(elem, ":D");
	}
	check_eq(i, 2);
}

test_case("reverse const iterator not empty") { testArrayListReverseConstIteratorNotEmpty(); }
comptime_test_case(reverse_const_iterator_not_empty, { testArrayListReverseConstIteratorNotEmpty(); });

#endif


