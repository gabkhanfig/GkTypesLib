#pragma once

#include "../basic_types.h"
#include "../utility.h"
#include "../doctest/doctest_proxy.h"
//#include "../Utility.h"
#include "hash.h"
#include <intrin.h>
#include "../option/option.h"
#include "../cpu_features/cpu_feature_detector.h"
#include <iostream>
#include "../allocator/allocator.h"
#include "../allocator/heap_allocator.h"

namespace gk 
{
	namespace internal
	{
		/**
		* Determines the bucket index. It's the 57 highest bits, right shifted.
		*/
		struct HashBucketBits {
			static constexpr size_t BITMASK = ~0b01111111ULL;
			size_t value;

			HashBucketBits(const size_t hashCode)
				: value((hashCode& BITMASK) >> 7) {}
		};

		/**
		* Determines the key's index within a group. It's the lowest 7 bits, or'ed with 0b10000000.
		*/
		struct PairHashBits {
			static constexpr i8 BITMASK = 0b01111111;
			i8 value;

			PairHashBits(const size_t hashCode)
				: value((hashCode& BITMASK) | 0b10000000) {}
		};

		u64 hashMapSimdFindEqualHashCodesBitmask(const i8* buffer, PairHashBits hashCode);
		Option<i8> firstAvailableGroupSlot(const i8* buffer);
		Option<usize> bitscanForwardNext(u64* bitmask);
	} // namespace internal

	template<typename Key, typename Value>
	struct HashIterPair {
		const Key* key;
		Value* value;
	};

	/**
	* A general purpose HashMap implementation, utilizing SIMD, and compile time chosen
	* in-place entry storage for fundamental and pointer types for optimal cache access.
	* Supports custom allocators.
	* 
	* Requires that template type `Key` satisfies the concept `Hashable`. This means:
	* 1. Has an overload for gk::hash<> OR is a pointer type.
	* 2. Is equality comparible using const Key&.
	* 
	* NOTE: The key value pairs do not have pointer stability. Any mutation to the hashmap
	* may modify the location where the values are stored, so they should not be directly stored
	* as references.
	* 
	* @param Key: Must satisy the `Hashable` concept.
	* @param Value: Has no restrictions.
	*/
	template<typename Key, typename Value>
		requires Hashable<Key>
	struct HashMap
	{
		static usize hashKey(const Key& key) {
			if constexpr (!std::is_pointer<Key>::value) {
				return gk::hash<Key>(key);
			}
			else {
				const usize ptrAsNum = reinterpret_cast<usize>(key);
				return ptrAsNum;
			}
		}

#pragma region Entry_Containers

	private:

		/**
		* Holds the keys and values, and uses SIMD to look up.
		*/
		struct alignas(64) Group
		{
			static constexpr usize ELEMENTS_PER_GROUP = 64;

		private:

			struct GroupPairsInPlace
			{
				GroupPairsInPlace() : keys{ 0 }, values{ 0 } {}
				GroupPairsInPlace(const GroupPairsInPlace&) = delete;
				GroupPairsInPlace& operator = (const GroupPairsInPlace&) = delete;
				GroupPairsInPlace(GroupPairsInPlace&& other) noexcept {
					memcpy(this->keys, other.keys, sizeof(GroupPairsInPlace::keys));
					memcpy(this->values, other.values, sizeof(GroupPairsInPlace::values));
				}
				GroupPairsInPlace& operator = (GroupPairsInPlace&& other) noexcept {
					memcpy(this->keys, other.keys, sizeof(GroupPairsInPlace::keys));
					memcpy(this->values, other.values, sizeof(GroupPairsInPlace::values));
					return *this;
				}
				~GroupPairsInPlace() = default;

				Key* getKey(usize index) { return &keys[index]; }
				Value* getValue(usize index) { return &values[index]; }
				const Key* getKey(usize index) const { return &keys[index]; }
				const Value* getValue(usize index) const { return &values[index]; }
				void eraseEntry(usize index, Allocator* allocator) {}
				void insertEntry(Key&& key, Value&& value, usize index, Allocator* allocator) {
					keys[index] = key;
					values[index] = value;
				}
			private:
				Key keys[ELEMENTS_PER_GROUP];
				Value values[ELEMENTS_PER_GROUP];
			};

