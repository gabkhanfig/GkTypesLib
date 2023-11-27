#include "callback.h"
#include <utility>

#if GK_TYPES_LIB_TEST
namespace gk
{
	namespace unitTests
	{
		class EventTestClass {
		public:

			float numFlt;
			int numInt;

			EventTestClass() : numFlt(0), numInt(0) {}

			void IncrementNumIntByOne() {
				numInt++;
			}

			void IncrementNumInt(int amount) {
				numInt += amount;
			}

			void IncrementBoth(int integerAmount, float floatAmount) {
				numInt += integerAmount;
				numFlt += floatAmount;
			}

			float MultiplyAll(float amount) {
				return static_cast<float>(numInt) * numFlt * amount;
			}

			float MultiplyAllConst(float amount) const {
				return static_cast<float>(numInt) * numFlt * amount;
			}

			int GetNumInt() const { return numInt; }
			float GetNumFlt() const { return numFlt; }

			virtual int VirtualFuncTest() {
				return 8;
			}

			virtual float VirtualFuncTestConst() const {
				return 1.5;
			}
		};

		class ChildEventTestClass : public EventTestClass {
		public:
			virtual int VirtualFuncTest() {
				return 16;
			}
			virtual float VirtualFuncTestConst() const {
				return 3.5;
			}
		};

		static void EventAddToNumber(int* var, int add) {
			*var += add;
		}

		static float EventFuncMultiplyReturn(float a, float b) {
			return a * b;
		}
	}
}

using gk::unitTests::EventTestClass;
using gk::unitTests::ChildEventTestClass;

test_case("NoReturnStandaloneFunction") {
	gk::Callback<void, int*, int> e = gk::Callback<void, int*, int>(gk::unitTests::EventAddToNumber);
	int* num = new int;
	*num = 5;
	e.invoke(num, 10);
	check_eq(*num, 15);
}

test_case("ReturnEvent") {
	gk::Callback<float, float, float> e = gk::Callback<float, float, float>(gk::unitTests::EventFuncMultiplyReturn);
	check_eq(e.invoke(10, 10), 100);
}

test_case("NoArgumentMemberFunction") {
	EventTestClass* obj = new EventTestClass();
	obj->numInt = 24;
	gk::Callback<void> e = gk::Callback<void>(obj, &EventTestClass::IncrementNumIntByOne);
	e.invoke();
	check_eq(obj->numInt, 25);
	delete obj;
}

test_case("MemberFunctionOneArgument") {
	EventTestClass* obj = new EventTestClass();
	obj->numInt = 25;
	gk::Callback<void, int> e = gk::Callback<void, int>(obj, &EventTestClass::IncrementNumInt);
	e.invoke(5);
	check_eq(obj->numInt, 30);
	delete obj;
}

test_case("MemberFunctionMultipleArguments") {
	EventTestClass* obj = new EventTestClass();
	obj->numInt = 25;
	obj->numFlt = 10.5;
	gk::Callback<void, int, float> e = gk::Callback<void, int, float>(obj, &EventTestClass::IncrementBoth);
	e.invoke(2, 0.5);
	check_eq(obj->numInt, 27);
	check_eq(obj->numFlt, 11);
	delete obj;
}

test_case("MemberFunctionReturn") {
	EventTestClass* obj = new EventTestClass();
	obj->numInt = 10;
	obj->numFlt = 2;
	gk::Callback<float, float> e = gk::Callback<float, float>(obj, &EventTestClass::MultiplyAll);
	check_eq(e.invoke(2), 40);
	delete obj;
}

test_case("ConstMemberFunctionNoArgument") {
	EventTestClass* obj = new EventTestClass();
	obj->numInt = 10;
	gk::Callback<int> e = gk::Callback<int>(obj, &EventTestClass::GetNumInt);
	check_eq(e.invoke(), 10);
	delete obj;
}

test_case("VirtualMemberFunction") {
	EventTestClass* obj = new EventTestClass();
	gk::Callback<int> e = gk::Callback<int>(obj, &EventTestClass::VirtualFuncTest);
	check_eq(e.invoke(), 8);
	delete obj;
}

test_case("VirtualMemberFunctionChild") {
	EventTestClass* obj = new ChildEventTestClass();
	gk::Callback<int> e = gk::Callback<int>(obj, &ChildEventTestClass::VirtualFuncTest);
	check_eq(e.invoke(), 16);
	delete obj;
}

test_case("VirtualMemberFunctionConst") {
	EventTestClass* obj = new EventTestClass();
	gk::Callback<float> e = gk::Callback<float>(obj, &EventTestClass::VirtualFuncTestConst);
	check_eq(e.invoke(), 1.5);
	delete obj;
}

test_case("VirtualMemberFunctionChildConst") {
	const EventTestClass* obj = new ChildEventTestClass();
	gk::Callback<float> e = gk::Callback<float>(obj, &ChildEventTestClass::VirtualFuncTestConst);
	check_eq(e.invoke(), 3.5);
	delete obj;
}

test_case("VirtualMemberFunctionUsesCorrectVtable") {
	EventTestClass* obj = new EventTestClass();
	gk::Callback<int> e = gk::Callback<int>(obj, &ChildEventTestClass::VirtualFuncTest);
	check_eq(e.invoke(), 8);
	delete obj;
}

test_case("FreeFunctionNoObject") {
	gk::Callback<void, int*, int> e = gk::Callback<void, int*, int>(gk::unitTests::EventAddToNumber);
	int* num = new int;
	*num = 5;
	EventTestClass* obj = new EventTestClass();
	check_not(e.isObject(obj));
	delete obj;
}

test_case("MemberFunctionIsObject") {
	EventTestClass* obj = new EventTestClass();
	gk::Callback<void, int> e = gk::Callback<void, int>(obj, &EventTestClass::IncrementNumInt);
	check(e.isObject(obj));
}

test_case("MemberFunctionIsNotObject") {
	EventTestClass* obj = new EventTestClass();
	EventTestClass* obj2 = new EventTestClass();
	gk::Callback<void, int> e = gk::Callback<void, int>(obj, &EventTestClass::IncrementNumInt);
	check_not(e.isObject(obj2));
}

#endif