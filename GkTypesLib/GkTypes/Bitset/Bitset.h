#pragma once

#include <type_traits>
#include "../BasicTypes.h"

namespace gk
{
	/* A more size compressed version of std::bitset.
	@param BitCount: Amount of bits to store. Must be between 1-64 inclusive. */
	template<uint8 BitCount>
	struct bitset
	{
		static_assert(BitCount > 0, "bit count must be greater than 0");
		static_assert(BitCount < 65, "bit count must be less than 65");

		typedef std::conditional_t <BitCount <= 8, unsigned char,
			std::conditional_t <BitCount <= 16, unsigned short,
			std::conditional_t<BitCount <= 32, unsigned int,
			unsigned long long >>> Bittype;

		constexpr static uint8 GetBitCount() { return BitCount; }

		Bittype bits;

		constexpr bitset() {
			bits = 0;
		}

		constexpr bitset(const Bittype initialFlags) {
			bits = initialFlags;
		}

		template<uint8 N>
		constexpr bitset(const bitset<N> other) {
			bits = other.bits;
		}

		constexpr bool GetBit(uint8 index) {
			return bits >> index & 1U;
		}

		constexpr bool operator [] (uint8 index) {
			return GetBit(index);
		}

		constexpr void SetBit(uint8 index, bool flag = true) {
			bits ^= (-(static_cast<uint8>(flag)) ^ bits) & 1ULL << index;
		}

		template<uint8 N>
		constexpr void operator = (const bitset<N> other) {
			bits = other.bits;
		}

		constexpr void operator = (const Bittype other) {
			bits = other;
		}

		template<uint8 N>
		constexpr bool operator == (const bitset<N> other) {
			return (uint64)bits == (uint64)other.bits;
		}

		constexpr bool operator == (uint64 other) {
			return (uint64)bits == other;
		}

		template<uint8 N>
		constexpr Bittype operator & (const bitset<N> other) {
			return bits & other.bits;
		}

		constexpr Bittype operator & (Bittype other) {
			return bits & other;
		}

		template<uint8 N>
		constexpr Bittype operator &= (const bitset<N> other) {
			bits &= other.bits;
			return bits;
		}

		constexpr Bittype operator &= (Bittype other) {
			bits &= other;
			return bits;
		}

		template<uint8 N>
		constexpr Bittype operator | (const bitset<N> other) {
			return bits | other.bits;
		}

		constexpr Bittype operator | (Bittype other) {
			return bits | other;
		}

		template<uint8 N>
		constexpr Bittype operator |= (const bitset<N> other) {
			bits |= other.bits;
			return bits;
		}

		constexpr Bittype operator |= (Bittype other) {
			bits |= other;
			return bits;
		}

		template<uint8 N>
		constexpr Bittype operator ^ (const bitset<N> other) {
			return bits ^ other.bits;
		}

		constexpr Bittype operator ^ (Bittype other) {
			return bits ^ other;
		}

		template<uint8 N>
		constexpr Bittype operator ^= (const bitset<N> other) {
			bits ^= other.bits;
			return bits;
		}

		constexpr Bittype operator ^= (Bittype other) {
			bits ^= other;
			return bits;
		}

		constexpr Bittype operator << (const uint8 shift) {
			return bits << shift;
		}

		constexpr Bittype operator <<= (const uint8 shift) {
			bits <<= shift;
			return bits;
		}

		constexpr Bittype operator >> (const uint8 shift) {
			return bits >> shift;
		}

		constexpr Bittype operator >>= (const uint8 shift) {
			bits >>= shift;
			return bits;
		}

		constexpr Bittype operator ~ () {
			return ~bits;
		}

	};
}