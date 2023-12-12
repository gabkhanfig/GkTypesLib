#pragma once
#include "../basic_types.h"
#include "../doctest/doctest_proxy.h"
#include "../option/option.h"
#include "utf8.h"
#include <xtr1common>
#include <string>
// TODO dont use <string>, find another way for std::char_traits

namespace gk
{
  /**
  * A utf8 string slice. Is ensured to be valid utf8.
  * Is not guaranteed to have a null terminator at buffer[len].
  */
  struct Str {
    /**
    */
    const char* buffer;

    /**
    * Number of char bytes representing the string slice. Is NOT the same as utf8 length. 
    * Does not include null terminator.
    */
    usize len;

    constexpr Str() : buffer(nullptr), len(0) {}

    constexpr Str(const Str&) = default;
    constexpr Str(Str&&) = default;
    constexpr Str& operator = (const Str&) = default;
    constexpr Str& operator = (Str&&) = default;

    template<usize N>
    constexpr Str(char const (&inStr)[N]) {
      buffer = inStr;
      len = N - 1;
    }

    /**
    * Creates a string slice from a pointer that is null terminated.
    * Checks that it's valid UTF-8.
    * 
    * @param inStr: Start of a non-null, utf8 valid, null terminated string.
    */
    static constexpr Str fromNullTerminated(const char* inStr) {
      check_ne(inStr, nullptr);
      Str str;
      str.buffer = inStr;
      str.len = std::char_traits<char>::length(inStr);
      return str;
    }

    /**
    * Creates a string slice from a pointer and a length.
    * Assumes that `start[0]` to `start[length - 1]` is valid
    * Checks that it's valid UTF-8.
    * 
    * @param start: Beginning of a UTF-8 slice.
    * @param length: Number of bytes of the slice (not including any potential null terminators).
    */
    static constexpr Str fromSlice(const char* start, usize length) {
      check_ne(start, nullptr);
      Str str;
      str.buffer = start;
      str.len = length;
      return str;
    }

    
    constexpr gk::Str substring(usize startIndexInclusive, usize endIndexExclusive) const {
      gk::Str sub;
      sub.buffer = buffer + startIndexInclusive;
      sub.len = endIndexExclusive - startIndexInclusive;
      return sub;
    }

    friend std::ostream& operator << (std::ostream& os, const Str& inStr) {
      return os.write(inStr.buffer, inStr.len);
    }
  };
}

consteval gk::Str operator "" _str(const char* inStr, size_t length) {
  gk::Str str;
  str.buffer = inStr;
  str.len = length;
  return str;
}