#pragma once

#include "has_n_fields.h"
#include <type_traits>
#include "../string/string.h"
#include "../string/str.h"
#include <source_location>
#include <iostream>

namespace gk
{
	namespace internal 
	{
		template <class T> 
		extern const T external;

		template <class TPtr>
		struct ptr { const TPtr* ptr; };

		template <auto Ptr>
		[[nodiscard]] consteval gk::Str getFieldNameImpl() {
			const gk::Str name = gk::Str::fromNullTerminated(std::source_location::current().function_name()); 
#if defined(__clang__)

#elif defined(__GNUC__)

#elif defined(_MSC_VER)
			const auto split = name.substring(0, name.findLast('}').some());
			return split.substring(split.findLast('>').some() + 1, split.len);
#endif
		}

		template<usize N, class T>
		[[nodiscard]] constexpr auto nthPtr(T&& t) {
			static_assert(!has_n_fields<T, 0>, "Cannot get the nth field name of a type with no fields");

			if constexpr (has_n_fields<T, 1>) {
				static_assert(N < 1, "Cannot access a field beyond the number within the field. T has 1 field");
				auto&& [p1] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
			}
			else if constexpr (has_n_fields<T, 2>) {
				static_assert(N < 2, "Cannot access a field beyond the number within the field. T has 2 fields");
				auto&& [p1, p2] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
			}
			else if constexpr (has_n_fields<T, 3>) {
				static_assert(N < 3, "Cannot access a field beyond the number within the field. T has 3 fields");
				auto&& [p1, p2, p3] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
			}
			else if constexpr (has_n_fields<T, 4>) {
				static_assert(N < 4, "Cannot access a field beyond the number within the field. T has 4 fields");
				auto&& [p1, p2, p3, p4] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
			}
			else if constexpr (has_n_fields<T, 5>) {
				static_assert(N < 5, "Cannot access a field beyond the number within the field. T has 5 fields");
				auto&& [p1, p2, p3, p4, p5] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
				if constexpr (N == 4) return ptr<decltype(p5)>{&p5};
			}
			else if constexpr (has_n_fields<T, 6>) {
				static_assert(N < 6, "Cannot access a field beyond the number within the field. T has 6 fields");
				auto&& [p1, p2, p3, p4, p5, p6] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
				if constexpr (N == 4) return ptr<decltype(p5)>{&p5};
				if constexpr (N == 5) return ptr<decltype(p6)>{&p6};
			}
			else if constexpr (has_n_fields<T, 7>) {
				static_assert(N < 7, "Cannot access a field beyond the number within the field. T has 7 fields");
				auto&& [p1, p2, p3, p4, p5, p6, p7] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
				if constexpr (N == 4) return ptr<decltype(p5)>{&p5};
				if constexpr (N == 5) return ptr<decltype(p6)>{&p6};
				if constexpr (N == 6) return ptr<decltype(p7)>{&p7};
			}
			else if constexpr (has_n_fields<T, 8>) {
				static_assert(N < 8, "Cannot access a field beyond the number within the field. T has 8 fields");
				auto&& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
				if constexpr (N == 4) return ptr<decltype(p5)>{&p5};
				if constexpr (N == 5) return ptr<decltype(p6)>{&p6};
				if constexpr (N == 6) return ptr<decltype(p7)>{&p7};
				if constexpr (N == 7) return ptr<decltype(p8)>{&p8};
			}
			else if constexpr (has_n_fields<T, 9>) {
				static_assert(N < 9, "Cannot access a field beyond the number within the field. T has 9 fields");
				auto&& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
				if constexpr (N == 4) return ptr<decltype(p5)>{&p5};
				if constexpr (N == 5) return ptr<decltype(p6)>{&p6};
				if constexpr (N == 6) return ptr<decltype(p7)>{&p7};
				if constexpr (N == 7) return ptr<decltype(p8)>{&p8};
				if constexpr (N == 8) return ptr<decltype(p9)>{&p9};
			}
			else if constexpr (has_n_fields<T, 9>) {
				static_assert(N < 9, "Cannot access a field beyond the number within the field. T has 9 fields");
				auto&& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
				if constexpr (N == 0) return ptr<decltype(p1)>{&p1};
				if constexpr (N == 1) return ptr<decltype(p2)>{&p2};
				if constexpr (N == 2) return ptr<decltype(p3)>{&p3};
				if constexpr (N == 3) return ptr<decltype(p4)>{&p4};
				if constexpr (N == 4) return ptr<decltype(p5)>{&p5};
				if constexpr (N == 5) return ptr<decltype(p6)>{&p6};
				if constexpr (N == 6) return ptr<decltype(p7)>{&p7};
				if constexpr (N == 7) return ptr<decltype(p8)>{&p8};
				if constexpr (N == 8) return ptr<decltype(p9)>{&p9};
			}
		}
	} // namespace internal

