#include "result.h"
#include "../string/string.h"

void gk::internal::resultExpectFail(const gk::String& message)
{
	std::cout << "Result is not the Error variant. Either it is Ok, or the Error variant has already been moved out\n";
	std::cout << "expected: " << message << '\n';
	check(false);
}

void gk::internal::resultExpectErrorFail(const gk::String& message)
{
	std::cout << "Result is not the Ok variant. Either it is Error, or the Ok variant has already been moved out\n";
	std::cout << "expected: " << message << '\n';
	check(false);
}

#if GK_TYPES_LIB_TEST

using gk::Result;
using gk::ResultOk;
using gk::ResultErr;
using gk::u32;

namespace gk
{
	namespace unitTests
	{
		constexpr Result<u32> testResultOk() {
			return ResultOk<u32>(13);
		}

		constexpr Result<void, u32> testResultError() {
			return ResultErr<u32>(55);
		}
	}
}

test_case("Result Ok") {
	u32 ok = gk::unitTests::testResultOk().ok();
	check_eq(ok, 13);
}

test_case("Result Err") {
	u32 err = gk::unitTests::testResultError().error();
	check_eq(err, 55);
}

test_case("Result Is Ok") {
	Result<u32> res = gk::unitTests::testResultOk();
	check(res.isOk());
	check(!res.isError());
}

test_case("Result Is Ok") {
	Result<void, u32> res = gk::unitTests::testResultError();
	check(!res.isOk());
	check(res.isError());
}

test_case("Result Expect") {
	u32 ok = gk::unitTests::testResultOk().expect("lmfao"_str);
	check_eq(ok, 13);
}

test_case("Result Expect Error") {
	u32 err = gk::unitTests::testResultError().expectError("lol"_str);
	check_eq(err, 55);
}

#endif