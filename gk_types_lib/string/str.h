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

    /**
    * Equality comparison for if this string slice is equal to a single character.
    * 
    * @return If the slice is length 1, and is referencing `c`.
    */
    [[nodiscard]] constexpr bool operator == (char c) const;

    /**
    * Equality comparison for this string slice and another.
    * Is SIMD optimized.
    */
    [[nodiscard]] constexpr bool operator == (const Str& str) const;

    /**
    * Find the index of a char within this string slice.
    * If it exists, the Some variant will be returned with the index,
    * otherwise the None variant will be returned.
    * 
    * @param c: Character to find. Should not be null terminator.
    * @return The index of the character if it exists.
    */
    [[nodiscard]] constexpr Option<usize> find(char c) const;

    /**
    * Find the start index of another string slice within this one.
    * If it exists, the Some variant will be returned with the index where it begins,
    * otherwise the None variant will be returned.
    * 
    * @param str: String slice to find
    * @return The index of the beginning of the slice if it exists.
    */
    [[nodiscard]] constexpr Option<usize> find(gk::Str str) const;

    /**
    * Find the last index of a char within this string slice.
    * If it exists, the Some variant will be returned with the index,
    * otherwise the None variant will be returned.
    *
    * @param c: Character to find. Should not be null terminator.
    * @return The index of the character if it exists.
    */
    [[nodiscard]] constexpr Option<usize> findLast(char c) const;

    /**
    * Find the start index of the last occurrence of another string slice within this one.
    * If it exists, the Some variant will be returned with the index where it begins,
    * otherwise the None variant will be returned.
    *
    * @param str: String slice to find
    * @return The index of the beginning of the slice if it exists.
    */
    [[nodiscard]] constexpr Option<usize> findLast(gk::Str str) const;

    /**
    * Creates a substring from `startIndexInclusive` to `endIndexExclusive`.
    * Checks that the substring is valid UTF8.
    * Uses indices into the string slice, not UTF8 codepoints.
    * Doing `substring(start, start + 10)` will make a substring of `len() == 10`.
    * Does NOT make a copy, is just an offset into the slice.
    *
    * @param startIndexInclusive: Beginning of the substring. buffer[startIndexInclusive] is included.
    * @param endIndexInclusive: End of the substring. buffer[endIndexExclusive] is NOT included.
    * @return Valid UTF8 substring.
    */
    [[nodiscard]] constexpr gk::Str substring(usize startIndexInclusive, usize endIndexExclusive) const;

    /**
    * Parses a bool from the string slice.
    * If the string slice is "true", returns an Ok variant of `true`.
    * If the string slice is "false", returns an Ok variant of `false`.
    * All other values will be an Error variant.
    *
    * @return The parsed boolean, or an error.
    */
    [[nodiscard]] constexpr Result<bool> parseBool() const;

    /**
    * Parses a signed 64 bit integer from the string.
    * For example, the string of "-1234" returns an Ok variant of `-1234`.
    *
    * Errors:
    *
    * - Decimals will return an Error variant (eg. 12.5).
    * - Anything out of the signed 64 bit range will return an Error (eg. "9223372036854775808" / "-9223372036854775809").
    *
    * @return The parsed signed 64 bit integer, or an error.
    */
    [[nodiscard]] constexpr Result<i64> parseInt() const;

    /**
    * Parses an unsigned 64 bit integer from the string slice.
    * For example, the slice of "1234" returns an Ok variant of `-1234`.
    *
    * Errors:
    *
    * - Negatives will return an Error variant.
    * - Decimals will return an Error variant (eg. 12.5).
    * - Anything out of the signed 64 bit range will return an Error (eg. "18446744073709551616").
    *
    * @return The parsed unsigned 64 bit integer, or an error.
    */
    [[nodiscard]] constexpr Result<u64> parseUint() const;

    friend std::ostream& operator << (std::ostream& os, const Str& inStr) {
      return os.write(inStr.buffer, inStr.len);
    }

  private:

    bool equalStr(const gk::Str& str) const;

    Option<usize> findChar(char c) const;
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

inline constexpr bool gk::Str::operator==(char c) const
{
  if (len == 1 && buffer[0] == c) {
    return true;
  }
  return false;
}

inline constexpr bool gk::Str::operator==(const Str& str) const
{
  if (len != str.len) {
    return false;
  }
  if (buffer == str.buffer) {
    return true;
  }

  if (std::is_constant_evaluated()) {
    for (usize i = 0; i < len; i++) {
      if (buffer[i] != str.buffer[i]) {
        return false;
      }
    }
    return true;
  }
  else {
    return equalStr(str);
  }
}

