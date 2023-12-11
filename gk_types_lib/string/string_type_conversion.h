#pragma once

#include "string.h"
#include "../array/array_list.h"

namespace gk
{
	template<typename T>
	constexpr String toString(const T& value);

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
				string.append(toString(arr[i]));
				if (i != (arr.len() - 1)) {
					string.append(", "_str);
				}
			}
			string.append(']');
			return string;
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
		else if constexpr (internal::is_array_list<T>) {
			return internal::convertArrayListToString(value);
		}
		else {
			return String::from(value);
		}
	}
}

