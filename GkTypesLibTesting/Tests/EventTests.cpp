#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/BasicTypes.h"
#include "../../GkTypesLib/GkTypes/Event/Event.h"
#include "../GkTest.h"

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

};

static void EventAddToNumber(int* var, int add) {
	*var += add;
}

static float EventFuncMultiplyReturn(float a, float b) {
	return a * b;
}

namespace UnitTests 
{
	TEST(Event, NoReturnStandaloneFunction) {
		gk::Event<void, int*, int>* e = gk::Event<void, int*, int>::Create(EventAddToNumber);
		int* num = new int;
		*num = 5;
		e->Invoke(num, 10);
		EXPECT_EQ(*num, 15);
	}

	TEST(Event, ReturnEvent) {
		gk::Event<float, float, float>* e = gk::Event<float, float, float>::Create(EventFuncMultiplyReturn);
		EXPECT_EQ(e->Invoke(10, 10), 100);
	}

	TEST(Event, NoArgumentMemberFunction) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 24;
		gk::Event<void>* e = gk::Event<void>::Create(obj, &EventTestClass::IncrementNumIntByOne);
		e->Invoke();
		EXPECT_EQ(obj->numInt, 25);
	}

	TEST(Event, MemberFunctionOneArgument) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 25;
		gk::Event<void, int>* e = gk::Event<void, int>::Create(obj, &EventTestClass::IncrementNumInt);
		e->Invoke(5);
		EXPECT_EQ(obj->numInt, 30);
	}

	TEST(Event, MemberFunctionMultipleArguments) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 25;
		obj->numFlt = 10.5;
		gk::Event<void, int, float>* e = gk::Event<void, int, float>::Create(obj, &EventTestClass::IncrementBoth);
		e->Invoke(2, 0.5);
		EXPECT_EQ(obj->numInt, 27);
		EXPECT_EQ(obj->numFlt, 11);
	}

	TEST(Event, MemberFunctionReturn) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 10;
		obj->numFlt = 2;
		gk::Event<float, float>* e = gk::Event<float, float>::Create(obj, &EventTestClass::MultiplyAll);
		EXPECT_EQ(e->Invoke(2), 40);
	}

	TEST(Event, ConstMemberFunctionNoArgument) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 10;
		gk::Event<int>* e = gk::Event<int>::Create(obj, &EventTestClass::GetNumInt);
		EXPECT_EQ(e->Invoke(), 10);
	}

	TEST(Event, MemberFunctionGetObject) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 10;
		obj->numFlt = 2;
		gk::Event<float, float>* e = gk::Event<float, float>::Create(obj, &EventTestClass::MultiplyAll);
		EventTestClass* objRef = e->Obj<EventTestClass>();
		EXPECT_EQ(obj, objRef);
	}

	TEST(Event, ConstMemberFunctionGetObject) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 10;
		obj->numFlt = 2;
		gk::Event<float, float>* e = gk::Event<float, float>::Create(obj, &EventTestClass::MultiplyAllConst);
		EventTestClass* objRef = e->Obj<EventTestClass>();
		EXPECT_EQ(obj, objRef);
	}

}