	template <class T>
	struct NamedField {
		using ValueType = T;

		gk::Str name{};
		T value{};
	};

	/**
	* Get the name of a class/struct member field as a string slice.
	* NOTE: does not work with private members, and will cause compilation errors.
	* 
	* @param T: Reflection class/struct.
	*/
	template<typename T, usize N>
	constexpr gk::Str getFieldName() {
		return internal::getFieldNameImpl<internal::nthPtr<N>(internal::external<T>)>();
	}

	template<typename T>
	[[nodiscard]] constexpr auto toNamedFields(const T& t) {
		static_assert(!internal::has_n_fields<T, 0>, "Cannot get the named fields of a type T with no fields");

		if constexpr (internal::has_n_fields<T, 1>) {
			auto&& [p1] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1});
		}
		else if constexpr (internal::has_n_fields<T, 2>) {
			auto&& [p1, p2] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2});
		}
		else if constexpr (internal::has_n_fields<T, 3>) {
			auto&& [p1, p2, p3] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3});
		}
		else if constexpr (internal::has_n_fields<T, 4>) {
			auto&& [p1, p2, p3, p4] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3},
				NamedField<decltype(p4)>{.name = getFieldName<T, 3>(), .value = p4});
		}
		else if constexpr (internal::has_n_fields<T, 5>) {
			auto&& [p1, p2, p3, p4, p5] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3},
				NamedField<decltype(p4)>{.name = getFieldName<T, 3>(), .value = p4},
				NamedField<decltype(p5)>{.name = getFieldName<T, 4>(), .value = p5});
		}
		else if constexpr (internal::has_n_fields<T, 6>) {
			auto&& [p1, p2, p3, p4, p5, p6] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3},
				NamedField<decltype(p4)>{.name = getFieldName<T, 3>(), .value = p4},
				NamedField<decltype(p5)>{.name = getFieldName<T, 4>(), .value = p5},
				NamedField<decltype(p6)>{.name = getFieldName<T, 5>(), .value = p6});
		}
		else if constexpr (internal::has_n_fields<T, 7>) {
			auto&& [p1, p2, p3, p4, p5, p6, p7] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3},
				NamedField<decltype(p4)>{.name = getFieldName<T, 3>(), .value = p4},
				NamedField<decltype(p5)>{.name = getFieldName<T, 4>(), .value = p5},
				NamedField<decltype(p6)>{.name = getFieldName<T, 5>(), .value = p6},
				NamedField<decltype(p7)>{.name = getFieldName<T, 6>(), .value = p7});
		}
		else if constexpr (internal::has_n_fields<T, 8>) {
			auto&& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3},
				NamedField<decltype(p4)>{.name = getFieldName<T, 3>(), .value = p4},
				NamedField<decltype(p5)>{.name = getFieldName<T, 4>(), .value = p5},
				NamedField<decltype(p6)>{.name = getFieldName<T, 5>(), .value = p6},
				NamedField<decltype(p7)>{.name = getFieldName<T, 6>(), .value = p7},
				NamedField<decltype(p8)>{.name = getFieldName<T, 7>(), .value = p8});
		}
		else if constexpr (internal::has_n_fields<T, 9>) {
			auto&& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
			return std::tuple(
				NamedField<decltype(p1)>{.name = getFieldName<T, 0>(), .value = p1},
				NamedField<decltype(p2)>{.name = getFieldName<T, 1>(), .value = p2},
				NamedField<decltype(p3)>{.name = getFieldName<T, 2>(), .value = p3},
				NamedField<decltype(p4)>{.name = getFieldName<T, 3>(), .value = p4},
				NamedField<decltype(p5)>{.name = getFieldName<T, 4>(), .value = p5},
				NamedField<decltype(p6)>{.name = getFieldName<T, 5>(), .value = p6},
				NamedField<decltype(p7)>{.name = getFieldName<T, 6>(), .value = p7},
				NamedField<decltype(p8)>{.name = getFieldName<T, 7>(), .value = p8},
				NamedField<decltype(p9)>{.name = getFieldName<T, 8>(), .value = p9});
		}
		else {
			return std::tuple();
		}
	}
} // namespace gk