#pragma once

#include <type_traits>
#include <concepts>

namespace gk 
{
	template<typename KeyType>
	constexpr size_t hash(const KeyType& key) = delete;

	template<>
	constexpr size_t hash<bool>(const bool& key) { // Why would you ever use this??
		return size_t(key);
	}

	template<>
	constexpr size_t hash<int8>(const int8& key) {
		return size_t(key) << 13;
	}

	template<>
	constexpr size_t hash<uint8>(const uint8& key) {
		return size_t(key) << 13;
	}
	

	template<>
	constexpr size_t hash<int16>(const int16& key) {
		return size_t(key) << 13;
	}

	template<>
	constexpr size_t hash<uint16>(const uint16& key) {
		return size_t(key) << 13;
	}

	template<>
	constexpr size_t hash<int>(const int& key) {
		return size_t(key) << 13;
	}

	template<>
	constexpr size_t hash<uint32>(const uint32& key) {
		return size_t(key) << 13;
	}

	template<>
	constexpr size_t hash<float>(const float& key) {
		const double asDouble = static_cast<double>(key);
		return std::bit_cast<size_t, double>(asDouble);
	}

	template<>
	constexpr size_t hash<double>(const double& key) {
		return std::bit_cast<size_t, double>(key);
	}

	template<>
	constexpr size_t hash<int64>(const int64& key) {
		size_t h = 0;
		h |= (key << 13); // Mix the bits so that every 8th number increment only occupies one "group" in gk::HashMap
		h |= (key >> 51);
		return h;
	}

	template<>
	constexpr size_t hash<uint64>(const uint64& key) {
		size_t h = 0;
		h |= (key << 13); // Mix the bits so that every 8th number increment only occupies one "group" in gk::HashMap
		h |= (key >> 51);
		return h;
	}

	template<typename T>
	concept CanBeHashed = requires(T a)
	{
		{ gk::hash<T>(a) } -> std::convertible_to<std::size_t>;
	};

	template<typename T>
	concept Hashable = CanBeHashed<T> && std::equality_comparable<T>;
}