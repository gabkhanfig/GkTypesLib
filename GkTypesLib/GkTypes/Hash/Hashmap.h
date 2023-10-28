#pragma once

#include "../BasicTypes.h"
#include "../Asserts.h"
#include "../Utility.h"
#include "Hash.h"
#include <intrin.h>
#include "../Option/Option.h"
#include "../CpuFeatures/CpuFeatureDetector.h"

namespace gk 
{
	template<typename Key, typename Value>
	struct HashPair {
		Key key;
		Value value;

		HashPair() = default;
		HashPair(const Key& inKey, const Value& inValue) : key(inKey), value(inValue) {}
		HashPair(Key&& inKey, const Value& inValue) : key(std::move(inKey)), value(inValue) {}
		HashPair(const Key& inKey, Value&& inValue) : key(inKey), value(std::move(inValue)) {}
		HashPair(Key&& inKey, Value&& inValue) : key(std::move(inKey)), value(std::move(inValue)) {}
		~HashPair() = default;
	};

	/* Key is required to implement a specialization for gk::hash<T>(const T&). 
	Key must also implement equality comparison.

	Technical details:
	gk::HashMap uses the 48 higher bits in the hash function output to determine the table/bucket index,
	and then the 16 lowest bits as a fast path. The 16 bits are stored along side the pointer to the key/value pair. */
	template<typename Key, typename Value>
		requires Hashable<Key>
	struct HashMap
	{
	private:

#pragma region Hash_Types

		// To determine the table index
		struct HashBucketBits {
			static constexpr size_t BITMASK = 0xFFFFFFFFFFFFULL << 16;
			size_t value;

			HashBucketBits(const size_t hashCode)
				: value((hashCode & BITMASK) >> 16) {}
		};

		// To determine the 16 unused pointer bits in the key/value pair
		struct PairHashBits {
			static constexpr size_t BITMASK = 0xFFFFULL;
			size_t value;

			PairHashBits(const size_t hashCode)
				: value((hashCode & BITMASK) << 48) {}
		};

		static size_t hashKey(const Key& key) {
			return gk::hash<Key>(key);
		}

#pragma endregion

#pragma region Key_Value_Storage

		struct Group {
			__m512i pairs;

			Group() {
				memset(&pairs, 0, sizeof(Group::pairs));
			}
			Group(const Group&) = delete;
			Group(Group&& other) noexcept {
				memcpy(pairs, other.pairs, sizeof(Group::pairs));
				memset(other.pairs, 0, sizeof(Group::pairs));
			}
			Group& operator = (const Group&) = delete;
			Group& operator = (Group&& other) noexcept {
				memcpy(&pairs, &other.pairs, sizeof(Group::pairs));
				memset(&other.pairs, 0, sizeof(Group::pairs));
				return *this;
			}
			~Group() {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;
				for (int i = 0; i < 8; i++) {
					gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(pairs.m512i_u64[i] & pointerBits);
					if (pair != nullptr) {
						delete pair;
					}
				}
				memset(&pairs, 0, sizeof(Group::pairs));
			}

			forceinline gk::Option<gk::HashPair<Key, Value>*> find(const Key& key, const PairHashBits hashCode) {
				// If AVX-512, is Group::avx512FindKey
				// Otherwise, if AVX-2, is Group::avx2FindKey
				return findKeyFunc(pairs, key, hashCode);
				
			}

			/* If the entire thing is full, returns false, otherwise inserts the pair and lower hash bits. */
			bool insert(gk::HashPair<Key, Value>* pair, const PairHashBits hashCode) {
				for (size_t i = 0; i < 8; i++) {
					if (pairs.m512i_u64[i] == 0) {
						pairs.m512i_u64[i] = (size_t)pair | hashCode.value;
						return true;
					}
				}
				return false;
			}

			forceinline bool erase(const Key& key, const PairHashBits hashCode) {
				// If AVX-512, is Group::avx512EraseKey
				// Otherwise, if AVX-2, is Group::avx2EraseKey
				// THIS WONT WORK FOR THE SAME REASON AS CHECKING FIND FOR VALID POINTERS
				return eraseKeyFunc(pairs, key, hashCode);
			}

#pragma region Find_Dynamic_Dispatch

		private:

			typedef gk::Option<gk::HashPair<Key, Value>*>(*FuncFindKey)(const __m512i&, const Key&, const PairHashBits);

