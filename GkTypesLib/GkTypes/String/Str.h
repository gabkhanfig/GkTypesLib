#pragma once
#include "../BasicTypes.h"
#include "../Asserts.h"
#include "Utf8.h"
#include <xtr1common>

namespace gk
{

  /* UTF8 characters pointer. Can deduce string length at compile time from string literals. 
  To enable UTF8, navigate to:
  Project (Or right click on the project in the solution explorer) -> [Your Project] Properties -> C/C++ -> Command Line -> Additional Options -> Insert "/utf-8" 
  It's also necessary that all of the project files are saved using UTF-8. 
  Example of usage:
  gk::Str str = gk::Str("hello world!);
  gk::Str str2 = "hello world!";
  auto str3 = "hello world!"_str; */
  struct Str {
    /* Reference to a pre-existing string. On destruction, will not free this data. */
    const char* str;
    /* Number of utf8 characters excluding null terminator. */
    uint64 len;
    /* Total string bytes and null terminator. */
    uint64 totalBytes;

    constexpr Str() : str(nullptr), len(0), totalBytes(0) {}

    template<uint64 N>
    consteval Str(char const (&inStr)[N]) { // consteval to force compile time execution of length from string literal
      const Utf8Metadata metadata = gk::utf8::strlen(inStr).ok();
      str = inStr;
      len = metadata.length;
      totalBytes = metadata.totalBytes;
      gk_assertm(inStr[totalBytes - 1] == '\0', "Str is not null terminated");
    }
    
    static constexpr Str fromAscii(const char* inStr) {
      Str str;
      str.str = inStr;
      str.len = std::char_traits<char>::length(inStr);
      str.totalBytes = str.len + 1;
      return str;
    }

    static constexpr Str fromAscii(const char* inStr, uint64 knownLength) {
      gk_assertm(inStr[knownLength] == '\0', "Str is not null terminated");
      Str str;
      str.str = inStr;
      str.len = knownLength;
      str.totalBytes = knownLength + 1;
      return str;
    }

    static constexpr gk::Result<Str, InvalidUtf8Error> fromUtf8(const char* inStr) {
      gk::Result<Utf8Metadata> result = gk::utf8::strlen(inStr);
      if (result.isError()) [[unlikely]] {
        return gk::ResultErr(result.errorMove());
      }
      Str str;
      str.str = inStr;
      str.len = result.ok().length;
      str.totalBytes = result.ok().totalBytes;
      return gk::ResultOk(str);
    }

    constexpr Str(const Str& other) : str(other.str), len(other.len), totalBytes(other.totalBytes) {}

    constexpr Str(Str&& other) noexcept : str(other.str), len(other.len), totalBytes(other.totalBytes) {
#if GK_CHECK
      other.str = nullptr;
      other.len = 0;
      other.totalBytes = 0;
#endif
    }

    constexpr Str& operator = (const Str& other) {
      str = other.str;
      len = other.len;
      totalBytes = other.totalBytes;
      return *this;
    }

    constexpr Str& operator = (Str&& other) noexcept {
      str = other.str;
      len = other.len;
      totalBytes = other.totalBytes; 
#if GK_CHECK
      other.str = nullptr;
      other.len = 0;
      other.totalBytes = 0;
#endif
      return *this;
    }
  };
}

consteval gk::Str operator "" _str(const char* inStr, size_t) {
  const gk::Utf8Metadata metadata = gk::utf8::strlen(inStr).ok();
  gk::Str str;
  str.str = inStr;
  str.len = metadata.length;
  str.totalBytes = metadata.totalBytes;
  gk_assertm(inStr[str.totalBytes - 1] == '\0', "Str is not null terminated");
  return str;
}