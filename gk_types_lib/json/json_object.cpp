#include "json_object.h"
#include <intrin.h>

// STORE THE HASH CODE IN PLACE 

using gk::usize;

/**
* Allocates the hash masks, and pairs in a single allocation.
* Naturally this allows for less overhead by doing less dynamic memory
* tomfoolery.
* 
* @param hashMaskCount: Number of hash masks. Must be a multiple of 64.
* @param pairCount: Number of pairs. Must be less than or equal to `hashMaskCount`
* @return Number of i8 to allocate.
*/
static constexpr usize calculateAllocationSize(usize hashMaskCount, usize pairCount) {
	check(hashMaskCount % 64 == 0);
	check_le(pairCount, hashMaskCount);

	return hashMaskCount + (sizeof(gk::internal::JsonKeyValue) * pairCount);
}

void gk::internal::JsonObjectBucket::defaultConstructRuntime(Allocator* allocator)
{
	constexpr usize alignment = 64;
	constexpr usize allocationSize = calculateAllocationSize(64, 4);

	i8* memory = allocator->mallocAlignedBuffer<i8>(allocationSize, alignment).ok();
	memset(memory, 0, allocationSize);

	this->maskCapacity = 64;
	this->pairCapacity = 4;
	this->hashMasks = memory;
	this->pairs = reinterpret_cast<JsonKeyValue*>(memory + maskCapacity); // offset by the masks
}

inline void gk::internal::JsonObjectBucket::reallocateMasksAndPairs(usize minCapacity, Allocator* allocator)
{
	constexpr usize alignment = 64;

	const usize hashMaskMallocCapacity = minCapacity + (64 - (minCapacity % 64));
	const usize allocationSize = calculateAllocationSize(hashMaskMallocCapacity, minCapacity);

	i8* memory = allocator->mallocAlignedBuffer<i8>(allocationSize, alignment).ok();
	memset(memory, 0, allocationSize);

	i8* newMasks = memory;
	JsonKeyValue* newPairs = reinterpret_cast<JsonKeyValue*>(memory + hashMaskMallocCapacity);

	memcpy(newMasks, hashMasks, maskCapacity);
	for (usize i = 0; i < length; i++) {
		new (newPairs + i) JsonKeyValue(std::move(pairs[i]));
	}

	allocator->freeAlignedBuffer<i8>(hashMasks, calculateAllocationSize(maskCapacity, pairCapacity), alignment);

	this->maskCapacity = hashMaskMallocCapacity;
	this->pairCapacity = minCapacity;
	this->hashMasks = newMasks;
	this->pairs = newPairs;
}

namespace gk {
	namespace internal {

		typedef Option<usize>(*FindHashBitsInJsonMaskFunc)(const String&, const i8*, const JsonKeyValue*, JsonPairHashBits, usize);
		typedef Option<usize>(*FindFirstAvailableSlotJsonFunc)(const i8*, usize);

		static Option<usize> avx512FindHashBitsInJsonMask(const String& key, const i8* hashMasks, const JsonKeyValue* pairs, JsonPairHashBits hashCode, usize len) {
			const __m512i hashCodeVec = _mm512_set1_epi8(hashCode.value);
			const __m512i* masksVec = reinterpret_cast<const __m512i*>(hashMasks);

			const usize iterationsToDo = ((len) % 64 == 0 ?
				len :
				len + (64 - (len % 64)))
				/ 64;

			for (usize i = 0; i < iterationsToDo; i++) {
				u64 bitmask = _mm512_cmpeq_epi8_mask(hashCodeVec, *masksVec);
				while (true) {
					Option<usize> optIndex = bitscanForwardNext(&bitmask);
					if (optIndex.none()) {
						break;
					}
					const usize index = optIndex.someCopy() + (i * 64);
					const String& name = pairs[index].key;
					if (name == key) {
						return Option<usize>(index);
					}
				}
			}

			return Option<usize>();
		}

		static Option<usize> avx512FindFirstAvailableSlot(const i8* hashMasks, usize pairCapacity) {
			const __m512i zeroVec = _mm512_set1_epi8(0);
			//const __m512i indices = _mm512_set_epi8(
			//	63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33,
			//	32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);

			const __m512i* hashMasksVec = reinterpret_cast<const __m512i*>(hashMasks);
			const usize iterationsToDo = ((pairCapacity) % 64 == 0 ?
				pairCapacity :
				pairCapacity + (64 - (pairCapacity % 64)))
				/ 64;

			for (usize i = 0; i < iterationsToDo; i++) {
				//const i8 num = i != (iterationsToDo - 1) ? static_cast<i8>(64) : static_cast<i8>((iterationsToDo * i) - pairCapacity);
				//check_le(num, 64);

				const u64 bitmask = _mm512_cmpeq_epi8_mask(zeroVec, *hashMasksVec);
				unsigned long bcfIndex;
				if (_BitScanForward64(&bcfIndex, bitmask) == 0) {
					hashMasksVec++;
					continue;
				}

				const usize index = static_cast<usize>(bcfIndex) + (i * 64);
				if (index >= pairCapacity) {
					return Option<usize>();
				}
				else {
					return Option<usize>(index);
				}
			}
			return Option<usize>();
		}
	}
}

