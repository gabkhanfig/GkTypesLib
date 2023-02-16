#pragma once

#include <concepts>
#include "Hash.h"

template<typename T>
concept Hashable = requires(T a)
{
	{ gk::hash<T>(a) } -> std::convertible_to<std::size_t>;
};

namespace gk 
{
	/** Key is required to be hashable. To make it hashable, the gk::hash function must be specialized. Example:
	* template<>
	* constexpr size_t gk::hash<int>(int key) { return size_t(key); }
	* This allows the key type to be used in the hashmap.
	*/
	template<typename Key, typename Value>
	requires Hashable<Key>
	struct Hashmap
	{
		constexpr static size_t Hash(Key key) {
			return gk::hash<Key>(key);
		}


	};

}


