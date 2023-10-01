#pragma once

#include "../Asserts.h"
#include "../BasicTypes.h"
#include "Error.h"
#include <type_traits>

namespace gk
{
	template<typename T>
	struct ResultOk {
		T _value;
		constexpr ResultOk(const T& value) : _value(value) {}
		constexpr ResultOk(T&& value) : _value(std::move(value)) {}
	};

	template<typename E>
		requires(std::is_base_of_v<Error, E>)
	struct ResultErr {
		E* _value;
		constexpr ResultErr(E* value) : _value(value) {}
	};

	template<typename T, typename E = Error, typename Enable = void>
		requires(std::is_base_of_v<Error, E>)
	struct Result;

	/* Result specialization for pointer types. */
	template<typename T, typename E>
	struct Result<T, E, typename std::enable_if_t<std::is_pointer_v<T>>>
	{
		Result() = delete;

		Result(const ResultOk<T>& _ok) {
			gk_assertm((_ok._value & POINTER_BITMASK) == _ok._value, "Value pointer must only occupy the first 48 bits");
			_value = _ok._value;
		}

		Result(const ResultErr<E>& _err) {
			const uint64 pointerBitmask = ((const uint64)_err._value) & POINTER_BITMASK;
			gk_assertm(pointerBitmask == (const uint64)_err._value, "Error pointer must only occupy the first 48 bits");
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_value = pointerBitmask | IS_ERROR_BITFLAG;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		Result(const ResultErr<OtherErr>& _err) {
			const uint64 pointerBitmask = ((const uint64)_err._value) & POINTER_BITMASK;
			gk_assertm(pointerBitmask == (const uint64)_err._value, "Error pointer must only occupy the first 48 bits");
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_value = pointerBitmask | IS_ERROR_BITFLAG;
		}

		/* Not allowed to copy due to potential ownership of error */
		Result(const Result<T, E>& other) = delete;

		Result(Result<T, E>&& other) noexcept : _value(other._value) {
			other._value = 0;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		Result(Result<T, OtherErr>&& other) noexcept : _value(other._value) {
			other._value = 0;
		}

		/* Not allowed to copy due to potential ownership of error */
		Result& operator = (const Result<T, E>& other) = delete;

		Result& operator = (Result<T, E>&& other) noexcept {
			_value = other._value;
			other._value = 0;
			return *this;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		Result& operator = (Result<T, E>&& other) noexcept {
			_value = other._value;
			other._value = 0;
			return *this;
		}

		~Result() {
			if (isError()) delete error();
		}

		[[nodiscard]] bool isError() const {
			return _value & IS_ERROR_BITFLAG;
		}

		[[nodiscard]] T ok() {
			gk_assertm(!isError(), "Cannot get the value result that is an error");
			return (T)ptr();
		}

		[[nodiscard]] const T ok() const {
			gk_assertm(!isError(), "Cannot get the const value result that is an error");
			return (const T)ptr();
		}

		[[nodiscard]] E* error() {
			gk_assertm(isError(), "Cannot get the error result that is a valid value");
			return (E*)ptr();
		}

		[[nodiscard]] const E* error() const {
			gk_assertm(isError(), "Cannot get the const error result that is a valid value");
			return (const E*)ptr();
		}

	private:

		void* ptr() {
			return (void*)(_value & POINTER_BITMASK);
		}

	private:

		// first 48 bits
		static constexpr uint64 POINTER_BITMASK = 0xFFFFFFFFFFFFULL;
		static constexpr uint64 IS_ERROR_BITFLAG = 1ULL << 63;

		uint64 _value;
	};

	/* Result specialization for small non-pointer types. */
	template<typename T, typename E>
		requires(std::is_base_of_v<Error, E>)
	struct Result<T, E, typename std::enable_if_t< !std::is_pointer_v<T> && (sizeof(T) < 8)>>
	{
		Result() = delete;

		Result(const ResultOk<T>& _ok) {
			_value = 0;
			memcpy(&_value, &_ok._value, sizeof(T));
			gk_assertm(!isError(), "Memcpy should not have overridden the first bit");
		}

		Result(const ResultErr<E>& _err) {
			const uint64 pointerBitmask = ((const uint64)_err._value) & POINTER_BITMASK;
			gk_assertm(pointerBitmask == (const uint64)_err._value, "Error pointer must only occupy the first 48 bits");
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_value = pointerBitmask | IS_ERROR_BITFLAG;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		Result(const ResultErr<OtherErr>& _err) {
			const uint64 pointerBitmask = ((const uint64)_err._value) & POINTER_BITMASK;
			gk_assertm(pointerBitmask == (const uint64)_err._value, "Error pointer must only occupy the first 48 bits");
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_value = pointerBitmask | IS_ERROR_BITFLAG;
		}

		/* Not allowed to copy due to potential ownership of error */
		Result(const Result<T, E>& other) = delete;

		Result(Result<T, E>&& other) noexcept : _value(other._value) {
			other._value = 0;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		Result(Result<T, OtherErr>&& other) noexcept : _value(other._value) {
			other._value = 0;
		}

		/* Not allowed to copy due to potential ownership of error */
		Result& operator = (const Result<T, E>& other) = delete;

		Result& operator = (Result<T, E>&& other) noexcept {
			_value = other._value;
			other._value = 0;
			return *this;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		Result& operator = (Result<T, OtherErr>&& other) noexcept {
			_value = other._value;
			other._value = 0;
			return *this;
		}

		~Result() {
			if (isError()) delete error();
		}

		[[nodiscard]] bool isError() const {
			return _value & IS_ERROR_BITFLAG;
		}

		[[nodiscard]] T ok() {
			gk_assertm(!isError(), "Cannot get the value result that is an error");
			if constexpr (std::is_fundamental_v<T>) {
				return *reinterpret_cast<T*>(&_value);
			}
			else {
				T obj;
				memcpy(&obj, &_value, sizeof(T));
				return obj;
			}
		}

		[[nodiscard]] const T ok() const {
			gk_assertm(!isError(), "Cannot get the const value result that is an error");
			if constexpr (std::is_fundamental_v<T>) {
				return *reinterpret_cast<T*>(&_value);
			}
			else {
				T obj;
				memcpy(&obj, &_value, sizeof(T));
				return obj;
			}
		}

		[[nodiscard]] E* error() {
			gk_assertm(isError(), "Cannot get the error result that is a valid value");
			return (E*)ptr();
		}

		[[nodiscard]] const E* error() const {
			gk_assertm(isError(), "Cannot get the const error result that is a valid value");
			return (const E*)ptr();
		}

	private:

		void* ptr() {
			return (void*)(_value & POINTER_BITMASK);
		}

	private:

		// first 48 bits
		static constexpr uint64 POINTER_BITMASK = 0xFFFFFFFFFFFFULL;
		static constexpr uint64 IS_ERROR_BITFLAG = 1ULL << 63;

		uint64 _value;
	};

	/* Result specialization for non-pointer types with a size greater than 7. */
	template<typename T, typename E>
		requires(std::is_base_of_v<Error, E>)
	struct Result<T, E, typename std::enable_if_t< !std::is_pointer_v<T> && (sizeof(T) >= 8)>> {

		Result() = delete;

		constexpr Result(const ResultOk<T>& _ok) {
			_error = nullptr;
			_value = _ok._value;
		}

		constexpr Result(const ResultErr<E>& _err) {
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_error = _err._value;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		constexpr Result(const ResultErr<OtherErr>& _err) {
			gk_assertm(_err._value != nullptr, "Cannot use a null error");
			_error = _err._value;
		}

		/* Not allowed to copy due to potential ownership of error */
		Result(const Result<T, E>& other) = delete;

		constexpr Result(Result<T, E>&& other) noexcept {
			_error = other._error;
			other._error = nullptr;
			if (_error == nullptr) {
				_value = std::move(other._value);
			}
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		constexpr Result(Result<T, OtherErr>&& other) noexcept {
			_error = other._error;
			other._error = nullptr;
			if (_error == nullptr) {
				_value = std::move(other._value);
			}
		}

		constexpr ~Result() {
			if (isError() && _error != nullptr) { delete _error; }
		}

		/* Not allowed to copy due to potential ownership of error */
		Result& operator = (const Result<T, E>& other) = delete;

		constexpr Result& operator = (Result<T, E>&& other) noexcept {
			_error = other._error;
			other._error = nullptr;
			if (_error == nullptr) {
				_value = std::move(other._value);
			}
			return *this;
		}

		template<typename OtherErr>
			requires(std::is_same_v<E, Error>, std::is_base_of_v<Error, OtherErr>)
		constexpr Result& operator = (Result<T, E>&& other) noexcept {
			_error = other._error;
			other._error = nullptr;
			if (_error == nullptr) {
				_value = std::move(other._value);
			}
			return *this;
		}

		constexpr [[nodiscard]] bool isError() const {
			return _error != nullptr;
		}

		constexpr [[nodiscard]] T ok() {
			gk_assertm(!isError(), "Cannot get the value result that is an error");
			return _value;
		}

		constexpr [[nodiscard]] const T ok() const {
			gk_assertm(!isError(), "Cannot get the const value result that is an error");
			return _value;
		}

		constexpr [[nodiscard]] T&& okMove() {
			gk_assertm(!isError(), "Cannot move the value result that is an error");
			return std::move(_value);
		}

		constexpr [[nodiscard]] E* error() {
			gk_assertm(isError(), "Cannot get the error result that is a valid value");
			return _error;
		}

		constexpr [[nodiscard]] const E* error() const {
			gk_assertm(isError(), "Cannot get the const error result that is a valid value");
			return _error;
		}

		constexpr [[nodiscard]] E* errorMove() {
			gk_assertm(isError(), "Cannot move the error result that is a valid value");
			E* ptr = _error;
			_error = nullptr;
			return ptr;
		}

	private:

		T _value;
		E* _error;

	};
}