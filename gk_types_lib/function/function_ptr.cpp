#include "function_ptr.h"
#include <utility>

#if GK_TYPES_LIB_TEST
namespace gk
{
	namespace unitTest
	{
		static void FuncReturnNothingNoArgs() {
			int a = 0;
		}

		static void FuncNoReturnManyArgsMultiply(int* num, int multiplier) {
			*num *= multiplier;
		}

		static double FuncReturnManyArgsAddAll(double a, double b, double c) {
			return a + b + c;
		}
	}
}

test_case("DefaultConstructionNotBound") {
	gk::Fptr<void> fptr;
	check(!fptr.isBound());
}

test_case("ConstructWithFunctionIsBound") {
	gk::Fptr<void> fptr = gk::unitTest::FuncReturnNothingNoArgs;
	check(fptr.isBound());
}

test_case("CopyConstructNotBound") {
	gk::Fptr<void> fptr;
	gk::Fptr<void> fptr2 = fptr;
	check(!fptr2.isBound());
}

test_case("CopyConstructBound") {
	gk::Fptr<void> fptr = gk::unitTest::FuncReturnNothingNoArgs;
	gk::Fptr<void> fptr2 = fptr;
	check(fptr2.isBound());
}

test_case("MoveConstructNotBound") {
	gk::Fptr<void> fptr;
	gk::Fptr<void> fptr2 = std::move(fptr);
	check(!fptr2.isBound());
}

test_case("MoveConstructBound") {
	gk::Fptr<void> fptr = gk::unitTest::FuncReturnNothingNoArgs;
	gk::Fptr<void> fptr2 = std::move(fptr);
	check(fptr2.isBound());
}

test_case("BindFunction") {
	gk::Fptr<void> fptr;
	fptr.bind(gk::unitTest::FuncReturnNothingNoArgs);
	check(fptr.isBound());
}

test_case("AssignFunctionAliasBind") {
	gk::Fptr<void> fptr;
	fptr = gk::unitTest::FuncReturnNothingNoArgs;
	check(fptr.isBound());
}

test_case("AssignCopyNotBound") {
	gk::Fptr<void> fptr;
	gk::Fptr<void> fptr2 = fptr;
	check(!fptr2.isBound());
}

test_case("AssignCopyBound") {
	gk::Fptr<void> fptr = gk::unitTest::FuncReturnNothingNoArgs;
	gk::Fptr<void> fptr2 = fptr;
	check(fptr2.isBound());
}

test_case("AssignMoveNotBound") {
	gk::Fptr<void> fptr;
	gk::Fptr<void> fptr2 = std::move(fptr);
	check(!fptr2.isBound());
}

test_case("AssignMoveBound") {
	gk::Fptr<void> fptr = gk::unitTest::FuncReturnNothingNoArgs;
	gk::Fptr<void> fptr2 = std::move(fptr);
	check(fptr2.isBound());
}

test_case("ExecuteNoReturnNoArgs") {
	gk::Fptr<void> fptr = gk::unitTest::FuncReturnNothingNoArgs;
	fptr.invoke();
}

test_case("ExecuteNoReturnManyArgs") {
	gk::Fptr<void, int*, int> fptr = gk::unitTest::FuncNoReturnManyArgsMultiply;
	int* a = new int;
	*a = 10;
	const int multiplier = 5;
	fptr.invoke(a, multiplier);
	check_eq(*a, 50);
	delete a;
}

test_case("ExecuteReturnManyArgs") {
	gk::Fptr<double, double, double, double> fptr = gk::unitTest::FuncReturnManyArgsAddAll;
	const double result = fptr.invoke(1.0, 2.0, 3.0);
	check_eq(result, 6.0);
}

#endif