gk::Option<usize> gk::internal::JsonObjectBucket::findIndexOfKeyRuntime(const String& key, JsonPairHashBits hashCode) const
{
	check_ne(hashMasks, nullptr);

	static FindHashBitsInJsonMaskFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (true) {
				std::cout << "[Json function loader]: Using AVX-512 key find\n";
			}

			return avx512FindHashBitsInJsonMask;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (true) {
				std::cout << "[Json function loader]: Using AVX-2 key find\n";
			}
			abort();
			return avx512FindHashBitsInJsonMask;
		}
		else {
			std::cout << "[Json function loader]: ERROR\nCannot load json key find functions if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(key, this->hashMasks, this->pairs, hashCode, this->length);
}

gk::Option<usize> gk::internal::JsonObjectBucket::firstAvailableSlot() const
{
	check_ne(hashMasks, nullptr);

	static FindFirstAvailableSlotJsonFunc func = []() {
		if (gk::x86::isAvx512Supported()) {
			if (true) {
				std::cout << "[Json function loader]: Using AVX-512 key find empty slot\n";
			}

			return avx512FindFirstAvailableSlot;
		}
		else if (gk::x86::isAvx2Supported()) {
			if (true) {
				std::cout << "[Json function loader]: Using AVX-2 key find empty slot\n";
			}
			abort();
			return avx512FindFirstAvailableSlot;
		}
		else {
			std::cout << "[Json function loader]: ERROR\nCannot load json key find empty slot functions if AVX-512 or AVX-2 aren't supported\n";
			abort();
		}
	}();

	return func(this->hashMasks, this->pairCapacity);
}

void gk::internal::JsonObjectBucket::freeRuntime(Allocator* allocator)
{
	if (hashMasks == nullptr) return;

	constexpr usize alignment = 64;
	const usize currentAllocationSize = calculateAllocationSize(maskCapacity, pairCapacity);
	allocator->freeAlignedBuffer<i8>(hashMasks, calculateAllocationSize(maskCapacity, pairCapacity), alignment);
	pairs = nullptr;
}

usize gk::JsonObject::calculateNewBucketCount(usize requiredCapacity)
{
	return requiredCapacity <= 64 ? 1 : static_cast<usize>(gk::upperPowerOfTwo(requiredCapacity / 32));
}

void gk::JsonObject::reallocateRuntime(usize requiredCapacity)
{
	using internal::JsonObjectBucket;
	Allocator* allocator = gk::globalHeapAllocator();

	const usize newBucketCount = calculateNewBucketCount(requiredCapacity);
	if (newBucketCount <= bucketCount) {
		return;
	}

	JsonObjectBucket* newBuckets = allocator->mallocBuffer<JsonObjectBucket>(newBucketCount).ok();
	for (usize i = 0; i < newBucketCount; i++) {
		new (newBuckets + i) JsonObjectBucket(allocator);
	}

	for (usize oldBucketIndex = 0; oldBucketIndex < bucketCount; oldBucketIndex++) {
		JsonObjectBucket& oldBucket = buckets[oldBucketIndex];
		for (usize i = 0; i < oldBucket.length; i++) {
			// Does not contain anything
			if (oldBucket.hashMasks[i] == 0) continue;

			internal::JsonKeyValue& pair = oldBucket.pairs[i];
			const usize hashCode = pair.key.hash();
			const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
			const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

			const usize newBucketIndex = bucketBits.value % newBucketCount;
			newBuckets[newBucketIndex].insert(std::move(pair.key), std::move(pair.value), pairBits, allocator);
		}
		oldBucket.length = 0;
		oldBucket.free(allocator);
	}
	if (buckets) {
		allocator->freeBuffer(buckets, bucketCount);
	}

	buckets = newBuckets;
	bucketCount = newBucketCount;
}

gk::Option<gk::JsonValue*> gk::JsonObject::addFieldRuntime(String&& name, JsonValue&& value)
{
	const usize hashCode = name.hash();
	const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
	const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);

	Option<JsonValue*> found = [&]() {
		if (elementCount == 0) {
			return Option<JsonValue*>();
		}
		const usize bucketIndex = bucketBits.value % bucketCount;
		return buckets[bucketIndex].find(name, pairBits);
	}();
	if (found.isSome()) {
		return found;
	}

	if (shouldReallocate(elementCount + 1)) {
		reallocateRuntime(elementCount + 1);
	}

	{
		const usize bucketIndex = bucketBits.value % bucketCount;
		buckets[bucketIndex].insert(std::move(name), std::move(value), pairBits, gk::globalHeapAllocator());
		return gk::Option<JsonValue*>();
	}
}

bool gk::JsonObject::eraseFieldRuntime(const String& name)
{
	const usize hashCode = name.hash();
	const internal::JsonHashBucketBits bucketBits = internal::JsonHashBucketBits(hashCode);
	const internal::JsonPairHashBits pairBits = internal::JsonPairHashBits(hashCode);
	const usize bucketIndex = bucketBits.value % bucketCount;
	return buckets[bucketIndex].erase(name, pairBits, gk::globalHeapAllocator());
}