			static gk::Option<gk::HashPair<Key, Value>*> avx512FindKey(const __m512i& avxPairs, const Key& key, const PairHashBits hashCode) {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;
				constexpr size_t nonPointerBits = ~0xFFFFFFFFFFFFULL;
				// TODO it needs to work with hash code of zero, so will need to be checked with non-null pointers.
				const __m512i pointerHashBitsVec = _mm512_set1_epi64(nonPointerBits);
				const __m512i pointerPtrBitsVec = _mm512_set1_epi64(pointerBits);

				const __m512i maskRemovedPointers = _mm512_and_epi64(avxPairs, pointerHashBitsVec);

				const __m512i maskOnlyPointers = _mm512_and_epi64(avxPairs, pointerPtrBitsVec);
				const __m512i nullptrVec = _mm512_set1_epi64((long long)nullptr);

				const __m512i hashCodeVec = _mm512_set1_epi64(hashCode.value);
				const unsigned long matchedHashCodes = (unsigned long)_mm512_cmpeq_epu64_mask(maskRemovedPointers, hashCodeVec);
				const unsigned long nonNullPtrs = (unsigned long)_mm512_cmpneq_epu64_mask(maskOnlyPointers, hashCodeVec);
				unsigned long result = matchedHashCodes & nonNullPtrs;

				if (result == 0) {
					return gk::Option<gk::HashPair<Key, Value>*>();
				}
				unsigned long index;
				(void)_BitScanForward(&index, result);

				gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(avxPairs.m512i_u64[index] & pointerBits);
				gk_assertm(pair != nullptr, "Pair is null, yet hash code was found?");
				if (pair->key == key) {
					return gk::Option<gk::HashPair<Key, Value>*>(pair);
				}

				while (true) {
					result = (result & ~(1 << index)) & 0xFF;
					if (!_BitScanForward(&index, result)) {
						//gk_assertm(false, "unreachable");
						return gk::Option<gk::HashPair<Key, Value>*>();
					}
					gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(avxPairs.m512i_u64[index] & pointerBits);
					gk_assertm(pair != nullptr, "Pair is null, yet hash code was found?");
					if (pair->key == key) {
						return gk::Option<gk::HashPair<Key, Value>*>(pair);
					}
				}
			}

			static gk::Option<gk::HashPair<Key, Value>*> avx2FindKey(const __m512i& avxPairs, const Key& key, const PairHashBits hashCode) {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;
				constexpr size_t nonPointerBits = ~0xFFFFFFFFFFFFULL;
				// TODO it needs to work with hash code of zero, so will need to be checked with non-null pointers.
				const __m256i pointerHashBitsVec = _mm256_set1_epi64x(nonPointerBits);
				const __m256i pointerPtrBitsVec = _mm256_set1_epi64x(pointerBits);
				const __m256i nullptrVec = _mm256_set1_epi64x((long long)nullptr);
				const __m256i hashCodeVec = _mm256_set1_epi64x(hashCode.value);

				const __m256i* avx256Pairs = reinterpret_cast<const __m256i*>(&avxPairs);

				unsigned long result = 0;
				for (int i = 0; i < 2; i++) {
					const __m256i maskRemovedPointers = _mm256_and_epi64(avx256Pairs[i], pointerHashBitsVec);

					const __m256i maskOnlyPointers = _mm256_and_epi64(avx256Pairs[i], pointerPtrBitsVec);

					const unsigned long matchedHashCodes = (unsigned long)_mm256_cmpeq_epu64_mask(maskRemovedPointers, hashCodeVec);
					const unsigned long nonNullPtrs = (unsigned long)_mm256_cmpneq_epu64_mask(maskOnlyPointers, hashCodeVec);

					if (i == 0) {
						result |= matchedHashCodes & nonNullPtrs;
					}
					else {
						result |= (matchedHashCodes & nonNullPtrs) << 4;
					}
				}

				if (result == 0) {
					return gk::Option<gk::HashPair<Key, Value>*>();
				}
				unsigned long index;
				(void)_BitScanForward(&index, result);

				gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(avxPairs.m512i_u64[index] & pointerBits);
				gk_assertm(pair != nullptr, "Pair is null, yet hash code was found?");
				if (pair->key == key) {
					return gk::Option<gk::HashPair<Key, Value>*>(pair);
				}

				while (true) {
					result = (result & ~(1 << index)) & 0xFF;
					if (!_BitScanForward(&index, result)) {
						return gk::Option<gk::HashPair<Key, Value>*>();
					}
					gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(avxPairs.m512i_u64[index] & pointerBits);
					gk_assertm(pair != nullptr, "Pair is null, yet hash code was found?");
					if (pair->key == key) {
						return gk::Option<gk::HashPair<Key, Value>*>(pair);
					}
				}
			}

