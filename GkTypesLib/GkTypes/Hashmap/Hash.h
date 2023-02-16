#pragma once

#include <type_traits>

namespace gk 
{
	template<typename KeyType>
	constexpr size_t hash(KeyType key) = delete;

	template<>
	constexpr size_t hash<size_t>(size_t key) {
		return key;
	}

	template<>
	constexpr size_t hash<int>(int key) {
		return size_t(key);
	}

}