			struct GroupPairsKeysInPlaceValuesHeap
			{
				GroupPairsKeysInPlaceValuesHeap() : keys{ 0 }, values{ nullptr } {}
				GroupPairsKeysInPlaceValuesHeap(const GroupPairsKeysInPlaceValuesHeap&) = delete;
				GroupPairsKeysInPlaceValuesHeap& operator = (const GroupPairsKeysInPlaceValuesHeap&) = delete;
				GroupPairsKeysInPlaceValuesHeap(GroupPairsKeysInPlaceValuesHeap&& other) noexcept {
					memcpy(this->keys, other.keys, sizeof(GroupPairsKeysInPlaceValuesHeap::keys));
					memcpy(this->values, other.values, sizeof(GroupPairsKeysInPlaceValuesHeap::values));
					memset(other.values, 0, sizeof(GroupPairsKeysInPlaceValuesHeap::values));
				}
				GroupPairsKeysInPlaceValuesHeap& operator = (GroupPairsKeysInPlaceValuesHeap&& other) noexcept {
					this->~GroupPairsKeysInPlaceValuesHeap();
					memcpy(this->keys, other.keys, sizeof(GroupPairsKeysInPlaceValuesHeap::keys));
					memcpy(this->values, other.values, sizeof(GroupPairsKeysInPlaceValuesHeap::values));
					memset(other.values, 0, sizeof(GroupPairsKeysInPlaceValuesHeap::values));
					return *this;
				}
				~GroupPairsKeysInPlaceValuesHeap() {
					for (usize i = 0; i < ELEMENTS_PER_GROUP; i++) {
						if (values[i] != nullptr) delete values[i];
					}
				}

				Key* getKey(usize index) { return &keys[index]; }
				Value* getValue(usize index) { return values[index]; }
				const Key* getKey(usize index) const { return &keys[index]; }
				const Value* getValue(usize index) const { return values[index]; }
				void eraseEntry(usize index, Allocator* allocator) {
					if (values[index] == nullptr) return;
					values[index].~Value();
					allocator->freeObject(values[index]);
					//values[index] = nullptr;
				}
				void insertEntry(Key&& key, Value&& value, usize index, Allocator* allocator) {
					keys[index] = key;
					Value* newValueMem = allocator->mallocObject<Value>().ok();
					new (newValueMem) Value(std::move(value)); // move into
					values[index] = newValueMem;
				}
			private:
				Key keys[ELEMENTS_PER_GROUP];
				Value* values[ELEMENTS_PER_GROUP];
			};

			struct GroupPairsBothHeap 
			{
			private:
				struct Pairs {
					Key key;
					Value value;
				};
			public:
				GroupPairsBothHeap() : pairs{ nullptr } {}
				GroupPairsBothHeap(const GroupPairsBothHeap&) = delete;
				GroupPairsBothHeap& operator = (const GroupPairsBothHeap&) = delete;
				GroupPairsBothHeap(GroupPairsBothHeap&& other) noexcept {
					memcpy(this->pairs, other.pairs, sizeof(GroupPairsBothHeap::pairs));
					memset(other.pairs, 0, sizeof(GroupPairsBothHeap::pairs));
				}
				GroupPairsBothHeap& operator = (GroupPairsBothHeap&& other) noexcept {
					this->~GroupPairsBothHeap();
					memcpy(this->pairs, other.pairs, sizeof(GroupPairsBothHeap::pairs));
					memset(other.pairs, 0, sizeof(GroupPairsBothHeap::pairs));
					return *this;
				}
				~GroupPairsBothHeap() {
					for (usize i = 0; i < ELEMENTS_PER_GROUP; i++) {
						if (pairs[i] != nullptr) delete pairs[i];
					}
				}