			[[nodiscard]] static FuncFindKey loadOptimalFindKeyFunc() {
				if (gk::x86::isAvx512Supported()) {
					std::cout << "[gk::HashMap function loader]: Using AVX-512 key find\n";
					return avx512FindKey;
				}
				else if (gk::x86::isAvx2Supported()) {
					std::cout << "[gk::HashMap function loader]: Using AVX-2 key find\n";
					return avx2FindKey;
				}
				else {
					std::cout << "[gk::HashMap function loader]: ERROR\nCannot load key find function if AVX-512 or AVX-2 aren't supported\n";
					abort();
				}
			}

			static inline FuncFindKey findKeyFunc = loadOptimalFindKeyFunc();

#pragma endregion

#pragma region Erase_Dynamic_Dispatch

		private:

			typedef bool(*FuncEraseKey)(__m512i&, const Key&, const PairHashBits);

			static bool avx512EraseKey(__m512i& avxPairs, const Key& key, const PairHashBits hashCode) {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;
				constexpr size_t nonPointerBits = ~0xFFFFFFFFFFFFULL;
				// TODO it needs to work with hash code of zero, so will need to be checked with non-null pointers.
				const __m512i pointerHashBitsVec = _mm512_set1_epi64(nonPointerBits);
				const __m512i pointerPtrBitsVec = _mm512_set1_epi64(pointerBits);

				const __m512i maskRemovedPointers = _mm512_and_epi64(avxPairs, pointerHashBitsVec);

				const __m512i maskOnlyPointers = _mm512_and_epi64(avxPairs, pointerPtrBitsVec);
				const __m512i nullptrVec = _mm512_set1_epi64((long long)nullptr);

				const __m512i hashCodeVec = _mm512_set1_epi64(hashCode.value);
				const unsigned long matchedHashCodes = (unsigned long)_mm512_cmpeq_epu64_mask(maskRemovedPointers, hashCodeVec);
				const unsigned long nonNullPtrs = (unsigned long)_mm512_cmpneq_epu64_mask(maskOnlyPointers, hashCodeVec);
				unsigned long result = matchedHashCodes & nonNullPtrs;

				if (result == 0) {
					return false;
				}
				unsigned long index;
				(void)_BitScanForward(&index, result);

				gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(avxPairs.m512i_u64[index] & pointerBits);
				gk_assertm(pair != nullptr, "Pair is null, yet hash code was found?");
				if (pair->key == key) {
					avxPairs.m512i_u64[index] = 0;
					delete pair;
					return true;
				}

				while (true) {
					result = (result & ~(1 << index)) & 0xFF;
					if (!_BitScanForward(&index, result)) {
						//gk_assertm(false, "unreachable");
						return false;
					}
					gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(avxPairs.m512i_u64[index] & pointerBits);
					gk_assertm(pair != nullptr, "Pair is null, yet hash code was found?");
					if (pair->key == key) {
						avxPairs.m512i_u64[index] = 0;
						delete pair;
						return true;
					}
				}
			}

			[[nodiscard]] static FuncEraseKey loadOptimalEraseKeyFunc() {
				if (gk::x86::isAvx512Supported()) {
					std::cout << "[gk::HashMap function loader]: Using AVX-512 key erase\n";
					return avx512EraseKey;
				}
				else if (gk::x86::isAvx2Supported()) {
					std::cout << "[gk::HashMap function loader]: Using AVX-2 key erase\n";
					return nullptr;
				}
				else {
					std::cout << "[gk::HashMap function loader]: ERROR\nCannot load key erase function if AVX-512 or AVX-2 aren't supported\n";
					abort();
				}
			}

			static inline FuncEraseKey eraseKeyFunc = loadOptimalEraseKeyFunc();

#pragma endregion

		};

