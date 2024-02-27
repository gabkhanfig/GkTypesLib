#pragma once

#include "hash.h"
#include "../doctest/doctest_proxy.h"
#include "../option/option.h"
#include "../allocator/allocator.h"
#include "../utility.h"

namespace gk
{
	namespace internal {
		template<typename Key, typename Value, usize GROUP_ALLOC_SIZE>
		struct HashMapGroup;
	}

	/**
	* A general purpose HashMap implementation, utilizing SIMD, and compile time chosen
	* in-place entry storage for appropriate types for optimal cache access.
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
	* @param GROUP_ALLOC_SIZE: Amount of pairs to reserve per group of pairs.
	* Must be a multiple of 16
	*/
	template<typename Key, typename Value, usize GROUP_ALLOC_SIZE = 32>
		requires (GROUP_ALLOC_SIZE % 16 == 0 && Hashable<Key>)
	struct HashMap
	{
	private:

		using GroupT = internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>;

		static constexpr usize hashKey(const Key& key);

	public:

		/**
		* Create a new instance of `HashMap`.
		* At runtime, uses the `gk::globalHeapAllocator()`.
		* For using custom allocators, see `init()` and `withCapacity()` functions.
		*/
		constexpr HashMap();

		/**
		*/
		constexpr HashMap(const HashMap& other);

		/**
		*/
		constexpr HashMap(HashMap&& other) noexcept;

		constexpr ~HashMap();

		/**
		*/
		constexpr HashMap& operator = (const HashMap& other);

		/**
		*/
		constexpr HashMap& operator = (HashMap&& other) noexcept;

		/**
		* @return Number of elements stored in the HashMap
		*/
		[[nodiscard]] constexpr usize size() const { return _elementCount; }

		/**
		* In constexpr, a custom allocator is not used, and thus this function is invalid in constexpr.
		*
		* @return Immutable reference to the allocator used by this HashMap.
		*/
		[[nodiscard]] const AllocatorRef& allocator() const { return _allocator; }

		/**
		* Finds an entry within the HashMap, returning an optional mutable value.
		*
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @return Some if the key exists in the map, or None if it doesn't
		*/
		[[nodiscard]] constexpr Option<Value*> find(const Key& key);

		/**
		* Finds an entry within the HashMap, returning an optional immutable value.
		*
		* NOTE: The HashMap does not have pointer stability. Subsequent mutation operations on the HashMap may
		* invalidate the returned Some pointer due to the underlying data being moved to a new location.
		*
		* @return Some if the key exists in the map, or None if it doesn't
		*/
		[[nodiscard]] constexpr Option<const Value*> find(const Key& key) const;

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
		constexpr Option<Value*> insert(Key&& key, Value&& value);

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
		constexpr Option<Value*> insert(const Key& key, Value&& value);

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
		constexpr Option<Value*> insert(Key&& key, const Value& value);

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
		constexpr Option<Value*> insert(const Key& key, const Value& value);

		/**
		* Invalidates any iterators.
		* Erases an entry from the HashMap.
		*
		* @return `true` if the key exists and was erased, or `false` if it wasn't in the HashMap.
		* Can be ignored.
		*/
		constexpr bool erase(const Key& key);

		/**
		* Reserves additional capacity in the HashMap. If it decides to reallocate,
		* all keys will be rehashed. The HashMap will be able to store at LEAST
		* `size()` + additional entries.
		*
		* @param additional: Minimum amount of elements to reserve extra capacity for
		*/
		constexpr void reserve(usize additional);

		struct Iterator;
		struct ConstIterator;

		/**
		* Begin of an Iterator with immutable keys, and mutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return Begin of mutable iterator
		*/
		constexpr Iterator begin();

		/**
		* End of an Iterator with immutable keys, and mutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of mutable iterator
		*/
		constexpr Iterator end();

		/**
		* Begin of an Iterator with immutable keys, and immutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return Begin of immutable iterator
		*/
		constexpr ConstIterator begin() const;

		/**
		* End of an Iterator with immutable keys, and immutable values, over
		* each entry in the HashMap.
		*
		* `insert()`, `erase()`, `reserve()`, `std::move()` and other mutation operations
		* can be assumed to invalidate the iterator.
		*
		* @return End of immutable iterator
		*/
		constexpr ConstIterator end() const;

		struct Iterator {
			struct Pair {
				const Key& key;
				Value& value;
			};

			static constexpr Iterator iterBegin(HashMap* map);

			static constexpr Iterator iterEnd(HashMap* map);

			constexpr bool operator ==(const Iterator& other) const;

			constexpr Pair operator*() const;

			constexpr Iterator& operator++();

		private:

			constexpr Iterator() : _map(nullptr), _currentGroup(nullptr), _currentElementIndex(0) {}
			HashMap* _map;
			internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>* _currentGroup;
			usize _currentElementIndex;
		}; // struct Iterator

		struct ConstIterator {
			struct Pair {
				const Key& key;
				const Value& value;
			};

			static constexpr ConstIterator iterBegin(const HashMap* map);

			static constexpr ConstIterator iterEnd(const HashMap* map);