				Key* getKey(usize index) { return &pairs[index]->key; }
				Value* getValue(usize index) { return &pairs[index]->value; }
				const Key* getKey(usize index) const { return &pairs[index]->key; }
				const Value* getValue(usize index) const { return &pairs[index]->value; }
				void eraseEntry(usize index, Allocator* allocator) {
					if (pairs[index] == nullptr) return;
					pairs[index]->~Pairs();
					allocator->freeObject(pairs[index]);
					//pairs[index] = nullptr;
				}
				void insertEntry(Key&& key, Value&& value, usize index, Allocator* allocator) {
					Pairs* newPairsMem = allocator->mallocObject<Pairs>().ok();
					new (&newPairsMem->key) Key(std::move(key)); // move into
					new (&newPairsMem->value) Value(std::move(value));
					pairs[index] = newPairsMem;
				}
			private:
				Pairs* pairs[ELEMENTS_PER_GROUP];
			};
			
			template<typename T>
			inline static constexpr bool CanTInPlace = std::is_fundamental<T>::value || std::is_pointer<T>::value;

			// It doesn't make sense to store the value in place, if the key is on the heap, since the memory access pattern would be functionally identical.
			using PairT = std::conditional_t<CanTInPlace<Key> && CanTInPlace<Value>, GroupPairsInPlace,			// Keys and values in place
				std::conditional_t<CanTInPlace<Key> && !CanTInPlace<Value>, GroupPairsKeysInPlaceValuesHeap,	// Keys in place, Values in heap
				GroupPairsBothHeap>>;																																					// Keys and values in heap

		public:

			Group() : hashMasks{ 0 } { /*std::cout << "group instantiation\n";*/ }
			Group(const Group&) = delete;
			Group& operator = (const Group&) = delete;
			Group(Group&& other) noexcept {
				memcpy(this->hashMasks, other.hashMasks, sizeof(Group::hashMasks));
				//memset(other.hashMasks, 0, sizeof(Group::hashMasks));
				this->pairs = std::move(other.pairs);
			}
			Group& operator = (Group&& other) = delete;
			~Group() {};

			/**
			*/
			void free(Allocator* allocator) {
				for (usize i = 0; i < ELEMENTS_PER_GROUP; i++) {
					pairs.eraseEntry(i, allocator);
				}
			}

			/**
			*/
			Option<Value*> find(const Key& key, internal::PairHashBits hashCode) {
				// Yeah it returns mutable pointers but the Bucket interface will take care of that
				using OptionT = Option<Value*>;

				u64 bitmask = internal::hashMapSimdFindEqualHashCodesBitmask(this->hashMasks, hashCode);
				Option<usize> firstOption = internal::bitscanForwardNext(&bitmask);
				if (firstOption.none()) return OptionT();
				usize index = firstOption.some();
				const Key* pairKey = this->pairs.getKey(index);
				if (*pairKey == key) {
					return OptionT(this->pairs.getValue(index));
				}
				while (true) { // if first hash doesn't match, loop until there is no other.
					Option<usize> option = internal::bitscanForwardNext(&bitmask);
					if (option.none()) return OptionT();
					index = option.some();
					const Key* pairKey = this->pairs.getKey(index);
					if (*pairKey == key) {
						return OptionT(this->pairs.getValue(index));
					}
				}
			}

			/**
			*/
			Option<const Value*> findConst(const Key& key, internal::PairHashBits hashCode) const {
				// Yeah it returns mutable pointers but the Bucket interface will take care of that
				using OptionT = Option<const Value*>;

				u64 bitmask = internal::hashMapSimdFindEqualHashCodesBitmask(this->hashMasks, hashCode);
				Option<usize> firstOption = internal::bitscanForwardNext(&bitmask);
				if (firstOption.none()) return OptionT();
				usize index = firstOption.some();
				const Key* pairKey = this->pairs.getKey(index);
				if (*pairKey == key) {
					return OptionT(this->pairs.getValue(index));
				}
				while (true) { // if first hash doesn't match, loop until there is no other.
					Option<usize> option = internal::bitscanForwardNext(&bitmask);
					if (option.none()) return OptionT();
					index = option.some();
					const Key* pairKey = this->pairs.getKey(index);
					if (*pairKey == key) {
						return OptionT(this->pairs.getValue(index));
					}
				}
			}