		struct Bucket {
			Group* groups;
			uint32 groupCount;
			uint32 pairCount;

			Bucket() {
				groups = new Group[1];
				groupCount = 1;
				pairCount = 0;
			}
			Bucket(const Bucket&) = delete;
			Bucket(Bucket&& other) noexcept {
				groups = other.groups;
				groupCount = other.groupCount;
				other.groups = nullptr;
				other.groupCount = 0;
			}
			Bucket& operator = (const Bucket&) = delete;
			Bucket& operator = (Bucket&& other) noexcept {
				if (groups) {
					delete[] groups;
				}
				groups = other.groups;
				groupCount = other.groupCount;
				other.groups = nullptr;
				other.groupCount = 0;
				return *this;
			}
			~Bucket() {
				if (groups) {
					delete[] groups;
					groups = nullptr;
				}
			}

			gk::Option<gk::HashPair<Key, Value>*> find(const Key& key, const PairHashBits hashCode) {
				for (uint32 i = 0; i < groupCount; i++) {
					gk::Option<gk::HashPair<Key, Value>*> option = groups[i].find(key, hashCode);
					if (option.none()) continue;
					return option;
				}
				return gk::Option<gk::HashPair<Key, Value>*>();
			}

			/* If it doesn't fit, will create a new group and add to there. Does not trigger a rehash. */
			void insert(gk::HashPair<Key, Value>* pair, const PairHashBits hashCode) {
				for (uint32 i = 0; i < groupCount; i++) {
					if (groups[i].insert(pair, hashCode)) {
						return; // successfully inserted
					}
				}
				const uint32 newGroupCount = groupCount + 1;
				Group* newGroups = new Group[newGroupCount];

				for (uint32 i = 0; i < groupCount; i++) {
					newGroups[i] = std::move(groups[i]);
				}
				delete[] groups;
				groups = newGroups;
				groupCount = newGroupCount;

				const bool success = groups[newGroupCount - 1].insert(pair, hashCode);
				gk_assertm(success, "this shouldn't ever fail");
				pairCount++;
			}

			bool erase(const Key& key, const PairHashBits hashCode) {
				for (uint32 i = 0; i < groupCount; i++) {
					if (groups[i].erase(key, hashCode)) {
						return true;
						pairCount--;
					}
				}
				return false;
			}

		};

#pragma endregion

	public:

#pragma region Iterator_Implementation

		friend class Iterator;
		friend class ConstIterator;

		class Iterator {
		public:

			static Iterator iterBegin(const HashMap* map) {
				Iterator iter;
				iter._map = map;
				iter._currentBucket = map->buckets;
				return iter;
			}

			static Iterator iterEnd(const HashMap* map) {
				Iterator iter;
				iter._map = map;
				iter._currentBucket = map->buckets + map->bucketCount;
				return iter;
			}

			Iterator& operator++() {
				while (true) {
					_currentElementIndex++;
					if (_currentElementIndex == 8) {
						_currentElementIndex = 0;
						_currentGroupIndex++;
					}

					if (_currentGroupIndex == _currentBucket->groupCount) {
						_currentBucket++;
						_currentElementIndex = 0;
						_currentGroupIndex = 0;
					}

					if (_currentBucket == _map->buckets + _map->bucketCount) {
						return *this;
					}

					if (_currentBucket->groups[_currentGroupIndex].pairs.m512i_u64[_currentElementIndex] != 0) {
						return *this;
					}
				}
			}

			bool operator !=(const Iterator& other) const {
				return _currentBucket != other._currentBucket;
			}

			gk::HashPair<const Key, Value>* operator*() const {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;
				return (gk::HashPair<const Key, Value>*)(_currentBucket->groups[_currentGroupIndex].pairs.m512i_u64[_currentElementIndex] & pointerBits);
			}

		private:
			Iterator() : _map(nullptr), _currentBucket(nullptr), _currentGroupIndex(0), _currentElementIndex(0) {}
			const HashMap* _map;
			const Bucket* _currentBucket;
			uint32 _currentGroupIndex;
			uint32 _currentElementIndex;
		};

		class ConstIterator {
		public:

			static ConstIterator iterBegin(const HashMap* map) {
				ConstIterator iter;
				iter._map = map;
				iter._currentBucket = map->buckets;
				return iter;
			}

