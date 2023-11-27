#pragma once

#include "../basic_types.h"

namespace gk
{
	namespace x86
	{
		/**
		* Check at runtime if AVX-512 is supported by this x86 processor.
		* 
		* @return If AVX-512 is supported
		*/
		[[nodiscard]] bool isAvx512Supported();

		/**
		* Check at runtime if AVX-2 is supported by this x86 processor.
		*
		* @return If AVX-2 is supported
		*/
		[[nodiscard]] bool isAvx2Supported();
	}

	/**
	* @return Number of logical cores (threads) on this system at runtime
	*/
	[[nodiscard]] u32 systemThreadCount();
}