			constexpr bool operator ==(const ConstIterator& other) const;

			constexpr Pair operator*() const;

			constexpr ConstIterator& operator++();

		private:

			constexpr ConstIterator() : _map(nullptr), _currentGroup(nullptr), _currentElementIndex(0) {}
			const HashMap* _map;
			const internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>* _currentGroup;
			usize _currentElementIndex;
		}; // struct ConstIterator

	private:

		static constexpr usize calculateNewGroupCount(usize requiredCapacity);

		constexpr bool shouldReallocate(usize requiredCapacity) const;

		constexpr void reallocate(usize requiredCapacity);

	private:

		GroupT* _groups;
		usize _groupCount;
		usize _elementCount;
		AllocatorRef _allocator;
	};

	namespace internal
	{
		/**
		* Determines the bucket index. It's the 57 highest bits, right shifted.
		*/
		struct HashMapGroupBitmask {
			static constexpr usize BITMASK = ~0b01111111ULL;
			usize value;

			constexpr HashMapGroupBitmask(const usize hashCode)
				: value((hashCode& BITMASK) >> 7) {}
		};

		/**
		* Determines the key's index within a group. It's the lowest 7 bits, or'ed with 0b10000000.
		*/
		struct HashMapPairBitmask {
			static constexpr i8 BITMASK = 0b01111111;
			i8 value;

			constexpr HashMapPairBitmask(const usize hashCode)
				: value((hashCode& BITMASK) | 0b10000000) {}
		};

#pragma region Pair_Containers

		template<typename Key, typename Value>
		struct HashPairInPlace
		{
			constexpr HashPairInPlace() = default;
			constexpr HashPairInPlace(const HashPairInPlace&) = delete;
			constexpr HashPairInPlace(HashPairInPlace&&) noexcept = default;
			constexpr HashPairInPlace& operator = (const HashPairInPlace&) = delete;
			constexpr HashPairInPlace& operator = (HashPairInPlace&&) noexcept = default;
			constexpr ~HashPairInPlace() = default;

			constexpr Key* getKey() { return &key; }
			constexpr Value* getValue() { return &value; }
			constexpr const Key* getKey() const { return &key; }
			constexpr const Value* getValue() const { return &value; }
			constexpr usize hashCode() const { return gk::hash<Key>(key); }

			constexpr void erase(AllocatorRef* allocator);
			constexpr void insert(Key&& inKey, Value&& inValue, usize inHashCode, AllocatorRef* allocator);

		private:
			Key key;
			Value value;
		};

		template<typename Key, typename Value>
		struct HashPairOnHeap
		{
			struct Pair {
				Key key;
				Value value;
				usize hashCode;
			};

			constexpr HashPairOnHeap() : pair(nullptr) {}
			constexpr HashPairOnHeap(const HashPairOnHeap&) = delete;
			constexpr HashPairOnHeap(HashPairOnHeap&& other) noexcept;
			constexpr HashPairOnHeap& operator = (const HashPairOnHeap&) = delete;
			constexpr HashPairOnHeap& operator = (HashPairOnHeap&& other) noexcept;
			constexpr ~HashPairOnHeap() = default;

			constexpr Key* getKey() { return &pair->key; }
			constexpr Value* getValue() { return &pair->value; }
			constexpr const Key* getKey() const { return &pair->key; }
			constexpr const Value* getValue() const { return &pair->value; }
			constexpr usize hashCode() const { return pair->hashCode; }

			constexpr void erase(AllocatorRef* allocator);
			constexpr void insert(Key&& inKey, Value&& inValue, usize inHashCode, AllocatorRef* allocator);

			Pair* pair;
		};

#pragma endregion

		template<typename T>
		inline static constexpr bool CAN_T_IN_PLACE() { return sizeof(T) <= (sizeof(gk::usize) / 2); }

		template<typename Key, typename Value>
		static constexpr usize calculateGroupAllocAlignment(usize groupAllocSize);

		// Find first available empty slot in a group using 16 byte simd instructions
		Option<usize> firstAvailableGroupSlot16(const i8* buffer, usize capacity);
		// Find first available empty slot in a group using 32 byte simd instructions
		Option<usize> firstAvailableGroupSlot32(const i8* buffer, usize capacity);
		// Find first available empty slot in a group using 64 OR 32 byte simd instructions, 
		// depending on what's available to the user's system at runtime.
		Option<usize> firstAvailableGroupSlot64(const i8* buffer, usize capacity);

		u64 simdFindEqualHashCodeBitmaskIteration16(const i8* bufferOffsetIter, HashMapPairBitmask hashBits);
		u64 simdFindEqualHashCodeBitmaskIteration32(const i8* bufferOffsetIter, HashMapPairBitmask hashBits);
		u64 simdFindEqualHashCodeBitmaskIteration64(const i8* bufferOffsetIter, HashMapPairBitmask hashBits);

		template<typename PairT>
		constexpr usize calculateHashMapGroupRuntimeAllocationSize(usize requiredCapacity);

		template<typename Key, typename Value, usize GROUP_ALLOC_SIZE>
		struct HashMapGroup {

