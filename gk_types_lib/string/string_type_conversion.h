#pragma once

#include "string.h"
#include "../array/array_list.h"
#include "global_string.h"

namespace gk
{
	/**
	* Similar to gk::String::from<T>() but allows
	* conversion from other types such as gk::ArrayList<T> and gk::GlobalString.
	*
	* @param value: Template value to be converted to a string.
	*/
	template<typename T>
	constexpr String toString(const T& value);

	/**
	* Similar to gk::Str::parse<T>() but allows
	* conversion to other types such as gk::ArrayList<T> and gk::GlobalString.
	* 
	* @param str: String slice to parse.
	*/
	template<typename T>
	constexpr Result<T> parseStr(const gk::Str& str);

	/**
	* Similar to gk::String::parse<T>() but allows
	* conversion to other types such as gk::ArrayList<T> and gk::GlobalString.
	* This is basically a wrapper around `gk::parseStr()`.
	* 
	* @param string: String to parse.
	*/
	template<typename T>
	constexpr Result<T> parseString(const gk::String& string);

	namespace internal
	{
		template <class T, template <class...> class Template>
		struct is_specialization : std::false_type {};

		template <template <class...> class Template, class... Args>
		struct is_specialization<Template<Args...>, Template> : std::true_type {};

		template <typename T>
		constexpr bool is_array_list = is_specialization<T, ArrayList>::value;

		template<typename T>
		constexpr String convertArrayListToString(const ArrayList<T>& arr) {
			String string = '[';
			for (usize i = 0; i < arr.len(); i++) {
				const bool isStringType = std::is_same_v<T, Str> || std::is_same_v<T, String> || std::is_same_v<T, GlobalString>;

				if (isStringType) {
					string.append('\"');
				}
				string.append(toString(arr[i]));
				if (isStringType) {
					string.append('\"');
				}

				if (i != (arr.len() - 1)) {
					string.append(", "_str);
				}
			}
			string.append(']');
			return string;
		}

		template<typename T>
		constexpr Result<ArrayList<T>> convertStrToArrayList(const gk::Str& str) {
			static_assert(!std::is_same_v<T, Str>, "Cannot convert string slice to array of string slices safely due to lifetime issues. Please use gk::ArrayList<gk::String> instead");

			if (str.len < 2) { // must be at least "[]"
				return ResultErr();
			}
			if (str.len == 2) {
				if (str == "[]"_str) {
					return ResultOk<ArrayList<T>>();
				}
				else {
					return ResultErr();
				}
			}

			if (str.buffer[0] != '[') {
				return ResultErr();
			}
			if (str.buffer[str.len - 1] != ']') {
				return ResultErr();
			}

			ArrayList<T> accumulate = ArrayList<T>::withCapacity(globalHeapAllocator()->clone(), 1);

			usize current = 1; // start at index after the first [
			Option<usize> indexOfComma;

			while (true) {
				indexOfComma = str.findFrom(',', current);
				const bool noMoreCommas = indexOfComma.none();


				const char* start = [&]() {
					if (noMoreCommas) {
						// before the ending ]
						const usize countToNext = (str.len - 1) - current;
						for (usize i = 0; i < countToNext; i++) {
							if ((str.buffer + current)[i] != ' ') {
								return str.buffer + current + i;
							}
						}
						return (const char*)nullptr;
					}
					else {
						const usize commaIndex = indexOfComma.someCopy();
						if (commaIndex == current) {
							return (const char*)nullptr;
						}
						for (usize i = current; i < commaIndex; i++) {
							if (str.buffer[i] != ' ') {
								return str.buffer + i;
							}
						}
						return (const char*)nullptr;
					}
				}();
				if (start == nullptr) return ResultErr();

				const char* end = [&]() {
					if (noMoreCommas) {
						usize i = str.len - 2;
						while (true) {
							if (str.buffer[i] != ' ') {
								return &str.buffer[i + 1];
							}
							i--;
						}
					}
					else {
						usize i = indexOfComma.someCopy() - 1;
						while (true) {
							if (str.buffer[i] != ' ') {
								return &str.buffer[i + 1];
							}
							i--;
						}
					}
				}();
				if (end == nullptr) return ResultErr();
				
				if (std::is_same_v<T, String> || std::is_same_v<T, GlobalString>) {
					if (*start != '\"' && *start != '\'') {
						return ResultErr();
					}
					if (*(end - 1) != '\"' && *(end - 1) != '\'') {
						return ResultErr();
					}
					if (start == (end - 1)) {
						return ResultErr();
					}
					const gk::Str elem = Str::fromSlice(start + 1, static_cast<usize>((end - 1) - (start + 1)));
					Result<T> parsed = parseStr<T>(elem);
					if (parsed.isError()) {
						return ResultErr();
					}
					accumulate.push(std::move(parsed.ok()));
				}
				else {
					const gk::Str elem = Str::fromSlice(start, static_cast<usize>(end - start));
					Result<T> parsed = parseStr<T>(elem);
					if (parsed.isError()) {
						return ResultErr();
					}
					accumulate.push(std::move(parsed.ok()));
				}
				
				if (noMoreCommas) {
					return ResultOk<ArrayList<T>>(std::move(accumulate));
				}
				else {
					current = indexOfComma.someCopy() + 1;
				}
			}
		}
	}

	template<typename T>
	constexpr String toString(const T& value)
	{
		if constexpr (std::is_same_v<T, String>) {
			return String(value);
		}
		else if constexpr (std::is_same_v<T, Str>) {
			return String(value);
		}
		else if constexpr (std::is_same_v<T, GlobalString>) {
			return String(value.toString());
		}
		else if constexpr (internal::is_array_list<T>) {
			return internal::convertArrayListToString(value);
		}
		else {
			return String::from(value);
		}
	}

	template<typename T>
	constexpr Result<T> parseStr(const gk::Str& str)
	{
		if constexpr (std::is_same_v<T, String>) {
			return ResultOk<String>(String(str));
		}
		else if constexpr (std::is_same_v<T, Str>) {
			return ResultOk<Str>(Str(str));
		}
		else if constexpr (std::is_same_v<T, GlobalString>) {
			return ResultOk<GlobalString>(GlobalString::create(String(str)));
		}
		else if constexpr (internal::is_array_list<T>) {
			return internal::convertStrToArrayList<typename T::ValueType>(str); //internal::convertArrayListToString(value);
		}
		else {
			return str.parse<T>();
		}
	}

	template<typename T>
	constexpr Result<T> parseString(const gk::String& string)
	{
		if constexpr (std::is_same_v<T, String>) {
			return String(string);
		}
		else if constexpr (std::is_same_v<T, Str>) {
			return Str(string.asStr());
		}
		else {
			return parseStr<T>(string.asStr());
		}	
	}
}

