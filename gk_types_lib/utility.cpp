#include "utility.h"

#pragma intrinsic(_BitScanForward64)

gk::Option<gk::usize> gk::bitscanForwardNext(u64* bitmask)
{
	unsigned long index;
	if (!_BitScanForward64(&index, *bitmask)) return gk::Option<gk::usize>();
	*bitmask = (*bitmask & ~(1ULL << index));
	return gk::Option<gk::usize>(index);
}
