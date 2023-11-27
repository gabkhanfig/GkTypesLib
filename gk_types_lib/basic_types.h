#pragma once

namespace gk
{
	// Signed 8 bit integer
	using i8 = char;
	// Signed 16 bit integer
	using i16 = short;
	// Signed 32 bit integer
	using i32 = int;
	// Signed 64 bit integer
	using i64 = long long;

	// Unsigned 8 bit integer
	using u8 = unsigned char;
	// Unsigned 16 bit integer
	using u16 = unsigned short;
	// Unsigned 32 bit integer
	using u32 = unsigned int;
	// Unsigned 64 bit integer
	using u64 = unsigned long long;

	using usize = size_t;
}

static_assert(sizeof(gk::i8) == 1);
static_assert(sizeof(gk::i16) == 2);
static_assert(sizeof(gk::i32) == 4);
static_assert(sizeof(gk::i64) == 8);

static_assert(sizeof(gk::u8) == 1);
static_assert(sizeof(gk::u16) == 2);
static_assert(sizeof(gk::u32) == 4);
static_assert(sizeof(gk::u64) == 8);

#ifdef _MSC_VER

/* Aligns data types on windows. */
#define ALIGN_AS(alignment) __declspec(align(alignment))
#define forceinline __forceinline

#else

/* Aligns data types on linux / mac. */
#define ALIGN_AS alignas(alignment)
#define forceinline __attribute__((always_inline))

#endif