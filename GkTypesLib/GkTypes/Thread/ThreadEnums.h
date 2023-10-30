#pragma once

namespace gk
{
	/* Controls thread safety access patterns. */
	enum class ThreadSafety {
		Safe,
		Unsafe
	};

	/* Controls how a lock will be acquired. */
	enum class AcquireLock {
		Yield,
		Spin
	};
}