			/** 
			* If the entire thing is full, returns false, otherwise inserts the pair and lower hash bits. 
			*/
			bool insert(Key& key, Value& value, internal::PairHashBits hashCode, Allocator* allocator) {
				Option<i8> opt = internal::firstAvailableGroupSlot(this->hashMasks);
				if (opt.none()) return false;

				const usize index = opt.some();
				pairs.insertEntry(std::move(key), std::move(value), index, allocator);
				hashMasks[index] = hashCode.value;
				return true;
			}

			/**
			* If the entry does not exist, returns false, otherwise erases the entry and frees all associated memory of it.
			*/
			bool erase(const Key& key, internal::PairHashBits hashCode, Allocator* allocator) {
				u64 bitmask = internal::hashMapSimdFindEqualHashCodesBitmask(this->hashMasks, hashCode);
				Option<usize> firstOption = internal::bitscanForwardNext(&bitmask);
				if (firstOption.none()) return false;
				usize index = firstOption.some();
				Key* pairKey = pairs.getKey(index);
				if (*pairKey == key) {
					hashMasks[index] = 0;
					pairs.eraseEntry(index, allocator);
					return true;
				}
				while (true) { // if first hash doesn't match, loop until there is no other.
					Option<usize> option = internal::bitscanForwardNext(&bitmask);
					if (option.none()) return false;
					index = option.some();
					Key* pairKey = this->pairs.getKey(index);
					if (*pairKey == key) {
						hashMasks[index] = 0;
						pairs.eraseEntry(index, allocator);
						return true;
					}
				}
			}



			i8 hashMasks[64]; // __m512i
			PairT pairs;

		}; // struct Group

		/**
		* Holds 1 or more groups
		*/
		struct Bucket
		{
			Bucket(Allocator* allocator) : groupCount(1)
			{
				groups = allocator->mallocBuffer<Group>(1).ok();
				new (groups) Group();
			}

			Bucket(const Bucket&) = delete;
			Bucket& operator = (const Bucket&) = delete;
			Bucket(Bucket&& other) noexcept {
				groups = other.groups;
				groupCount = other.groupCount;
				other.groups = nullptr;
				other.groupCount = 0;
			}
			Bucket& operator = (Bucket&& other) = delete;

			~Bucket() {}

			void free(Allocator* allocator) {
				for (usize i = 0; i < groupCount; i++) {
					Group& group = groups[i];
					group.free(allocator);
					//group.~Group();
				}
				allocator->freeBuffer(groups, groupCount);
				groupCount = 0;
			}

			Option<Value*> find(const Key& key, internal::PairHashBits hashCode) {
				for (usize i = 0; i < groupCount; i++) {
					Group& group = groups[i];
					Option<Value*> foundOpt = group.find(key, hashCode);
					if (foundOpt.isSome()) return foundOpt;
				}
				return Option<Value*>();
			}

			Option<const Value*> findConst(const Key& key, internal::PairHashBits hashCode) const {
				for (usize i = 0; i < groupCount; i++) {
					Group& group = groups[i];
					Option<const Value*> foundOpt = group.findConst(key, hashCode);
					if (foundOpt.isSome()) return foundOpt;
				}
				return Option<const Value*>();
			}

			void insert(Key&& key, Value&& value, internal::PairHashBits hashCode, Allocator* allocator) {
				for (usize i = 0; i < groupCount; i++) {
					if (groups[i].insert(key, value, hashCode, allocator)) {
						return; // successfully inserted
					}
				}
				const usize newGroupCount = groupCount + 1;
				Group* newGroups = allocator->mallocBuffer<Group>(1).ok();

				for (usize i = 0; i < groupCount; i++) {
					new (newGroups + i) Group(std::move(groups[i]));
				}
				allocator->freeBuffer(groups, groupCount);
				groups = newGroups;
				groupCount = newGroupCount;

				const bool success = groups[newGroupCount - 1].insert(key, value, hashCode, allocator);
				check_message(success, "this shouldn't ever fail");
			}