			static ConstIterator iterEnd(const HashMap* map) {
				ConstIterator iter;
				iter._map = map;
				iter._currentBucket = map->buckets + map->bucketCount;
				return iter;
			}

			ConstIterator& operator++() {
				while (true) {
					_currentElementIndex++;
					if (_currentElementIndex == 8) {
						_currentElementIndex = 0;
						_currentGroupIndex++;
					}

					if (_currentGroupIndex == _currentBucket->groupCount) {
						_currentBucket++;
						_currentElementIndex = 0;
						_currentGroupIndex = 0;
					}

					if (_currentBucket == _map->buckets + _map->bucketCount) {
						return *this;
					}

					if (_currentBucket->groups[_currentGroupIndex].pairs.m512i_u64[_currentElementIndex] != 0) {
						return *this;
					}
				}
			}

			bool operator !=(const ConstIterator& other) const {
				return _currentBucket != other._currentBucket;
			}

			const gk::HashPair<Key, Value>* operator*() const {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;
				return (const gk::HashPair<Key, Value>*)(_currentBucket->groups[_currentGroupIndex].pairs.m512i_u64[_currentElementIndex] & pointerBits);
			}

		private:
			ConstIterator() : _map(nullptr), _currentBucket(nullptr), _currentGroupIndex(0), _currentElementIndex(0) {}
			const HashMap* _map;
			const Bucket* _currentBucket;
			uint32 _currentGroupIndex;
			uint32 _currentElementIndex;
		};

#pragma endregion

#pragma region Construct_Destruct_Assign
		
		HashMap()
			: buckets(nullptr), bucketCount(0), elementCount(0)
		{}

		HashMap(const HashMap& other) :
			buckets(nullptr), bucketCount(0), elementCount(0)
		{
			if (other.elementCount == 0) {
				return;
			}

			bucketCount = other.elementCount < 9 ? 1 : static_cast<uint32>(gk::UpperPowerOfTwo((other.elementCount * 2) >> 3));

			gk_assertm(bucketCount > 0, "Cannot allocate 0 buckets");
			buckets = new Bucket[bucketCount];

			for (const gk::HashPair<Key, Value>* pair : other) { // TODO operate on the new buckets directly
				insert(pair->key, pair->value);// , "should've been able to insert key/value pair from copying other hashmap");
			}
		}

		HashMap(HashMap&& other) noexcept {
			buckets = other.buckets;
			bucketCount = other.bucketCount;
			elementCount = other.elementCount;
			other.buckets = nullptr;
			other.bucketCount = 0;
			other.elementCount = 0;
		}

		~HashMap() {
			delete[] buckets;
		}

		HashMap& operator = (const HashMap& other) {
			if (buckets != nullptr) {
				delete[] buckets;
			}
			buckets = nullptr;
			bucketCount = 0;
			elementCount = 0;

			if (other.elementCount == 0) {
				return *this;
			}

			bucketCount = other.elementCount < 9 ? 1 : static_cast<uint32>(gk::UpperPowerOfTwo((other.elementCount * 2) >> 3));

			gk_assertm(bucketCount > 0, "Cannot allocate 0 buckets");
			buckets = new Bucket[bucketCount];

			for (const gk::HashPair<Key, Value>* pair : other) { // TODO operate on the new buckets directly
				insert(pair->key, pair->value);// , "should've been able to insert key/value pair from copying other hashmap");
			}
			return *this;
		}

		HashMap& operator = (HashMap&& other) noexcept {
			if (buckets != nullptr) {
				delete[] buckets;
			}
			buckets = other.buckets;
			bucketCount = other.bucketCount;
			elementCount = other.elementCount;
			other.buckets = nullptr;
			other.bucketCount = 0;
			other.elementCount = 0;
			return *this;
		}

#pragma endregion
		
		/* Get the number of entries in the hashmap. */
		[[nodiscard]] forceinline uint32 size() const {
			return elementCount;
		}

		/* Find an optional mutable value in the map. If the key doesn't exist in the map,
		the option will be none. */
		[[nodiscard]] gk::Option<Value*> find(const Key& key) {
			if (elementCount == 0) {
				return gk::Option<Value*>();
			}
			gk_assertm(buckets != nullptr, "Buckets of groups array should be valid after the above check");
			gk_assertm(bucketCount > 0, "After this check, bucket count should be greater than 0");

			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);