			using PairT = std::conditional_t<CAN_T_IN_PLACE<Key>() && CAN_T_IN_PLACE<Value>(),
				HashPairInPlace<Key, Value>,	// Keys and values in place
				HashPairOnHeap<Key, Value>>;	// Keys and values in heap

			static constexpr usize ALLOC_ALIGNMENT = calculateGroupAllocAlignment<Key, Value>(GROUP_ALLOC_SIZE);
			static_assert(ALLOC_ALIGNMENT % 16 == 0);

			i8* hashMasks;
			PairT* pairs;
			usize pairCount;
			usize capacity;

			constexpr HashMapGroup() : hashMasks(nullptr), pairs(nullptr), pairCount(0), capacity(0) {}
			constexpr HashMapGroup(const HashMapGroup&) = delete;
			constexpr HashMapGroup(HashMapGroup&& other) noexcept;
			constexpr HashMapGroup& operator = (const HashMapGroup&) = delete;
			constexpr HashMapGroup& operator = (HashMapGroup&& other) noexcept;
			constexpr ~HashMapGroup();

			constexpr void defaultInit(AllocatorRef* allocator);

			constexpr void free(AllocatorRef* allocator);

			constexpr Option<Value*> find(const Key& key, usize hashCode);

			constexpr Option<const Value*> find(const Key& key, usize hashCode) const;

			constexpr Option<Value*> insert(Key&& key, Value&& value, usize hashCode, AllocatorRef* allocator);

			constexpr bool erase(const Key& key, usize hashCode, AllocatorRef* allocator);

			//private:

			constexpr Option<usize> findIndexOfKey(const Key& key, usize hashCode) const;

			Option<usize> findIndexOfKeyRuntime(const Key& key, usize hashCode) const;

			Option<usize> findIndexOfKeyRuntime16(const Key& key, usize hashCode) const;

			Option<usize> findIndexOfKeyRuntime32(const Key& key, usize hashCode) const;

			Option<usize> findIndexOfKeyRuntime64(const Key& key, usize hashCode) const;

			constexpr Option<usize> firstAvailableGroupSlot() const;

			constexpr void reallocate(usize newCapacity, AllocatorRef* allocator);

		}; // struct HashMapGroup

	} // namespace internal
} // namespace gk

template<typename Key, typename Value>
constexpr gk::usize gk::internal::calculateGroupAllocAlignment(usize groupAllocSize)
{
	usize keyValueMinAlignment = alignof(Key) < alignof(Value) ? alignof(Value) : alignof(Key);
	usize minGroupAlignment = 0;
	if (groupAllocSize == 16) {
		minGroupAlignment = 16;
	}
	else if (groupAllocSize == 32) {
		minGroupAlignment = 32;
	}
	else {
		minGroupAlignment = 64;
	}
	if (keyValueMinAlignment < minGroupAlignment) {
		return minGroupAlignment;
	}
	else {
		return keyValueMinAlignment;
	}
}

template<typename PairT>
inline constexpr gk::usize gk::internal::calculateHashMapGroupRuntimeAllocationSize(usize requiredCapacity)
{
	check_eq((requiredCapacity % 16), 0);

	return requiredCapacity + (sizeof(PairT) * requiredCapacity);
}

template<typename Key, typename Value>
inline constexpr void gk::internal::HashPairInPlace<Key, Value>::erase(AllocatorRef* allocator)
{
	key.~Key();
	value.~Value();
}

template<typename Key, typename Value>
inline constexpr void gk::internal::HashPairInPlace<Key, Value>::insert(Key&& inKey, Value&& inValue, usize inHashCode, gk::AllocatorRef* allocator)
{
	if (std::is_constant_evaluated()) {
		key = std::move(inKey);
		value = std::move(inValue);
	}
	else {
		new (&key) Key(std::move(inKey));
		new (&value) Value(std::move(inValue));
	}
}

template<typename Key, typename Value>
inline constexpr gk::internal::HashPairOnHeap<Key, Value>::HashPairOnHeap(HashPairOnHeap&& other) noexcept
{
	pair = other.pair;
	other.pair = nullptr;
}

template<typename Key, typename Value>
inline constexpr gk::internal::HashPairOnHeap<Key, Value>& gk::internal::HashPairOnHeap<Key, Value>::operator=(HashPairOnHeap&& other) noexcept
{
	//check(pair == nullptr);
	pair = other.pair;
	other.pair = nullptr;
	return *this;
}

template<typename Key, typename Value>
inline constexpr void gk::internal::HashPairOnHeap<Key, Value>::erase(AllocatorRef* allocator)
{
	if (std::is_constant_evaluated()) {
		delete pair;
	}
	else {
		pair->~Pair();
		allocator->freeObject(pair);
	}
}

