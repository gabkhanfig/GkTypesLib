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

namespace UnitTests 
{
	TEST(Event, NoReturnStandaloneFunction) {
		gk::Event<void, int*, int> e = gk::Event<void, int*, int>(EventAddToNumber);
		int* num = new int;
		*num = 5;
		e.invoke(num, 10);
		EXPECT_EQ(*num, 15);
	}

	TEST(Event, ReturnEvent) {
		gk::Event<float, float, float> e = gk::Event<float, float, float>(EventFuncMultiplyReturn);
		EXPECT_EQ(e.invoke(10, 10), 100);
	}

	TEST(Event, NoArgumentMemberFunction) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 24;
		gk::Event<void> e = gk::Event<void>(obj, &EventTestClass::IncrementNumIntByOne);
		e.invoke();
		EXPECT_EQ(obj->numInt, 25);
		delete obj;
	}

	TEST(Event, MemberFunctionOneArgument) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 25;
		gk::Event<void, int> e = gk::Event<void, int>(obj, &EventTestClass::IncrementNumInt);
		e.invoke(5);
		EXPECT_EQ(obj->numInt, 30);
		delete obj;
	}

	TEST(Event, MemberFunctionMultipleArguments) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 25;
		obj->numFlt = 10.5;
		gk::Event<void, int, float> e = gk::Event<void, int, float>(obj, &EventTestClass::IncrementBoth);
		e.invoke(2, 0.5);
		EXPECT_EQ(obj->numInt, 27);
		EXPECT_EQ(obj->numFlt, 11);
		delete obj;
	}

	TEST(Event, MemberFunctionReturn) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 10;
		obj->numFlt = 2;
		gk::Event<float, float> e = gk::Event<float, float>(obj, &EventTestClass::MultiplyAll);
		EXPECT_EQ(e.invoke(2), 40);
		delete obj;
	}

	TEST(Event, ConstMemberFunctionNoArgument) {
		EventTestClass* obj = new EventTestClass();
		obj->numInt = 10;
		gk::Event<int> e = gk::Event<int>(obj, &EventTestClass::GetNumInt);
		EXPECT_EQ(e.invoke(), 10);
		delete obj;
	}

	TEST(Event, VirtualMemberFunction) {
		EventTestClass* obj = new EventTestClass();
		gk::Event<int> e = gk::Event<int>(obj, &EventTestClass::VirtualFuncTest);
		EXPECT_EQ(e.invoke(), 8);
		delete obj;
	}

	TEST(Event, VirtualMemberFunctionChild) {
		EventTestClass* obj = new ChildEventTestClass();
		gk::Event<int> e = gk::Event<int>(obj, &ChildEventTestClass::VirtualFuncTest);
		EXPECT_EQ(e.invoke(), 16);
		delete obj;
	}

	TEST(Event, VirtualMemberFunctionConst) {
		EventTestClass* obj = new EventTestClass();
		gk::Event<float> e = gk::Event<float>(obj, &EventTestClass::VirtualFuncTestConst);
		EXPECT_EQ(e.invoke(), 1.5);
		delete obj;
	}

	TEST(Event, VirtualMemberFunctionChildConst) {
		const EventTestClass* obj = new ChildEventTestClass();
		gk::Event<float> e = gk::Event<float>(obj, &ChildEventTestClass::VirtualFuncTestConst);
		EXPECT_EQ(e.invoke(), 3.5);
		delete obj;
	}

	TEST(Event, VirtualMemberFunctionUsesCorrectVtable) {
		EventTestClass* obj = new EventTestClass();
		gk::Event<int> e = gk::Event<int>(obj, &ChildEventTestClass::VirtualFuncTest);
		EXPECT_EQ(e.invoke(), 8);
		delete obj;
	}

	TEST(Event, FreeFunctionNoObject) {
		gk::Event<void, int*, int> e = gk::Event<void, int*, int>(EventAddToNumber);
		int* num = new int;
		*num = 5;
		EventTestClass* obj = new EventTestClass();
		EXPECT_FALSE(e.isObject(obj));
		delete obj;
	}

	TEST(Event, MemberFunctionIsObject) {
		EventTestClass* obj = new EventTestClass();
		gk::Event<void, int> e = gk::Event<void, int>(obj, &EventTestClass::IncrementNumInt);
		EXPECT_TRUE(e.isObject(obj));
	}

	TEST(Event, MemberFunctionIsNotObject) {
		EventTestClass* obj = new EventTestClass();
		EventTestClass* obj2 = new EventTestClass();
		gk::Event<void, int> e = gk::Event<void, int>(obj, &EventTestClass::IncrementNumInt);
		EXPECT_FALSE(e.isObject(obj2));
	}
}