			bool erase(const Key& key, internal::PairHashBits hashCode, Allocator* allocator) {
				for (usize i = 0; i < groupCount; i++) {
					if (groups[i].erase(key, hashCode, allocator)) {
						return true;
					}
				}
				return false;
			}


			Group* groups;
			usize groupCount;
		};

#pragma endregion

#pragma region Iterator

		friend class Iterator;
		friend class ConstIterator;

		class Iterator {
		public:

			static Iterator iterBegin(HashMap* map) {
				Iterator iter;
				iter._map = map;
				iter._currentBucket = map->_buckets;
				if (map->_buckets != nullptr) {
					iter.step();
				}		
				return iter;
			}

			static Iterator iterEnd(HashMap* map) {
				Iterator iter;
				iter._map = map;
				iter._currentBucket = map->_buckets + map->_bucketCount;
				return iter;
			}

			Iterator& operator++() {
				step();
				return *this;
			}

			bool operator !=(const Iterator& other) const {
				return _currentBucket != other._currentBucket;
			}

			HashIterPair<Key, Value> operator*() const {
				Group& group = _currentBucket->groups[_currentGroupIndex];
				//if (group.hashMasks[_currentElementIndex] == 0) {
				//	std::cout << "first in hashmap is not a valid element\n";
				//	for (int i = 0; i < 64; i++) {
				//		std::cout << int(group.hashMasks[_currentElementIndex]) << ", ";
				//	}
				//	abort();
				//}
				HashIterPair<Key, Value> pair;
				pair.key = group.pairs.getKey(_currentElementIndex);
				pair.value = group.pairs.getValue(_currentElementIndex);
				return pair;
			}

			void step() {
				while (true) {
					_currentElementIndex++;
					if (_currentElementIndex == Group::ELEMENTS_PER_GROUP) {
						_currentElementIndex = 0;
						_currentGroupIndex++;
					}

					if (_currentGroupIndex == _currentBucket->groupCount) {
						_currentBucket++;
						_currentElementIndex = 0;
						_currentGroupIndex = 0;
					}

					if (_currentBucket == _map->_buckets + _map->_bucketCount) {
						return;
					}

					Group& group = _currentBucket->groups[_currentGroupIndex];
					if (group.hashMasks[_currentElementIndex] != 0) {
						return;
					}
				}
			}

		private:
			Iterator() : _map(nullptr), _currentBucket(nullptr), _currentGroupIndex(0), _currentElementIndex(0) {}
			HashMap* _map;
			Bucket* _currentBucket;
			usize _currentGroupIndex;
			usize _currentElementIndex;
		};

		class ConstIterator {
		public:

			static ConstIterator iterBegin(const HashMap* map) {
				ConstIterator iter;
				iter._map = map;
				iter._currentBucket = map->_buckets;
				if (map->_buckets != nullptr) {
					iter.step();
				}
				return iter;
			}

			static ConstIterator iterEnd(const HashMap* map) {
				ConstIterator iter;
				iter._map = map;
				iter._currentBucket = map->_buckets + map->_bucketCount;
				return iter;
			}

			ConstIterator& operator++() {
				step();
				return *this;
			}

			bool operator !=(const ConstIterator& other) const {
				return _currentBucket != other._currentBucket;
			}

			HashIterPair<Key, const Value> operator*() const {
				Group& group = _currentBucket->groups[_currentGroupIndex];
				HashIterPair<Key, const Value> pair;
				pair.key = group.pairs.getKey(_currentElementIndex);
				pair.value = group.pairs.getValue(_currentElementIndex);
				return pair;
			}

