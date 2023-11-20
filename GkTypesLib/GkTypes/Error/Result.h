#pragma once

#include "../Asserts.h"
#include "../BasicTypes.h"
#include "Error.h"
#include <type_traits>

namespace gk
{
	template<typename T>
	struct ResultOk {
		using OkT = std::conditional_t<std::is_same_v<T, void>, bool, T>;
		OkT _value;

		constexpr ResultOk() {
			if constexpr (std::is_same_v<T, void>) {
				_value = false;
			}
			else {
				_value = T();
			}
		}
		constexpr ResultOk(const OkT& value) : _value(value) {}
		constexpr ResultOk(OkT&& value) : _value(std::move(value)) {}
	};

	template<typename E = void>
	struct ResultErr {
		using ErrorT = std::conditional_t<std::is_same_v<E, void>, bool, E>;
		ErrorT _value;

		constexpr ResultErr() {
			if constexpr (std::is_same_v<E, void>) {
				_value = false;
			}
			else {
				_value = E();
			}
		}
		constexpr ResultErr(const ErrorT& value) : _value(value) {}
		constexpr ResultErr(ErrorT&& value) : _value(std::move(value)) {}
	};

	namespace internal {
		enum class ResultState : char {
			Ok,
			Error,
			Invalid
		};
	}

	/**
	* Result that can store an Ok and Error variant of any type.
	* Using ok() or error() will invalidate the Result, as both values are moved out of the Result's ownership.
	* 
	* @param T: The "Ok" type. Can be void in which ok() will be ignored.
	* @param E: The "Error" type. Can be void in which error() will be ignored. Default = void
	*/
	template<typename T, typename E = void>
	struct Result {

	private:

		using OkT = std::conditional_t<std::is_same_v<T, void>, bool, T>;
		using ErrorT = std::conditional_t<std::is_same_v<E, void>, bool, E>;

		union ResultUnion {
			OkT ok;
			ErrorT error;

			constexpr ResultUnion(ResultOk<T>&& _ok)
				: ok(std::move(_ok._value)) {}

			constexpr ResultUnion(ResultErr<E>&& _err)
				: error(std::move(_err._value)) {}
		};

	public:

		Result() = delete;

		constexpr Result(ResultOk<T>&& _ok) :
			_union(std::move(_ok)), _state(internal::ResultState::Ok) {}

		constexpr Result(ResultErr<E>&& _err) :
			_union(std::move(_err)), _state(internal::ResultState::Error) {}

		constexpr Result(const Result&) = delete;
		constexpr Result(Result&& other) noexcept {
			_state = other._state;
			switch (_state) {
			case internal::ResultState::Ok:
				_union.ok = std::move(_union.ok);
				break;
			case internal::ResultState::Error:
				_union.error = std::move(_union.error);
				break;
			default:
				break;
			}
		}

		constexpr Result& operator = (const Result&) = delete;
		constexpr Result& operator = (Result&&) = delete;

		constexpr ~Result() {
			switch (_state) {
			case internal::ResultState::Ok:
				_union.ok.~OkT();
				break;
			case internal::ResultState::Error:
				_union.error.~ErrorT();
				break;
			default:
				break;
			}
		}

		[[nodiscard]] constexpr bool isOk() const {
			return _state == internal::ResultState::Ok;
		}

		[[nodiscard]] constexpr bool isError() const {
			return _state == internal::ResultState::Error;
		}

		[[nodiscard]] constexpr T ok() {
			gk_assertm(isOk(), "Result is not the Ok variant. Either it is an error, or the Ok variant has already been moved out");
			_state = internal::ResultState::Invalid;
			if constexpr (std::is_same_v<T, void>) {
				return;
			}
			else {
				return std::move(_union.ok);
			}
		}

		[[nodiscard]] constexpr E error() {
			gk_assertm(isOk(), "Result is not the Error variant. Either it is ok, or the Error variant has already been moved out");
			_state = internal::ResultState::Invalid;
			if constexpr (std::is_same_v<E, void>) {
				return;
			}
			else {
				return std::move(_union.error);
			}
		}

	private:

		ResultUnion _union;
		internal::ResultState _state;

	};
}