template<typename Key, typename Value>
inline constexpr void gk::internal::HashPairOnHeap<Key, Value>::insert(Key&& inKey, Value&& inValue, usize inHashCode, AllocatorRef* allocator)
{
	check(pair == nullptr);
	if (std::is_constant_evaluated()) {
		pair = new Pair();
		pair->key = std::move(inKey);
		pair->value = std::move(inValue);
		pair->hashCode = inHashCode;
	}
	else {
		pair = allocator->mallocObject<Pair>().ok();
		new (&pair->key) Key(std::move(inKey));
		new (&pair->value) Value(std::move(inValue));
		pair->hashCode = inHashCode;
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::HashMapGroup(HashMapGroup&& other) noexcept
{
	check_eq(pairs, nullptr);

	hashMasks = other.hashMasks;
	pairs = other.pairs;
	pairCount = other.pairCount;
	capacity = other.capacity;
	other.hashMasks = nullptr;
	other.pairs = nullptr;
	other.pairCount = 0;
	other.capacity = 0;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>& gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::operator=(HashMapGroup&& other) noexcept
{
	check_eq(pairs, nullptr);

	hashMasks = other.hashMasks;
	pairs = other.pairs;
	pairCount = other.pairCount;
	capacity = other.capacity;
	other.hashMasks = nullptr;
	other.pairs = nullptr;
	other.pairCount = 0;
	other.capacity = 0;

	return *this;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::~HashMapGroup()
{
	//check_eq(pairs, nullptr);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr void gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::defaultInit(AllocatorRef* allocator)
{
	check_eq(pairs, nullptr);

	if (std::is_constant_evaluated()) {
		hashMasks = new i8[GROUP_ALLOC_SIZE];
		for (usize i = 0; i < GROUP_ALLOC_SIZE; i++) {
			hashMasks[i] = 0;
		}
		pairs = new PairT[GROUP_ALLOC_SIZE];
		capacity = GROUP_ALLOC_SIZE;
	}
	else {
		constexpr usize INITIAL_ALLOCATION_SIZE = calculateHashMapGroupRuntimeAllocationSize<PairT>(GROUP_ALLOC_SIZE);

		i8* memory = allocator->mallocAlignedBuffer<i8>(INITIAL_ALLOCATION_SIZE, ALLOC_ALIGNMENT).ok();
		memset(memory, 0, INITIAL_ALLOCATION_SIZE);

		hashMasks = memory;
		pairs = reinterpret_cast<PairT*>(memory + GROUP_ALLOC_SIZE);
		capacity = GROUP_ALLOC_SIZE;
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr void gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::free(AllocatorRef* allocator)
{
	if (std::is_constant_evaluated()) {
		for (usize i = 0; i < capacity; i++) {
			if (hashMasks[i] == 0) continue;
			pairs[i].erase(nullptr);
		}
		delete[] hashMasks;
		delete[] pairs;
		pairs = nullptr;
		hashMasks = nullptr;
		return;
	}

	for (usize i = 0; i < capacity; i++) {
		if (hashMasks[i] != 0) {
			pairs[i].erase(allocator);
		}
	}

	const usize currentAllocationSize = calculateHashMapGroupRuntimeAllocationSize<PairT>(capacity);
	allocator->freeAlignedBuffer<i8>(hashMasks, currentAllocationSize, ALLOC_ALIGNMENT);
	pairs = nullptr;
	check_eq(hashMasks, nullptr);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::Option<Value*> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::find(const Key& key, usize hashCode)
{
	Option<usize> index = findIndexOfKey(key, hashCode);
	if (index.none()) {
		return Option<Value*>();
	}
	return Option<Value*>((Value*)pairs[index.someCopy()].getValue());
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::Option<const Value*> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::find(const Key& key, usize hashCode) const
{
	Option<usize> index = findIndexOfKey(key, hashCode);
	if (index.none()) {
		return Option<const Value*>();
	}
	return Option<const Value*>((const Value*)pairs[index.someCopy()].getValue());
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::Option<Value*> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::insert(Key&& key, Value&& value, usize hashCode, AllocatorRef* allocator)
{
	Option<Value*> existingValue = /*((gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>*)this)->*/
		find(key, hashCode);

	if (existingValue.isSome()) {
		return existingValue;
	}

	if (pairCount == capacity) {
		reallocate(capacity * 2, allocator);
	}

	usize availableIndex = firstAvailableGroupSlot().some();
	pairs[availableIndex].insert(std::move(key), std::move(value), hashCode, allocator);
	hashMasks[availableIndex] = internal::HashMapPairBitmask(hashCode).value;
	pairCount++;
	return Option<Value*>();
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr bool gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::erase(const Key& key, usize hashCode, AllocatorRef* allocator)
{
	Option<usize> foundIndex = findIndexOfKey(key, hashCode);
	if (foundIndex.none()) {
		return false;
	}

	const usize index = foundIndex.someCopy();
	hashMasks[index] = 0;
	pairs[index].erase(allocator);
	pairCount--;
	return true;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::Option<gk::usize> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::findIndexOfKey(const Key& key, usize hashCode) const
{
	if (!std::is_constant_evaluated()) {
		return findIndexOfKeyRuntime(key, hashCode);
	}

	const internal::HashMapPairBitmask hashBitmask = hashCode;
	for (usize i = 0; i < capacity; i++) {
		if (hashMasks[i] == hashBitmask.value) {
			if (*pairs[i].getKey() == key) {
				return Option<usize>(i);
			}
		}
	}
	return Option<usize>();
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline gk::Option<gk::usize> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::findIndexOfKeyRuntime(const Key& key, usize hashCode) const
{
	if constexpr (GROUP_ALLOC_SIZE == 16) {
		return findIndexOfKeyRuntime16(key, hashCode);
	}
	else if constexpr (GROUP_ALLOC_SIZE == 32) {
		return findIndexOfKeyRuntime32(key, hashCode);
	}
	else {
		return findIndexOfKeyRuntime64(key, hashCode);
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline gk::Option<gk::usize> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::findIndexOfKeyRuntime16(const Key& key, usize hashCode) const
{
	const usize iterationCount = capacity / 16;

	for (usize i = 0; i < iterationCount; i++) {
		u64 bitmask = simdFindEqualHashCodeBitmaskIteration16(hashMasks + (i * 16), HashMapPairBitmask(hashCode));
		while (true) {
			Option<usize> indexOption = bitscanForwardNext(&bitmask);
			if (indexOption.none()) break;

			const usize index = indexOption.someCopy() + (i * 16);

			if (*pairs[index].getKey() == key) {
				return Option<usize>(index);
			}
		}
	}
	return Option<usize>();
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline gk::Option<gk::usize> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::findIndexOfKeyRuntime32(const Key& key, usize hashCode) const
{
	const usize iterationCount = capacity / 32;

	for (usize i = 0; i < iterationCount; i++) {
		u64 bitmask = simdFindEqualHashCodeBitmaskIteration32(hashMasks + (i * 32), HashMapPairBitmask(hashCode));
		while (true) {
			Option<usize> indexOption = bitscanForwardNext(&bitmask);
			if (indexOption.none()) break;

			const usize index = indexOption.someCopy() + (i * 32);

			if (*pairs[index].getKey() == key) {
				return Option<usize>(index);
			}
		}
	}
	return Option<usize>();
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline gk::Option<gk::usize> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::findIndexOfKeyRuntime64(const Key& key, usize hashCode) const
{
	const usize iterationCount = capacity / 64;

	for (usize i = 0; i < iterationCount; i++) {
		u64 bitmask = simdFindEqualHashCodeBitmaskIteration64(hashMasks + (i * 64), HashMapPairBitmask(hashCode));
		while (true) {
			Option<usize> indexOption = bitscanForwardNext(&bitmask);
			if (indexOption.none()) break;

			const usize index = indexOption.someCopy() + (i * 64);

			if (*pairs[index].getKey() == key) {
				return Option<usize>(index);
			}
		}
	}
	return Option<usize>();
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr gk::Option<gk::usize> gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::firstAvailableGroupSlot() const
{
	if (std::is_constant_evaluated()) {
		for (usize i = 0; i < capacity; i++) {
			if (hashMasks[i] == 0) {
				return Option<usize>(i);
			}
		}
		return Option<usize>();
	}
	else {
		if constexpr (GROUP_ALLOC_SIZE == 16) {
			return firstAvailableGroupSlot16(hashMasks, capacity);
		}
		else if constexpr (GROUP_ALLOC_SIZE == 32) {
			return firstAvailableGroupSlot32(hashMasks, capacity);
		}
		else {
			return firstAvailableGroupSlot64(hashMasks, capacity);
		}
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
inline constexpr void gk::internal::HashMapGroup<Key, Value, GROUP_ALLOC_SIZE>::reallocate(usize newCapacity, AllocatorRef* allocator)
{
	check_eq((newCapacity % 16), 0);
	if (newCapacity < capacity) {
		return;
	}

	//this->free(allocator);
	//check_eq(pairs, nullptr);

	if (std::is_constant_evaluated()) {
		i8* newHashMasks = new i8[newCapacity];
		for (usize i = 0; i < newCapacity; i++) {
			newHashMasks[i] = 0;
		}
		PairT* newPairs = new PairT[newCapacity];

		usize newIter = 0;
		for (usize i = 0; i < capacity; i++) {
			if (hashMasks[i] == 0) {
				continue;
			}
			newHashMasks[newIter] = hashMasks[i];
			newPairs[newIter] = std::move(pairs[i]);
			newIter++;
		}
		if (pairs != nullptr) {
			delete[] pairs;
			check_ne(hashMasks, nullptr);
			delete[] hashMasks;
		}

		hashMasks = newHashMasks;
		pairs = newPairs;
		capacity = newCapacity;
	}
	else {
		const usize allocationSize = calculateHashMapGroupRuntimeAllocationSize<PairT>(newCapacity);

		i8* memory = allocator->mallocAlignedBuffer<i8>(allocationSize, ALLOC_ALIGNMENT).ok();
		memset(memory, 0, allocationSize);

		i8* newHashMasks = memory;
		PairT* newPairs = reinterpret_cast<PairT*>(memory + newCapacity);
		usize newIter = 0;
		for (usize i = 0; i < capacity; i++) {
			if (hashMasks[i] == 0) {
				continue;
			}
			newHashMasks[newIter] = hashMasks[i];
			newPairs[newIter] = std::move(pairs[i]);
			newIter++;
		}

		const usize currentAllocationSize = calculateHashMapGroupRuntimeAllocationSize<PairT>(capacity);
		allocator->freeAlignedBuffer<i8>(hashMasks, currentAllocationSize, ALLOC_ALIGNMENT);
		pairs = nullptr;
		check_eq(hashMasks, nullptr);

		hashMasks = newHashMasks;
		pairs = newPairs;
		capacity = newCapacity;
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::usize gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::hashKey(const Key& key)
{
	if constexpr (!std::is_pointer<Key>::value) {
		return gk::hash<Key>(key);
	}
	else {
		if (std::is_constant_evaluated()) {
			throw std::invalid_argument("Cannot use pointer type for HashMap Key in constexpr contexts");
		}
		const usize ptrAsNum = reinterpret_cast<usize>(key);
		return ptrAsNum;
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::HashMap()
	: _groups(nullptr), _groupCount(0), _elementCount(0)
{
	if (!std::is_constant_evaluated()) {
		new (&_allocator) AllocatorRef(globalHeapAllocatorRef());
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::HashMap(const HashMap& other)
	: _groups(nullptr), _groupCount(0), _elementCount(0)
{
	if (!std::is_constant_evaluated()) {
		new (&_allocator) AllocatorRef(globalHeapAllocatorRef());
	}
	if (other._groupCount == 0) {
		return;
	}

	reallocate(other._elementCount);

	for (usize i = 0; i < other._groupCount; i++) {
		const GroupT& otherGroup = other._groups[i];

		for (usize groupPairIter = 0; groupPairIter < otherGroup.capacity; groupPairIter++) {
			if (otherGroup.hashMasks[groupPairIter] == 0) {
				continue;
			}

			typename GroupT::PairT& pair = otherGroup.pairs[groupPairIter];

			const usize hashCode = pair.hashCode();
			const Key* key = pair.getKey();
			const Value* value = pair.getValue();

			const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);
			const usize groupIndex = groupBitmask.value % _groupCount;
			GroupT& group = _groups[groupIndex];
			group.insert(Key(*key), Value(*value), hashCode, &_allocator);
			_elementCount++;
		}
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::HashMap(HashMap&& other) noexcept
	: _groups(other._groups), _groupCount(other._groupCount), _elementCount(other._elementCount), _allocator(std::move(other._allocator))
{
	other._groups = nullptr;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::~HashMap()
{
	if (_groups == nullptr) return;

	for (usize i = 0; i < _groupCount; i++) {
		_groups[i].free(&_allocator);
	}
	if (std::is_constant_evaluated()) {
		delete[] _groups;
	}
	else {
		_allocator.freeBuffer<GroupT>(_groups, _groupCount);
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>& gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::operator=(const HashMap& other)
{
	if (_groups != nullptr) {
		for (usize i = 0; i < _groupCount; i++) {
			GroupT& group = _groups[i];
			for (usize pairIter = 0; pairIter < group.capacity; pairIter++) {
				if (group.hashMasks[pairIter] != 0) {
					group.pairs[i].erase(&_allocator);
					group.hashMasks[i] = 0;
				}
			}
		}
		if (other._elementCount == 0) {
			for (usize i = 0; i < _groupCount; i++) {
				_groups[i].free(&_allocator);
			}
			if (std::is_constant_evaluated()) {
				delete[] _groups;
			}
			else {
				_allocator.freeBuffer<GroupT>(_groups, _groupCount);
			}
			return *this;
		}
	}
	_elementCount = 0;

	if (shouldReallocate(other._elementCount)) {
		reallocate(other._elementCount);
	}

	for (usize i = 0; i < other._groupCount; i++) {
		const GroupT& otherGroup = other._groups[i];

		for (usize groupPairIter = 0; groupPairIter < otherGroup.capacity; groupPairIter++) {
			if (otherGroup.hashMasks[groupPairIter] == 0) {
				continue;
			}

			typename GroupT::PairT& pair = otherGroup.pairs[groupPairIter];

			const usize hashCode = pair.hashCode();
			const Key* key = pair.getKey();
			const Value* value = pair.getValue();

			const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);
			const usize groupIndex = groupBitmask.value % _groupCount;
			GroupT& group = _groups[groupIndex];
			group.insert(Key(*key), Value(*value), hashCode, &_allocator);
			_elementCount++;
		}
	}
	return *this;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>& gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::operator=(HashMap&& other) noexcept
{
	if (_groups != nullptr) {
		for (usize i = 0; i < _groupCount; i++) {
			_groups[i].free(&_allocator);
		}
		if (std::is_constant_evaluated()) {
			delete[] _groups;
		}
		else {
			_allocator.freeBuffer<GroupT>(_groups, _groupCount);
		}
	}

	_groups = other._groups;
	_groupCount = other._groupCount;
	_elementCount = other._elementCount;
	_allocator = std::move(other._allocator);
	other._groups = nullptr;
	return *this;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::Option<Value*> gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::find(const Key& key)
{
	if (_elementCount == 0) {
		return Option<Value*>();
	}

	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);
	const usize groupIndex = groupBitmask.value % _groupCount;

	GroupT& group = _groups[groupIndex];
	return group.find(key, hashCode);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::Option<const Value*> gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::find(const Key& key) const
{
	if (_elementCount == 0) {
		return Option<const Value*>();
	}

	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);
	const usize groupIndex = groupBitmask.value % _groupCount;

	const GroupT& group = _groups[groupIndex];
	return group.find(key, hashCode);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::Option<Value*> gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::insert(Key&& key, Value&& value)
{
	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);

	if (_elementCount != 0) {
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		Option<Value*> foundValue = group.insert(std::move(key), std::move(value), hashCode, &_allocator);
		if (foundValue.isSome()) {
			return foundValue; // already exists
		}
		else {
			_elementCount++;
			return Option<Value*>();
		}
	}

	if (shouldReallocate(_elementCount + 1)) {
		reallocate(_elementCount + 1); // reallocate handles allocating extra space
	}

	{
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		group.insert(std::move(key), std::move(value), hashCode, &_allocator);
		_elementCount++;
		return Option<Value*>();
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::Option<Value*> gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::insert(const Key& key, Value&& value)
{
	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);

	if (_elementCount != 0) {
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		Option<Value*> foundValue = group.insert(Key(key), std::move(value), hashCode, &_allocator);
		if (foundValue.isSome()) {
			return foundValue; // already exists
		}
		else {
			_elementCount++;
			return Option<Value*>();
		}
	}

	if (shouldReallocate(_elementCount + 1)) {
		reallocate(_elementCount + 1); // reallocate handles allocating extra space
	}

	{
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		group.insert(Key(key), std::move(value), hashCode, &_allocator);
		_elementCount++;
		return Option<Value*>();
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::Option<Value*> gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::insert(Key&& key, const Value& value)
{
	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);

	if (_elementCount != 0) {
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		Option<Value*> foundValue = group.insert(std::move(key), Value(value), hashCode, &_allocator);
		if (foundValue.isSome()) {
			return foundValue; // already exists
		}
		else {
			_elementCount++;
			return Option<Value*>();
		}
	}

	if (shouldReallocate(_elementCount + 1)) {
		reallocate(_elementCount + 1); // reallocate handles allocating extra space
	}

	{
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		group.insert(std::move(key), Value(value), hashCode, &_allocator);
		_elementCount++;
		return Option<Value*>();
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::Option<Value*> gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::insert(const Key& key, const Value& value)
{
	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);

	if (_elementCount != 0) {
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		Option<Value*> foundValue = group.insert(Key(key), Value(value), hashCode, &_allocator);
		if (foundValue.isSome()) {
			return foundValue; // already exists
		}
		else {
			_elementCount++;
			return Option<Value*>();
		}
	}

	if (shouldReallocate(_elementCount + 1)) {
		reallocate(_elementCount + 1); // reallocate handles allocating extra space
	}

	{
		const usize groupIndex = groupBitmask.value % _groupCount;
		GroupT& group = _groups[groupIndex];
		group.insert(Key(key), Value(value), hashCode, &_allocator);
		_elementCount++;
		return Option<Value*>();
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr bool gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::erase(const Key& key)
{
	if (_elementCount == 0) {
		return false;
	}

	const usize hashCode = hashKey(key);
	const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);
	const usize groupIndex = groupBitmask.value % _groupCount;

	GroupT& group = _groups[groupIndex];
	_elementCount--;
	return group.erase(key, hashCode, &_allocator);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr void gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::reserve(usize additional)
{
	const usize requiredCapacity = _elementCount + additional;
	if (shouldReallocate(requiredCapacity)) {
		reallocate(requiredCapacity);
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::begin()
{
	return Iterator::iterBegin(this);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::end()
{
	return Iterator::iterEnd(this);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::begin() const
{
	return ConstIterator::iterBegin(this);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::end() const
{
	return ConstIterator::iterEnd(this);
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::usize gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::calculateNewGroupCount(usize requiredCapacity)
{
	if (requiredCapacity <= GROUP_ALLOC_SIZE) {
		return 1;
	}
	else {
		const usize out = gk::upperPowerOfTwo(requiredCapacity / (GROUP_ALLOC_SIZE / 8));
		check_gt(out, 1);
		return out;
	}
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr bool gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::shouldReallocate(usize requiredCapacity) const
{
	if (_groupCount == 0) {
		return true;
	}
	const usize loadFactorScaledPairCount = (_elementCount >> 2) * 3; // multiply by 0.75
	return requiredCapacity > loadFactorScaledPairCount;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr void gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::reallocate(usize requiredCapacity)
{
	const usize newGroupCount = calculateNewGroupCount(requiredCapacity);
	if (newGroupCount <= _groupCount) {
		return;
	}

	GroupT* newGroupCollection = [&]() {
		if (std::is_constant_evaluated()) {
			GroupT* newGroups = new GroupT[newGroupCount];
			for (usize i = 0; i < newGroupCount; i++) {
				newGroups[i].defaultInit(nullptr);
			}
			return newGroups;
		}
		else {
			GroupT* memory = _allocator.mallocBuffer<GroupT>(newGroupCount).ok();
			for (usize i = 0; i < newGroupCount; i++) {
				new (memory + i) GroupT();
				memory[i].defaultInit(&_allocator);
			}
			return memory;
		}
	}();

	// Loop through the old buckets, and move their data to the new buckets
	// For non-in-place stored pairs, their hash code is already stored.
	// For in-place stored pairs, their hash codes need to be recalculated, 
	// but it should be very cheap for those types
	for (usize oldGroupIndex = 0; oldGroupIndex < _groupCount; oldGroupIndex++) {
		GroupT& oldGroup = _groups[oldGroupIndex];
		for (usize i = 0; i < oldGroup.capacity; i++) {
			if (oldGroup.hashMasks[i] == 0) {
				continue;
			}

			typename GroupT::PairT& pair = oldGroup.pairs[i];

			const usize hashCode = pair.hashCode();
			const internal::HashMapGroupBitmask groupBitmask = internal::HashMapGroupBitmask(hashCode);
			const usize newGroupIndex = groupBitmask.value % newGroupCount;

			GroupT& newGroup = newGroupCollection[newGroupIndex];
			if (newGroup.pairCount == newGroup.capacity) {
				newGroup.reallocate(newGroup.capacity * 2, &_allocator);
			}
			check_lt(newGroup.pairCount, newGroup.capacity);

			newGroup.hashMasks[newGroup.pairCount] = oldGroup.hashMasks[i];
			if (std::is_constant_evaluated()) {
				newGroup.pairs[newGroup.pairCount] = std::move(pair);
			}
			else {
				new (newGroup.pairs + newGroup.pairCount) typename GroupT::PairT(std::move(pair));
			}

			newGroup.pairCount++;
			oldGroup.hashMasks[i] = 0;
		}
		oldGroup.free(&_allocator);
	}
	if (_groups) {
		if (std::is_constant_evaluated()) {
			delete[] _groups;
		}
		else {
			_allocator.freeBuffer<GroupT>(_groups, _groupCount);
		}
	}

	_groups = newGroupCollection;
	_groupCount = newGroupCount;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator::iterBegin(HashMap* map)
{
	Iterator iter;
	iter._map = map;
	iter._currentGroup = map->_groups;
	if (iter._currentGroup != nullptr) {
		if (iter._currentGroup->hashMasks[0] == 0) { // step forward if slot doesnt have an entry
			iter.operator++();
		}
	}
	return iter;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator::iterEnd(HashMap* map)
{
	Iterator iter;
	iter._map = map;
	iter._currentGroup = map->_groups + map->_groupCount;
	return iter;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr bool gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator::operator==(const Iterator& other) const
{
	return _currentGroup == other._currentGroup && _currentElementIndex == other._currentElementIndex;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator::Pair gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator::operator*() const
{
	auto& pair = _currentGroup->pairs[_currentElementIndex];
	Pair out = {
		.key = *pair.getKey(), .value = *pair.getValue()
	};
	return out;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator& gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::Iterator::operator++()
{
	while (true) {
		_currentElementIndex++;
		if (_currentElementIndex == _currentGroup->capacity) {
			_currentElementIndex = 0;
			_currentGroup++;
		}

		if (_currentGroup == _map->_groups + _map->_groupCount) {
			return *this;
		}

		if (_currentGroup->hashMasks[_currentElementIndex] != 0) {
			return *this;
		}
	}
}



template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator::iterBegin(const HashMap* map)
{
	ConstIterator iter;
	iter._map = map;
	iter._currentGroup = map->_groups;
	if (iter._currentGroup != nullptr) {
		if (iter._currentGroup->hashMasks[0] == 0) { // step forward if slot doesnt have an entry
			iter.operator++();
		}
	}
	return iter;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator::iterEnd(const HashMap* map)
{
	ConstIterator iter;
	iter._map = map;
	iter._currentGroup = map->_groups + map->_groupCount;
	return iter;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr bool gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator::operator==(const ConstIterator& other) const
{
	return _currentGroup == other._currentGroup && _currentElementIndex == other._currentElementIndex;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator::Pair gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator::operator*() const
{
	const auto& pair = _currentGroup->pairs[_currentElementIndex];
	Pair out = {
		.key = *pair.getKey(), .value = *pair.getValue()
	};
	return out;
}

template<typename Key, typename Value, gk::usize GROUP_ALLOC_SIZE>
	requires (GROUP_ALLOC_SIZE % 16 == 0 && gk::Hashable<Key>)
inline constexpr gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator& gk::HashMap<Key, Value, GROUP_ALLOC_SIZE>::ConstIterator::operator++()
{
	while (true) {
		_currentElementIndex++;
		if (_currentElementIndex == _currentGroup->capacity) {
			_currentElementIndex = 0;
			_currentGroup++;
		}

		if (_currentGroup == _map->_groups + _map->_groupCount) {
			return *this;
		}

		if (_currentGroup->hashMasks[_currentElementIndex] != 0) {
			return *this;
		}
	}
}
