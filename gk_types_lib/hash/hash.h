#pragma once

#include "../basic_types.h"
#include <type_traits>
#include <concepts>
#include <utility>
#include <string>

namespace gk 
{
	template<typename KeyType>
	constexpr size_t hash(const KeyType& key) = delete;

	template<>
	constexpr size_t hash<bool>(const bool& key) { // Why would you ever use this??
		return size_t(key);
	}

	template<>
	constexpr size_t hash<i8>(const i8& key) {
		return size_t(key) << 1;
	}

	template<>
	constexpr size_t hash<u8>(const u8& key) {
		return size_t(key) << 1;
	}
	

	template<>
	constexpr size_t hash<i16>(const i16& key) {
		return size_t(key) << 1;
	}

	template<>
	constexpr size_t hash<u16>(const u16& key) {
		return size_t(key) << 1;
	}

	template<>
	constexpr size_t hash<i32>(const i32& key) {
		return size_t(key) << 1;
	}

	template<>
	constexpr size_t hash<u32>(const u32& key) {
		return size_t(key) << 1;
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
	constexpr size_t hash<i64>(const i64& key) {
		size_t h = 0;
		h |= (key << 1); // Mix the bits so that every 8th number increment only occupies one "group" in gk::HashMap
		h |= (key >> 63);
		return h;
	}

	template<>
	constexpr size_t hash<u64>(const u64& key) {
		size_t h = 0;
		h |= (key << 1); // Mix the bits so that every 8th number increment only occupies one "group" in gk::HashMap
		h |= (key >> 63);
		return h;
	}

	namespace internal
	{
		template<typename T>
		concept CanBeHashed = requires(T a)
		{
			{ gk::hash<T>(a) } -> std::convertible_to<std::size_t>;
		};
	}

	/**
	* A type if considered `Hashable` under two conditions:
	* 1. Has an overload for gk::hash<> OR is a pointer type.
	* 2. Is equality comparible.
	*/
	template<typename T>
	concept Hashable = (internal::CanBeHashed<T> || std::is_pointer<T>::value) && std::equality_comparable<T>;
}