#pragma once

#include "basic_types.h"
#include "option/option.h"

namespace gk
{
	[[nodiscard]] constexpr u64 upperPowerOfTwo(u64 v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v |= v >> 32;
		v++;
		return v;
	}

	Option<usize> bitscanForwardNext(u64* bitmask);
}