			const uint32 bucketIndex = bucketBits.value % bucketCount;
			
			gk::Option<gk::HashPair<Key, Value>*> optionalPair = buckets[bucketIndex].find(key, pairBits);

			if (optionalPair.none()) {
				return gk::Option<Value*>();
			}
			return gk::Option<Value*>(&optionalPair.some()->value);
		}
		
		/* Find an optional const value in the map. If the key doesn't exist in the map,
		the option will be none. */
		[[nodiscard]] gk::Option<const Value*> find(const Key& key) const {
			if (elementCount == 0) return gk::Option<const Value*>();
			gk_assertm(buckets != nullptr, "Buckets of groups array should be valid after the above check");
			gk_assertm(bucketCount > 0, "After this check, bucket count should be greater than 0");

			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);

			const uint32 bucketIndex = bucketBits.value % bucketCount;

			gk::Option<gk::HashPair<Key, Value>*> optionalPair = buckets[bucketIndex].find(key, pairBits);

			if (optionalPair.none()) {
				return gk::Option<const Value*>();
			}
			return gk::Option<const Value*>(&optionalPair.some()->value);
		}

		/* Invalidates any active iterators.
		@return If the key already exists, a valid option with the held value is returned. 
		If the key DOESN'T exist, an empty option is returned. (can be ignored) */
		gk::Option<const Value*> insert(const Key& key, const Value& value) {
			const uint32 requiredCapacity = elementCount + 1;
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}

			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);
			const uint32 bucketIndex = bucketBits.value % bucketCount;

			gk::Option<gk::HashPair<Key, Value>*> optionalPair = buckets[bucketIndex].find(key, pairBits);

			
			if (!optionalPair.none()) { // if key exists
				return gk::Option<const Value*>(&optionalPair.some()->value);
			}

			gk::HashPair<Key, Value>* newPair = new gk::HashPair<Key, Value>(key, value); 
			//newPair->key = key;
			//newPair->value = value;

			buckets[bucketIndex].insert(newPair, pairBits);

			elementCount++;
			return gk::Option<const Value*>();
		}

		/* Invalidates any active iterators.
		@return If the key already exists, a valid option with the held value is returned.
		If the key DOESN'T exist, an empty option is returned. (can be ignored) */
		gk::Option<const Value*> insert(Key&& key, const Value& value) {
			const uint32 requiredCapacity = elementCount + 1;
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}

			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);
			const uint32 bucketIndex = bucketBits.value % bucketCount;

			gk::Option<gk::HashPair<Key, Value>*> optionalPair = buckets[bucketIndex].find(key, pairBits);

			if (!optionalPair.none()) { // if key exists
				return gk::Option<const Value*>(&optionalPair.some()->value);
			}

			gk::HashPair<Key, Value>* newPair = new gk::HashPair<Key, Value>(std::move(key), value);
			//newPair->key = std::move(key);
			//newPair->value = value;

			buckets[bucketIndex].insert(newPair, pairBits);

			elementCount++;
			return gk::Option<const Value*>();
		}

		/* Invalidates any active iterators.
		@return If the key already exists, a valid option with the held value is returned.
		If the key DOESN'T exist, an empty option is returned. (can be ignored) */
		gk::Option<const Value*> insert(const Key& key, Value&& value) {
			const uint32 requiredCapacity = elementCount + 1;
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}

			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);
			const uint32 bucketIndex = bucketBits.value % bucketCount;

			gk::Option<gk::HashPair<Key, Value>*> optionalPair = buckets[bucketIndex].find(key, pairBits);

			if (!optionalPair.none()) { // if key exists
				return gk::Option<const Value*>(&optionalPair.some()->value);
			}

			gk::HashPair<Key, Value>* newPair = new gk::HashPair<Key, Value>(key, std::move(value));
			//newPair->key = key;
			//newPair->value = std::move(value);

			buckets[bucketIndex].insert(newPair, pairBits);

			elementCount++;
			return gk::Option<const Value*>();
		}

		/* Invalidates any active iterators.
		@return If the key already exists, a valid option with the held value is returned.
		If the key DOESN'T exist, an empty option is returned. (can be ignored) */
		gk::Option<const Value*> insert(Key&& key, Value&& value) {
			const uint32 requiredCapacity = elementCount + 1;
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}

			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);
			const uint32 bucketIndex = bucketBits.value % bucketCount;

			gk::Option<gk::HashPair<Key, Value>*> optionalPair = buckets[bucketIndex].find(key, pairBits);

			if (!optionalPair.none()) { // if key exists
				return gk::Option<const Value*>(&optionalPair.some()->value);
			}

			gk::HashPair<Key, Value>* newPair = new gk::HashPair<Key, Value>(std::move(key), std::move(value));
			//newPair->key = std::move(key);
			//newPair->value = std::move(value);

			buckets[bucketIndex].insert(newPair, pairBits);

			elementCount++;
			return gk::Option<const Value*>();
		}

		/* Invalidates any active iterators.
		@return true if the key exists, otherwise false if doesn't exist. (can be ignored) */
		bool erase(const Key& key) {
			const size_t hashCode = hashKey(key);
			const HashBucketBits bucketBits = HashBucketBits(hashCode);
			const PairHashBits pairBits = PairHashBits(hashCode);

			const uint32 bucketIndex = bucketBits.value % bucketCount;
			elementCount--;
			return buckets[bucketIndex].erase(key, pairBits);
		}

		void reserve(const uint32 requiredCapacity) {
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}
		}

		/* Begin of a iterator with const keys and mutable values to the entries in this map. */
		Iterator begin() { return Iterator::iterBegin(this); }

		/* End of a iterator with const keys and mutable values to the entries in this map. */
		Iterator end() { return Iterator::iterEnd(this); }

		/* Begin of a iterator with const keys and const values to the entries in this map. */
		ConstIterator begin() const { return ConstIterator::iterBegin(this); }

		/* End of a iterator with const keys and const values to the entries in this map. */
		ConstIterator end() const { return ConstIterator::iterEnd(this); }

		private:

			bool shouldReallocate(const uint32 requiredCapacity) const {
				// load factor is 0.75

				if (bucketCount == 0) {
					return true;
				}

				//uint32 totalPairCount = 0;
				//for (uint32 i = 0; i < bucketCount; i++) {
				//	gk_assertm(buckets != nullptr, "Buckets array should be valid");
				//	totalPairCount += buckets[i].pairCount;
				//}
				
				const uint32 loadFactorScaledPairCount = (elementCount >> 2) * 3; // multiply by 0.75
				return requiredCapacity > loadFactorScaledPairCount;
			}

			void reallocate(const uint32 requiredCapacity) {
				constexpr size_t pointerBits = 0xFFFFFFFFFFFFULL;

				// Each group in a bucket can hold 8 elements, so no need to over allocate.
				const uint32 newBucketCount = requiredCapacity < 9 ? 1 : static_cast<uint32>(gk::UpperPowerOfTwo((requiredCapacity * 2) >> 3));
				if (newBucketCount <= bucketCount) {
					return;
				}

				gk_assertm(newBucketCount > 0, "Cannot allocate 0 buckets");
				Bucket* newBuckets = new Bucket[newBucketCount];

				for (uint32 bucketIndex = 0; bucketIndex < bucketCount; bucketIndex++) {
					Bucket& oldBucket = buckets[bucketIndex];

					for (uint32 groupIndex = 0; groupIndex < oldBucket.groupCount; groupIndex++) {
						Group& oldGroup = oldBucket.groups[groupIndex];

						for (uint32 i = 0; i < 8; i++) {
							gk::HashPair<Key, Value>* pair = (gk::HashPair<Key, Value>*)(oldGroup.pairs.m512i_u64[i] & pointerBits);
							if (pair == nullptr) {
								continue;
							}
							oldGroup.pairs.m512i_u64[i] = 0;
							const size_t hashCode = hashKey(pair->key);
							const HashBucketBits bucketBits = HashBucketBits(hashCode);
							const PairHashBits pairBits = PairHashBits(hashCode);

							const uint32 bucketIndexToInsert = bucketBits.value % newBucketCount;
							Bucket& bucketToInsert = newBuckets[bucketIndexToInsert];

							bucketToInsert.insert(pair, pairBits);
						}
					}			
				}

				delete[] buckets;
				buckets = newBuckets;
				bucketCount = newBucketCount;
			}

		private:

			Bucket* buckets;
			uint32 bucketCount;
			uint32 elementCount;

	};

}