			void step() {
				while (true) {
					_currentElementIndex++;
					if (_currentElementIndex == Group::ELEMENTS_PER_GROUP) {
						_currentElementIndex = 0;
						_currentGroupIndex++;
					}

					if (_currentGroupIndex == _currentBucket->groupCount) {
						_currentBucket++;
						_currentElementIndex = 0;
						_currentGroupIndex = 0;
					}

					if (_currentBucket == _map->_buckets + _map->_bucketCount) {
						return;
					}

					Group& group = _currentBucket->groups[_currentGroupIndex];
					if (group.hashMasks[_currentElementIndex] != 0) {
						return;
					}
				}
			}

		private:
			ConstIterator() : _map(nullptr), _currentBucket(nullptr), _currentGroupIndex(0), _currentElementIndex(0) {}
			const HashMap* _map;
			const Bucket* _currentBucket;
			usize _currentGroupIndex;
			usize _currentElementIndex;
		};
		
#pragma endregion

	public:

		HashMap() 
			: _buckets(nullptr), _bucketCount(0), _elementCount(0), _allocator(globalHeapAllocator()->clone()) 
		{}

		HashMap(const HashMap& other) 
			: _buckets(nullptr), _bucketCount(0), _elementCount(0), _allocator(other._allocator.clone())
		{
			if (other._elementCount == 0) {
				return;
			}

			reallocate(other._elementCount);
			for (auto pair : other) {
				insert(Key(*pair.key), Value(*pair.value));
			}
		}

		HashMap(HashMap&& other) noexcept
			: _buckets(other._buckets), _bucketCount(other._bucketCount), _elementCount(other._elementCount), _allocator(std::move(other._allocator))
		{
			other._buckets = nullptr;
		}

		HashMap& operator = (const HashMap& other) {
			if (_buckets != nullptr) {
				for (usize i = 0; i < _bucketCount; i++) {
					_buckets[i].free(&_allocator);
				}
				_bucketCount = 0;
				// DONT free the actual buckets buffer yet
			}

			_elementCount = 0;
			if (other._elementCount == 0) {
				return *this;
			}

			reallocate(other._elementCount);
			for (auto pair : other) {
				insert(Key(*pair.key), Value(*pair.value));
			}
			return *this;
		}

		HashMap& operator = (HashMap&& other) noexcept {
			if (_buckets != nullptr) {
				for (usize i = 0; i < _bucketCount; i++) {
					_buckets[i].free(&_allocator);
				}
				_allocator.freeBuffer(_buckets, _bucketCount);
			}

			_buckets = other._buckets;
			_bucketCount = other._bucketCount;
			_elementCount = other._elementCount;
			_allocator = std::move(other._allocator);
			other._buckets = nullptr;
			return *this;
		}

		~HashMap() {
			if (_buckets == nullptr) return;

			for (usize i = 0; i < _bucketCount; i++) {
				_buckets[i].free(&_allocator);
			}
			_allocator.freeBuffer(_buckets, _bucketCount);
		}

		/**
		* @return Number of elements stored in the HashMap
		*/
		[[nodiscard]] usize size() const { return _elementCount; }

		/**
		* @return Immutable reference to the allocator used by this HashMap
		*/
		[[nodiscard]] const Allocator& allocator() const { return _allocator; }

		/**
		* Finds an entry within the HashMap, returning an optional mutable value.
		* 
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		* 
		* @return Some if the key exists in the map, or None if it doesn't
		*/
		[[nodiscard]] Option<Value*> find(const Key& key) {
			if (_elementCount == 0) {
				return gk::Option<Value*>();
			}
			check_message(_buckets != nullptr, "Buckets of groups array should be valid after the above check");
			check_message(_bucketCount > 0, "After this check, bucket count should be greater than 0");

			const usize hashCode = hashKey(key);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);

			const usize bucketIndex = bucketBits.value % _bucketCount;

			return _buckets[bucketIndex].find(key, pairBits);
		}

