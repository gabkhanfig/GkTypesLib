# Gk Types Library
### CMake library of Useful Containers and Systems for Video Games

Library of C++ data types for games on Windows. Requires 64 bit processors, and AVX-2 support minimum.

Examples of type usage can be seen from the corresponding .cpp files. For any types that use SIMD, 
512 bit or 256 bit extensions usage is automatically determined at runtime given the CPU's featureset.

Uses [doctest](https://github.com/doctest/doctest) for unit testing, and modified doctest checks for asserts.
Runtime asserts are disabled in Release.

<h3>Currently Added Containers and Systems:</h3>

- [Allocators](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/allocator/allocator.h)
- [Array List](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/array/array_list.h)
- [String](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/string/string.h)
- [Str](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/string/str.h)
- [Global String](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/string/global_string.h)
- [Hash Map](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/hash/hashmap.h)
- [Option](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/option/option.h)
- [Result](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/error/result.h)
- [Fptr](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/function/function_ptr.h)
- [Callback](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/function/callback.h)
- [Mutex](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/sync/mutex.h)
- [RwLock](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/sync/rw_lock.h)
- [Job Future](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/job/job_future.h)
- [Job Thread](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/job/job_thread.h)
- [Job System](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/job/job_system.h)
- [Ring Queue](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/queue/ring_queue.h)

<h2>

[Allocators](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/allocator/allocator.h)

</h2>

Custom allocator objects for highly controlled memory allocation strategies.

<h2>

[Array List](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/array/array_list.h)

</h2>

A constexpr replacement to std::vector, supporting custom runtime allocators, and SIMD element finding.

<h2>

[String](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/string/string.h)

</h2>

A utf8 constexpr replacement to std::string, supporting an in-place [**SSO**](https://blogs.msmvps.com/gdicanio/2016/11/17/the-small-string-optimization/) buffer
of 31 bytes. Uses SIMD for extremely fast comparisons and finding.

<h2>

[Str](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/string/str.h)

</h2>

Utf8 compatible string slice, supporting compile time utf8 parsing optimizations.

<h2>

[Global String](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/string/global_string.h)

</h2>

Thread safe string compression into 4 byte integer unique ids, that last for the entire program duration.

<h2>

[Hash Map](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/hash/hashmap.h)

</h2>

A replacement to std::unordered_map that's vastly more optimized, using better caching strategies, SIMD hash finding, and custom allocator support.
It's inspired by this [talk](https://youtube.com/watch?v=ncHmEUmJZf4&), and extended further.

<h2>

[Option](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/option/option.h)

</h2>

Rust's [option](https://doc.rust-lang.org/std/option/enum.Option.html) type implemented in c++.

<h2>

[Result](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/error/result.h)

</h2>

Rust's [result](https://doc.rust-lang.org/std/result/#:~:text=Module%20std%3A%3Aresult&text=Error%20handling%20with%20the%20Result,and%20containing%20an%20error%20value.) type implenented in c++.

<h2>

[Fptr](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/function/function_ptr.h)

</h2>

Simple way to express function pointers for free functions.

<h2>

[Callback](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/function/callback.h)

</h2>

Allow binding specific objects with corresponding member functions, or free functions, to be called with arguments any amount of times later.

<h2>

[Mutex](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/sync/mutex.h)

</h2>

High performance rust style [mutex](https://doc.rust-lang.org/std/sync/struct.Mutex.html) designed
to stop developers forgetting to unlock their mutexes.

<h2>

[RwLock](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/sync/rw_lock.h)

</h2>

High performance rust style [rwlock](https://doc.rust-lang.org/std/sync/struct.RwLock.html) designed
to stop developers forgetting to unlock their rwlocks.

<h2>

[Job Future](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/job/job_future.h)

</h2>

Future to wait on asynchronous jobs created from either job threads, or job systems. See below.

<h2>

[Job Thread](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/job/job_thread.h)

</h2>

Thread wrapper to handle running asynchronous jobs, optimized to allocate as little memory as possible.
It's fully thread safe.

<h2>

[Job System](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/job/job_system.h)

</h2>

Dispatches jobs across threads, doing automatic load balancing.
It's fully thread safe.

<h2>

[Ring Queue](https://github.com/gabkhanfig/GkTypesLib/blob/master/gk_types_lib/queue/ring_queue.h)

</h2>

A fixed size [circular buffer](https://en.wikipedia.org/wiki/Circular_buffer) that does not do extra memory allocation. 
