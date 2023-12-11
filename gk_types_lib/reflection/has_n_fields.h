#pragma once

#include <type_traits>

namespace gk
{
	namespace internal
	{
		struct any_type {
			template<typename T>
			constexpr operator T() const noexcept;
		};

		template<typename T, typename Is, typename = void>
		struct constructible_with_n_fields_impl : std::false_type {};

		template<typename T, size_t... Is>
		struct constructible_with_n_fields_impl <
			T, std::index_sequence<Is...>,
			std::void_t<decltype(T{ (void(Is), any_type{})... }) >> : std::true_type {};

		template<typename T, size_t N>
		using constructible_with_n_fields =
			constructible_with_n_fields_impl<T, std::make_index_sequence<N>>;

		/**
		* Check if a given type T has N member fields.
		* 
		* @param T: type
		* @param N: number of fields to check has
		*/
		template<typename T, size_t N>
		constexpr bool has_n_fields = constructible_with_n_fields<T, N>::value && !constructible_with_n_fields<T, N + 1>::value;
	}
}