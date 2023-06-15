#pragma once

#include <type_traits>
#include "../BasicTypes.h"
#include <array>

namespace gk
{
	/* A more size compressed version of std::bitset.
	@param BitCount: Amount of bits to store. Must be between 1-64 inclusive. */ //, typename = typename std::enable_if<(BitCount < 65 && BitCount > 0), void>::type
	template<int BitCount>
	struct bitset
	{
		static_assert(BitCount > 0, "bit count must be greater than 0");
		//static_assert(BitCount < 65, "bit count must be less than 65");

		typedef std::conditional_t <BitCount <= 8, unsigned char,
			std::conditional_t <BitCount <= 16, unsigned short,
			std::conditional_t<BitCount <= 32, unsigned int,
			std::conditional_t<BitCount <= 64, unsigned long long,
			unsigned long long >>>> Bittype;

		constexpr static int GetBitCount() { return BitCount; }

		constexpr static int GetBitArrayCount() { 
			if constexpr (BitCount <= 64) return 1;
			return ((BitCount - 1) / 64) + 1; 
		}

		union {
			Bittype bits;
			Bittype bitsArray[GetBitArrayCount()];
		};
		

		bitset() {
			if constexpr (BitCount < 65) {
				bits = 0;
			}
			else {
				for (int i = 0; i < GetBitArrayCount(); i++) {
					bitsArray[i] = 0;
				}
			}
		}

		bitset(const Bittype initialFlags) {
			bits = initialFlags;
		}
		
		template<size_t N>
		bitset(std::array<unsigned long long, N> initialFlagsArray) {
			for (int i = 0; i < GetBitArrayCount(); i++) {
				if (i < N) {
					bitsArray[i] = initialFlagsArray[i];
				}
				else {
					bitsArray[i] = 0;
				}
			}
		}

		template<int N>
		bitset(const bitset<N> other) {
			if constexpr (N < 65) {
				bits = other.bits;
			}
			else {
				constexpr int otherBitArrayCount = ((N - 1) / 64) + 1;
				for (int i = 0; i < GetBitArrayCount(); i++) {
					if (i < otherBitArrayCount) { // If the other bitset has bits in range of this bitset, copy them
						bitsArray[i] = other.bitsArray[i];
					}
					else {
						bitsArray[i] = 0;
					}
				}
			}
		}

		/* Uses array indexing rules. Index 0 = first element. */
		[[nodiscard]] bool GetBit(unsigned long long index) const {
			if constexpr (BitCount < 65) {
				return bits >> index & 1ULL;
			}
			else {
				const unsigned long long arrayIndex = index / 64;
				const int element = index % 64;
				return bitsArray[arrayIndex] >> element & 1ULL;
			}
		}

		/* Uses array indexing rules. Index 0 = first element. */
		[[nodiscard]] bool operator [] (unsigned long long index) const {
			return GetBit(index);
		}

		/* Uses array indexing rules. Index 0 = first element. */
		bitset& SetBit(unsigned long long index, bool flag = true) {
			if constexpr (BitCount < 65) {
				bits ^= (-(static_cast<long long>(flag)) ^ bits) & 1ULL << index;
			}
			else {
				const unsigned long long arrayIndex = index / 64;
				const int element = index % 64;
				bitsArray[arrayIndex] ^= (-(static_cast<long long>(flag)) ^ bitsArray[arrayIndex]) & 1ULL << element;
			}
			return *this;
		}

		template<int N>
		void operator = (const bitset<N> other) {
			bits = other.bits;
		}

		void operator = (const Bittype other) {
			bits = other;
		}

		template<size_t N>
		void operator = (std::array<unsigned long long, N> other) {
			for (int i = 0; i < GetBitArrayCount(); i++) {
				if (i < N) {
					bitsArray[i] = other[i];
				}
				else {
					bitsArray[i] = 0;
				}
			}
		}

		template<int N>
		[[nodiscard]] bool operator == (const bitset<N> other) const {
			return (uint64)bits == (uint64)other.bits;
		}

		[[nodiscard]] bool operator == (uint64 other) const {
			return (uint64)bits == other;
		}

		template<int N>
		[[nodiscard]] bitset operator & (const bitset<N> other) const {
			return bits & other.bits;
		}

		[[nodiscard]] bitset operator & (Bittype other) const {
			return bits & other;
		}

		template<int N>
		bitset& operator &= (const bitset<N> other) {
			bits &= other.bits;
			return *this;
		}

		bitset& operator &= (Bittype other) {
			bits &= other;
			return *this;
		}

		template<int N>
		[[nodiscard]] bitset operator | (const bitset<N> other) const {
			return bits | other.bits;
		}

		[[nodiscard]] bitset operator | (Bittype other) const {
			return bits | other;
		}

		template<int N>
		bitset& operator |= (const bitset<N> other) {
			bits |= other.bits;
			return *this;
		}

		bitset& operator |= (Bittype other) {
			bits |= other;
			return *this;
		}

		template<int N>
		[[nodiscard]] bitset operator ^ (const bitset<N> other) const {
			return bits ^ other.bits;
		}

		[[nodiscard]] bitset operator ^ (Bittype other) const {
			return bits ^ other;
		}

		template<int N>
		bitset& operator ^= (const bitset<N> other) {
			bits ^= other.bits;
			return *this;
		}

		bitset& operator ^= (Bittype other) {
			bits ^= other;
			return *this;
		}

		[[nodiscard]] bitset operator << (const uint8 shift) const {
			return bits << shift;
		}

		[[nodiscard]] Bittype operator <<= (const uint8 shift) {
			bits <<= shift;
			return *this;
		}

		bitset operator >> (const uint8 shift) const {
			return bits >> shift;
		}

		bitset& operator >>= (const uint8 shift) {
			bits >>= shift;
			return *this;
		}

		[[nodiscard]] bitset operator ~ () const {
			return ~bits;
		}

		/* Useful for sending bitset data to GPU */
		template<size_t bufferElementCount> 
		void CopyTo32BitBuffer(unsigned int* buffer) {
			if constexpr (BitCount <= 32) { // For a bitset of size 32 or less, just copy the bits directly. 
				buffer[0] = bits;
				return;
			}
			else if constexpr (BitCount <= 64) { // For a bitset of size between 33 and 64 inclusive...
				if constexpr (bufferElementCount > 1) { // If the buffer size is greater than one, copy 64 bits.
					memcpy(buffer, &bits, 8);
				}
				else { // Otherwise just copy 32 bits.
					memcpy(buffer, &bits, 4);
				}
			}
			else {
				if constexpr (bufferElementCount < GetBitArrayCount() * 2) {
					memcpy(buffer, bitsArray, bufferElementCount * 4);
				}
				else {
					memcpy(buffer, bitsArray, GetBitArrayCount() * 8);
				}
			}
		}

	};
}