inline constexpr gk::Option<gk::usize> gk::Str::find(char c) const
{
  if (std::is_constant_evaluated()) {
    for (usize i = 0; i < len; i++) {
      if (buffer[i] == c) return Option<usize>(i);
    }
    return Option<usize>();
  }
  else {
    return findChar(c);
  }
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
  else if (str.len == len) {
    if (*this == str) {
      return Option<usize>(0);
    }
    return Option<usize>();
  }

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
  else if (str.len == len) {
    if (*this == str) {
      return Option<usize>(0);
    }
    return Option<usize>();
  }
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

inline constexpr gk::Result<bool> gk::Str::parseBool() const
{
  constexpr usize TRUE_STRING_BUFFER = []() {
    usize out = 0;
    out |= static_cast<usize>('t');
    out |= static_cast<usize>('r') << 8;
    out |= static_cast<usize>('u') << 16;
    out |= static_cast<usize>('e') << 24;
    return out;
  }();

  constexpr usize FALSE_STRING_BUFFER = []() {
    usize out = 0;
    out |= static_cast<usize>('f');
    out |= static_cast<usize>('a') << 8;
    out |= static_cast<usize>('l') << 16;
    out |= static_cast<usize>('s') << 24;
    out |= static_cast<usize>('e') << 32;
    return out;
  }();

  if (len == 4) { // maybe true
    const usize cmp = [&]() {
      usize out = 0;
      out |= static_cast<usize>(buffer[0]);
      out |= static_cast<usize>(buffer[1]) << 8;
      out |= static_cast<usize>(buffer[2]) << 16;
      out |= static_cast<usize>(buffer[3]) << 24;
      return out;
    }();

    if (cmp == TRUE_STRING_BUFFER) {
      return ResultOk<bool>(true);
    }
    else {
      return ResultErr();
    }
  }
  else if (len == 5) { // maybe false
    const usize cmp = [&]() {
      usize out = 0;
      out |= static_cast<usize>(buffer[0]);
      out |= static_cast<usize>(buffer[1]) << 8;
      out |= static_cast<usize>(buffer[2]) << 16;
      out |= static_cast<usize>(buffer[3]) << 24;
      out |= static_cast<usize>(buffer[4]) << 32;
      return out;
    }();

    if (cmp == FALSE_STRING_BUFFER) {
      return ResultOk<bool>(false);
    }
    else {
      return ResultErr();
    }
  }
  else {
    return ResultErr();
  }
}

namespace gk {
  namespace internal {
    constexpr u64 convertCharToInt(char c) {
      return static_cast<u64>(c - '0');
    }
  }
}

inline constexpr gk::Result<gk::i64> gk::Str::parseInt() const
{
  if (len == 0) return ResultErr();

  const bool isNegative = buffer[0] == '-';

  if (len == 1) { // fast return
    if (buffer[0] >= '0' && buffer[0] <= '9') {
      return ResultOk<i64>(static_cast<i64>(internal::convertCharToInt(buffer[0])));
    }
  }
  else if (len == 2 && isNegative) {
    if (buffer[1] >= '0' && buffer[1] <= '9') {
      return ResultOk<i64>(static_cast<i64>(internal::convertCharToInt(buffer[1])) * -1LL);
    }
  }

  // validate
  do {
    // max/min signed int64.
    constexpr usize MAX_NUMBER_LENGTH = 19;
    bool isLengthMax = false;
    if (isNegative) {
      if (len > (MAX_NUMBER_LENGTH + 1)) return ResultErr();
      if (len == (MAX_NUMBER_LENGTH + 1)) isLengthMax = true;
    }
    else {
      if (len > MAX_NUMBER_LENGTH) return ResultErr();
      if (len == MAX_NUMBER_LENGTH) isLengthMax = true;
    }

    usize i = static_cast<usize>(isNegative); // start at 0 for positive, or 1 for negative;
    for (; i < len; i++) {
      const char c = buffer[i];
      if (c >= '0' && c <= '9') {
        continue;
      }
      return ResultErr();
    }

    // MUST ensure string is within 64 bit signed int bounds.
    if (isLengthMax) {
      i = static_cast<usize>(isNegative);

      // + 9,223,372,036,854,775,807
      // - 9,223,372,036,854,775,808
      if (buffer[i] != '9') {
        break;
      }
      if (buffer[i + 1] > '2') {
        return ResultErr();
      }
      if (buffer[i + 2] > '2') {
        return ResultErr();
      }
      if (buffer[i + 3] > '3') {
        return ResultErr();
      }
      if (buffer[i + 4] > '3') {
        return ResultErr();
      }
      if (buffer[i + 5] > '7') {
        return ResultErr();
      }
      if (buffer[i + 6] > '2') {
        return ResultErr();
      }
      if (buffer[i + 7] > '0') {
        return ResultErr();
      }
      if (buffer[i + 8] > '3') {
        return ResultErr();
      }
      if (buffer[i + 9] > '6') {
        return ResultErr();
      }
      if (buffer[i + 10] > '8') {
        return ResultErr();
      }
      if (buffer[i + 11] > '5') {
        return ResultErr();
      }
      if (buffer[i + 12] > '4') {
        return ResultErr();
      }
      if (buffer[i + 13] > '7') {
        return ResultErr();
      }
      if (buffer[i + 14] > '7') {
        return ResultErr();
      }
      if (buffer[i + 15] > '5') {
        return ResultErr();
      }
      if (buffer[i + 16] > '8') {
        return ResultErr();
      }
      if (buffer[i + 17] > '0') {
        return ResultErr();
      }

      if (isNegative) {
        if (buffer[i + 18] > '8') {
          return ResultErr();
        }
      }
      else {
        if (buffer[i + 18] > '7') {
          return ResultErr();
        }
      }
    }
  } while (false); // allow breaking

  const char* end = buffer + len - 1;

  i64 out = 0;

  {
    const i64 lengthToCheck = static_cast<i64>(isNegative ? len - 1 : len);
    for (i64 i = 0; i < lengthToCheck; i++) {
      const i64 tens = [](i64 index) {
        i64 ret = 1;
        for (i64 _i = 0; _i < index; _i++) {
          ret *= 10;
        }
        return ret;
      }(i);

      const char c = *(end - i); // decrement
      out += static_cast<i64>(internal::convertCharToInt(c)) * tens;
    }
  }

  if (isNegative) {
    out *= -1LL;
  }

  return ResultOk<i64>(out);
}

inline constexpr gk::Result<gk::u64> gk::Str::parseUint() const
{
  if (len == 0) return ResultErr();

  if (len == 1) { // fast return
    if (buffer[0] >= '0' && buffer[0] <= '9') {
      return ResultOk<u64>(internal::convertCharToInt(buffer[0]));
    }
  }

  // validate
  do {
    // max chars in an unsigned 64 bit integer
    constexpr usize MAX_NUMBER_LENGTH = 20;
    const bool isLengthMax = len == MAX_NUMBER_LENGTH;

    if (len > MAX_NUMBER_LENGTH) return ResultErr();

    for (usize i = 0; i < len; i++) {
      const char c = buffer[i];
      if (c >= '0' && c <= '9') {
        continue;
      }
      return ResultErr();
    }

    if (!isLengthMax) {
      break;
    }

    // 18,446,744,073,709,551,615
    if (buffer[0] > '1') {
      return ResultErr();
    }
    if (buffer[1] > '8') {
      return ResultErr();
    }
    if (buffer[2] > '4') {
      return ResultErr();
    }
    if (buffer[3] > '4') {
      return ResultErr();
    }
    if (buffer[4] > '6') {
      return ResultErr();
    }
    if (buffer[5] > '7') {
      return ResultErr();
    }
    if (buffer[6] > '4') {
      return ResultErr();
    }
    if (buffer[7] > '4') {
      return ResultErr();
    }
    if (buffer[8] > '0') {
      return ResultErr();
    }
    if (buffer[9] > '7') {
      return ResultErr();
    }
    if (buffer[10] > '3') {
      return ResultErr();
    }
    if (buffer[11] > '7') {
      return ResultErr();
    }
    if (buffer[12] > '0') {
      return ResultErr();
    }
    if (buffer[13] > '9') {
      return ResultErr();
    }
    if (buffer[14] > '5') {
      return ResultErr();
    }
    if (buffer[15] > '5') {
      return ResultErr();
    }
    if (buffer[16] > '1') {
      return ResultErr();
    }
    if (buffer[17] > '6') {
      return ResultErr();
    }
    if (buffer[18] > '1') {
      return ResultErr();
    }
    if (buffer[19] > '5') {
      return ResultErr();
    }
  } while (false);

  const char* end = buffer + len - 1;

  u64 out = 0;

  {
    for (u64 i = 0; i < len; i++) {
      const u64 tens = [](u64 index) {
        u64 ret = 1;
        for (u64 _i = 0; _i < index; _i++) {
          ret *= 10;
        }
        return ret;
      }(i);

      const char c = *(end - i); // decrement
      out += internal::convertCharToInt(c) * tens;
    }
  }

  return ResultOk<u64>(out);
}