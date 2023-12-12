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
    constexpr Str(char const (&inStr)[N]) : buffer(inStr), len(N - 1) {}

    /**
    * Creates a string slice from a pointer that is null terminated.
    * Checks that it's valid UTF-8.
    * 
    * @param inStr: Start of a non-null, utf8 valid, null terminated string.
    */
    [[nodiscard]] static constexpr Str fromNullTerminated(const char* inStr);

    /**
    * Creates a string slice from a pointer and a length.
    * Assumes that `start[0]` to `start[length - 1]` is valid
    * Checks that it's valid UTF-8.
    * 
    * @param start: Beginning of a UTF-8 slice.
    * @param length: Number of bytes of the slice (not including any potential null terminators).
    */
    [[nodiscard]] static constexpr Str fromSlice(const char* start, usize length);

    [[nodiscard]] constexpr Option<usize> find(char c) const;

    [[nodiscard]] constexpr Option<usize> find(gk::Str str) const;

    [[nodiscard]] constexpr Option<usize> findLast(char c) const;

    [[nodiscard]] constexpr Option<usize> findLast(gk::Str str) const;

    [[nodiscard]] constexpr gk::Str substring(usize startIndexInclusive, usize endIndexExclusive) const;

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

inline constexpr gk::Str gk::Str::fromNullTerminated(const char* inStr)
{
  check_ne(inStr, nullptr);
  Str str;
  str.buffer = inStr;
  str.len = std::char_traits<char>::length(inStr); // TODO not use char traits
  return str;
}

inline constexpr gk::Str gk::Str::fromSlice(const char* start, usize length)
{
  check_ne(start, nullptr);
  Str str;
  str.buffer = start;
  str.len = length;
  return str;
}

inline constexpr gk::Option<gk::usize> gk::Str::find(char c) const
{
  for (usize i = 0; i < len; i++) {
    if (buffer[i] == c) return Option<usize>(i);
  }
  return Option<usize>();
}

inline constexpr gk::Option<gk::usize> gk::Str::find(gk::Str str) const
{
  if (str.len == 0) {
    return Option<usize>();
  }
  else if (str.len == 1) {
    return find(str.buffer[0]);
  }
  else if (str.len > len) {
    return Option<usize>();
  }
  //else if (str.len == len) {
  //  if (*this == str) {
  //    return Option<usize>(0);
  //  }
  //  return Option<usize>();
  //}

  if (true) { // TODO SIMD
    const char firstChar = str.buffer[0];
    for (u64 i = 0; i < len; i++) {
      if (buffer[i] == firstChar) {
        const char* thisCompareStart = buffer + i;

        bool found = true;
        for (u64 compareIndex = 1; compareIndex < str.len; compareIndex++) { // dont need to check the first character
          if (thisCompareStart[compareIndex] != str.buffer[compareIndex]) {
            found = false;
            break;
          }
        }
        if (found) {
          return Option<usize>(i); // All has been checked.
        }
      }
    }
    return Option<usize>();
  }
}

inline constexpr gk::Option<gk::usize> gk::Str::findLast(char c) const
{
  usize i = len - 1;
  while (true) {
    if (buffer[i] == c) return Option<usize>(i);
    if (i == 0) return Option<usize>();
    i--;
  }
}

inline constexpr gk::Option<gk::usize> gk::Str::findLast(gk::Str str) const
{
  if (str.len == 0) {
    return Option<usize>();
  }
  else if (str.len == 1) {
    return find(str.buffer[0]);
  }
  else if (str.len > len) {
    return Option<usize>();
  }
  //else if (str.len == len) {
  //  if (*this == str) {
  //    return Option<usize>(0);
  //  }
  //  return Option<usize>();
  //}
  if (true) { // TODO SIMD
    usize i = len - 1;
    const char firstChar = str.buffer[0];
    while (true) {
      if (buffer[i] == firstChar) {
        const char* thisCompareStart = buffer + i;

        bool found = true;
        for (usize compareIndex = 1; compareIndex < str.len; compareIndex++) { // dont need to check the first character
          if (thisCompareStart[compareIndex] != str.buffer[compareIndex]) {
            found = false;
            break;
          }
        }
        if (found) {
          return Option<usize>(i); // All has been checked.
        }
      }
      if (i == 0) return Option<usize>();
      i--;
    }
  }
}

inline constexpr gk::Str gk::Str::substring(usize startIndexInclusive, usize endIndexExclusive) const
{
  gk::Str sub;
  sub.buffer = buffer + startIndexInclusive;
  sub.len = endIndexExclusive - startIndexInclusive;
  return sub;
}