		/**
		* Finds an entry within the HashMap, returning an optional immutable value.
		*
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @return Some if the key exists in the map, or None if it doesn't
		*/
		[[nodiscard]] Option<const Value*> find(const Key& key) const {
			if (_elementCount == 0) {
				return gk::Option<const Value*>();
			}
			check_message(_buckets != nullptr, "Buckets of groups array should be valid after the above check");
			check_message(_bucketCount > 0, "After this check, bucket count should be greater than 0");

			const usize hashCode = hashKey(key);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);

			const usize bucketIndex = bucketBits.value % _bucketCount;

			return _buckets[bucketIndex].findConst(key, pairBits);
		}

		/**
		* Invalidates any iterators.
		* Inserts an entry into the HashMap if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		* 
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		* 
		* @return The entry if it already exists in the HashMap, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		Option<Value*> insert(Key&& key, Value&& value) {
			Key tempKey = std::move(key);
			const usize hashCode = hashKey(tempKey);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);

			if (_bucketCount > 0) {			
				const usize bucketIndex = bucketBits.value % _bucketCount;
				Option<Value*> found = _buckets[bucketIndex].find(tempKey, pairBits);

				if (found.isSome()) return found;
			}
			
			performInsert(std::move(tempKey), std::move(value), hashCode, pairBits);
			return Option<Value*>();
		}

		/**
		* Invalidates any iterators.
		* Inserts an entry into the HashMap if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		* 
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		* 
		* @return The entry if it already exists in the HashMap, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		Option<Value*> insert(const Key& key, Value&& value) {
			const usize hashCode = hashKey(key);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);

			if (_bucketCount > 0) {			
				const usize bucketIndex = bucketBits.value % _bucketCount;
				Option<Value*> found = _buckets[bucketIndex].find(key, pairBits);

				if (found.isSome()) return found;
			}
			
			performInsert(Key(key), std::move(value), hashCode, pairBits);
			return Option<Value*>();
		}

		/**
		* Invalidates any iterators.
		* Inserts an entry into the HashMap if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		*
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @return The entry if it already exists in the HashMap, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		Option<Value*> insert(Key&& key, const Value& value) {
			Key tempKey = std::move(key);
			const usize hashCode = hashKey(tempKey);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);

			if (_bucketCount > 0) {
				const usize bucketIndex = bucketBits.value % _bucketCount;
				Option<Value*> found = _buckets[bucketIndex].find(tempKey, pairBits);

				if (found.isSome()) return found;
			}

			performInsert(std::move(tempKey), Value(value), hashCode, pairBits);
			return Option<Value*>();
		}

		/**
		* Invalidates any iterators.
		* Inserts an entry into the HashMap if it DOES NOT exist.
		* If it does exist, an option containing the existing value will be returned, which can be modified.
		*
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @return The entry if it already exists in the HashMap, or a None option if it didn't exist and thus was added.
		* Can be ignored.
		*/
		Option<Value*> insert(const Key& key, const Value& value) {
			const usize hashCode = hashKey(key);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);

			if (_bucketCount > 0) {
				const usize bucketIndex = bucketBits.value % _bucketCount;
				Option<Value*> found = _buckets[bucketIndex].find(key, pairBits);

				if (found.isSome()) return found;
			}

			performInsert(Key(key), Value(value), hashCode, pairBits);
			return Option<Value*>();
		}

		/**
		* Invalidates any iterators.
		* Erases an entry from the HashMap.
		* 
		* @return `true` if the key exists and was erased, or `false` if it wasn't in the HashMap.
		* Can be ignored.
		*/
		bool erase(const Key& key) {
			const usize hashCode = hashKey(key);
			const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
			const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);
			const usize bucketIndex = bucketBits.value % _bucketCount;

			_elementCount--;
			return _buckets[bucketIndex].erase(key, pairBits, &_allocator);
		}

		/**
		* Reserves additional capacity in the HashMap. If it decides to reallocate,
		* all keys will be rehashed. The HashMap will be able to store at LEAST 
		* `size()` + additional entries.
		* 
		* @param additional: Minimum amount of elements to reserve extra capacity for
		*/
		void reserve(usize additional) {
			const usize requiredCapacity = _elementCount + additional;
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}
		}

		/**
		* Begin of an Iterator with immutable keys, and mutable values, over 
		* each entry in the HashMap.
		* 
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		* 
		* @return Begin of mutable iterator
		*/
		Iterator begin() { return Iterator::iterBegin(this); }

		/**
		* End of an Iterator with immutable keys, and mutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of mutable iterator
		*/
		Iterator end() { return Iterator::iterEnd(this); }

		/**
		* Begin of an Iterator with immutable keys, and immutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return Begin of immutable iterator
		*/
		ConstIterator begin() const { return ConstIterator::iterBegin(this); }

		/**
		* End of an Iterator with immutable keys, and immutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of immutable iterator
		*/
		ConstIterator end() const { return ConstIterator::iterEnd(this); }

	private:

		usize calculateNewBucketCount(usize requiredCapacity) {
			return requiredCapacity <= Group::ELEMENTS_PER_GROUP ?
				1 :
				static_cast<usize>(gk::upperPowerOfTwo(requiredCapacity * (Group::ELEMENTS_PER_GROUP / 2)));
		}

		bool shouldReallocate(usize requiredCapacity) const {
			// load factor is 0.75
			if (_bucketCount == 0) {
				return true;
			}
			const usize loadFactorScaledPairCount = (_elementCount >> 2) * 3; // multiply by 0.75
			return requiredCapacity > loadFactorScaledPairCount;
		}

		void reallocate(usize requiredCapacity) {
			const usize newBucketCount = calculateNewBucketCount(requiredCapacity);
			if (newBucketCount <= _bucketCount) {
				return;
			}

			// Construct the new buckets
			Bucket* newBuckets = _allocator.mallocBuffer<Bucket>(newBucketCount).ok();
			for (usize i = 0; i < newBucketCount; i++) {
				new (newBuckets + i) Bucket(&_allocator);
			}

			// Loop through the old buckets, and move their data to the new buckets
			// Rehashes all keys
			for (usize oldBucketIndex = 0; oldBucketIndex < _bucketCount; oldBucketIndex++) {
				Bucket& bucket = _buckets[oldBucketIndex];
				for (usize groupIndex = 0; groupIndex < bucket.groupCount; groupIndex++) {
					Group& group = bucket.groups[groupIndex];
					for (usize i = 0; i < Group::ELEMENTS_PER_GROUP; i++) {
						// Does not contain anything
						if (group.hashMasks[i] == 0) continue;

						Key* key = group.pairs.getKey(i);
						Value* value = group.pairs.getValue(i);

						const usize hashCode = hashKey(*key);
						const internal::HashBucketBits bucketBits = internal::HashBucketBits(hashCode);
						const internal::PairHashBits pairBits = internal::PairHashBits(hashCode);
						const usize newBucketIndex = bucketBits.value % newBucketCount;

						newBuckets[newBucketIndex].insert(std::move(*key), std::move(*value), pairBits, &_allocator);
					}
				}

				// Free existing buckets
				bucket.free(&_allocator);
			}
			if (_buckets) {
				_allocator.freeBuffer(_buckets, _bucketCount);
			}
			
			_buckets = newBuckets;
			_bucketCount = newBucketCount;
		}

		void performInsert(Key&& key, Value&& value, internal::HashBucketBits bucketBits, internal::PairHashBits pairBits) {
			const usize requiredCapacity = _elementCount + 1;
			if (shouldReallocate(requiredCapacity)) {
				reallocate(requiredCapacity);
			}

			const usize bucketIndex = bucketBits.value % _bucketCount;
			_buckets[bucketIndex].insert(std::move(key), std::move(value), pairBits, &_allocator);
			_elementCount++;
		}

	private:

		Bucket* _buckets;
		usize _bucketCount;
		usize _elementCount;
		Allocator _allocator;

	}; // struct HashMap
} // namespace gk


