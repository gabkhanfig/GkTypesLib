#pragma once

#include "../doctest/doctest_proxy.h"
#include "../basic_types.h"
#include <type_traits>

namespace gk
{
	struct String;

	namespace internal
	{
		void resultExpectFail(const gk::String& message);
		void resultExpectErrorFail(const gk::String& message);
	}

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

		/**
		* @return If the variant is Ok
		*/
		[[nodiscard]] constexpr bool isOk() const {
			return _state == internal::ResultState::Ok;
		}
		
		/**
		* @return If the variant is Error
		*/
		[[nodiscard]] constexpr bool isError() const {
			return _state == internal::ResultState::Error;
		}

		/**
		* Moves out the Ok variant of this Result.
		* If the Result is the Error variant, or has been invalidated through a prior call to `ok()` or `error()`,
		* will assert.
		*
		* @return Ownership of the Ok value
		*/
		[[nodiscard]] constexpr T ok() {
			check_message(isOk(), "Result is not the Ok variant. Either it is an error, or the Ok variant has already been moved out");
			
			_state = internal::ResultState::Invalid;
			if constexpr (std::is_same_v<T, void>) {
				return;
			}
			else {
				return std::move(_union.ok);
			}
		}

		/**
		* Moves out the Error variant of this Result.
		* If the Result is the Ok variant, or has been invalidated through a prior call to `ok()` or `error()`,
		* will assert.
		* 
		* @return Ownership of the Error value
		*/
		[[nodiscard]] constexpr E error() {
			check_message(isError(), "Result is not the Error variant. Either it is ok, or the Error variant has already been moved out");

			_state = internal::ResultState::Invalid;
			if constexpr (std::is_same_v<E, void>) {
				return;
			}
			else {
				return std::move(_union.error);
			}
		}

		/**
		* Expects an Ok variant and returns it by move. 
		* If it's an Error variant, prints a custom message and debug breaks.
		* 
		* @param message: Custom message to be printed if it's the wrong variant
		*/
		[[nodiscard]] T expect(const struct gk::String& message) {
#if defined GK_TYPES_LIB_DEBUG || defined GK_TYPES_LIB_TEST
			if (!isOk()) {
				internal::resultExpectFail(message);
			}
#endif
			return ok();
		}

		/**
		* Expects an Error variant and returns it by move, and prints a message if it's not, along with debug breaking.
		* If it's an Ok variant, prints a custom message and debug breaks.
		* 
		* @param message: Custom message to be printed if it's the wrong variant
		*/
		[[nodiscard]] E expectError(const struct gk::String& message) {
#if defined GK_TYPES_LIB_DEBUG || defined GK_TYPES_LIB_TEST
			if (!isError()) {
				internal::resultExpectErrorFail(message);
			}
#endif
			return error();
		}

	private:

		

	private:

		ResultUnion _union;
		internal::ResultState _state;

	};
}