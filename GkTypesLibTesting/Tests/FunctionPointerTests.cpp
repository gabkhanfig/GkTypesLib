#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/Function/Fptr.h"
#include "../GkTest.h"

static void FuncReturnNothingNoArgs() {
	int a = 0;
}

static void FuncNoReturnManyArgsMultiply(int* num, int multiplier) {
	*num *= multiplier;
}

static double FuncReturnManyArgsAddAll(double a, double b, double c) {
	return a + b + c;
}

namespace UnitTests 
{
	TEST(FunctionPointer, DefaultConstructionNotBound) {
		gk::Fptr<void> fptr;
		EXPECT_FALSE(fptr.isBound());
	}

	TEST(FunctionPointer, ConstructWithFunctionIsBound) {
		gk::Fptr<void> fptr = FuncReturnNothingNoArgs;
		EXPECT_TRUE(fptr.isBound());
	}

	TEST(FunctionPointer, CopyConstructNotBound) {
		gk::Fptr<void> fptr;
		gk::Fptr<void> fptr2 = fptr;
		EXPECT_FALSE(fptr2.isBound());
	}

	TEST(FunctionPointer, CopyConstructBound) {
		gk::Fptr<void> fptr = FuncReturnNothingNoArgs;
		gk::Fptr<void> fptr2 = fptr;
		EXPECT_TRUE(fptr2.isBound());
	}

	TEST(FunctionPointer, MoveConstructNotBound) {
		gk::Fptr<void> fptr;
		gk::Fptr<void> fptr2 = std::move(fptr);
		EXPECT_FALSE(fptr2.isBound());
	}

	TEST(FunctionPointer, MoveConstructBound) {
		gk::Fptr<void> fptr = FuncReturnNothingNoArgs;
		gk::Fptr<void> fptr2 = std::move(fptr);
		EXPECT_TRUE(fptr2.isBound());
	}

	TEST(FunctionPointer, BindFunction) {
		gk::Fptr<void> fptr;
		fptr.bind(FuncReturnNothingNoArgs);
		EXPECT_TRUE(fptr.isBound());
	}

	TEST(FunctionPointer, AssignFunctionAliasBind) {
		gk::Fptr<void> fptr;
		fptr = FuncReturnNothingNoArgs;
		EXPECT_TRUE(fptr.isBound());
	}

	TEST(FunctionPointer, AssignCopyNotBound) {
		gk::Fptr<void> fptr;
		gk::Fptr<void> fptr2 = fptr;
		EXPECT_FALSE(fptr2.isBound());
	}

	TEST(FunctionPointer, AssignCopyBound) {
		gk::Fptr<void> fptr = FuncReturnNothingNoArgs;
		gk::Fptr<void> fptr2 = fptr;
		EXPECT_TRUE(fptr2.isBound());
	}

	TEST(FunctionPointer, AssignMoveNotBound) {
		gk::Fptr<void> fptr;
		gk::Fptr<void> fptr2 = std::move(fptr);
		EXPECT_FALSE(fptr2.isBound());
	}

	TEST(FunctionPointer, AssignMoveBound) {
		gk::Fptr<void> fptr = FuncReturnNothingNoArgs;
		gk::Fptr<void> fptr2 = std::move(fptr);
		EXPECT_TRUE(fptr2.isBound());
	}

	TEST(FunctionPointer, ExecuteNoReturnNoArgs) {
		gk::Fptr<void> fptr = FuncReturnNothingNoArgs;
		fptr.invoke();
	}

	TEST(FunctionPointer, ExecuteNoReturnManyArgs) {
		gk::Fptr<void, int*, int> fptr = FuncNoReturnManyArgsMultiply;
		int* a = new int;
		*a = 10;
		const int multiplier = 5;
		fptr.invoke(a, multiplier);
		EXPECT_EQ(*a, 50);
		delete a;
	}

	TEST(FunctionPointer, ExecuteReturnManyArgs) {
		gk::Fptr<double, double, double, double> fptr = FuncReturnManyArgsAddAll;
		const double result = fptr.invoke(1.0, 2.0, 3.0);
		EXPECT_EQ(result, 6.0);
	}
}