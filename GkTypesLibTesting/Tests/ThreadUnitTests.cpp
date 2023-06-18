#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Thread/Thread.h"

static void DoSomeWork() {
	int* number = new int;
	*number = 10;
	delete number;
}

void CheckThreadId(bool* out, uint32 id) {
	const std::thread::id threadId = std::this_thread::get_id();
	uint32 thisId = *(uint32*)&threadId;
	*out = thisId == id;
}

void AddOne(int* num) {
	*num = *num + 1;
}

void DoDelayedWork(int* num) {
 	Sleep(10);
	AddOne(num);
}

class TestThreadClass {
public:
	int a;
	void SomeFunc(int newVal) {
		a = newVal;
	}
	
};

namespace UnitTests {

	TEST(Thread, CreateAndDestroy) {
		gk::Thread* thread = new gk::Thread();
		delete thread;
	}

	TEST(Thread, CreateExecuteAndDestroy) {
		gk::Thread* thread = new gk::Thread();
		thread->BindFunction(DoSomeWork);
		thread->Execute();
		delete thread;
	}

	TEST(Thread, WaitForThread) {
		gk::Thread* thread = new gk::Thread();
		thread->BindFunction(DoSomeWork);
		thread->Execute();
		while (!thread->IsReady());
		delete thread;
	}

	TEST(Thread, ThreadId) {
		bool* boolean = new bool;
		*boolean = false;
		gk::Thread* thread = new gk::Thread();
		thread->BindFunction(std::bind(CheckThreadId, boolean, thread->GetThreadId()));
		thread->Execute();
		while (!thread->IsReady());
		ASSERT_TRUE(*boolean);
		delete thread;
		delete boolean;
	}

	TEST(Thread, DifferentThreadIds) {
		gk::Thread* thread1 = new gk::Thread();
		gk::Thread* thread2 = new gk::Thread();
		ASSERT_NE(thread1->GetThreadId(), thread2->GetThreadId());
		delete thread1;
		delete thread2;
	}

	TEST(Thread, MultipleBindsInOneExecution) {
		gk::Thread* thread = new gk::Thread();
		int* num1 = new int;
		*num1 = 10;
		int* num2 = new int;
		*num2 = 20;
		thread->BindFunction(std::bind(AddOne, num1));
		thread->BindFunction(std::bind(AddOne, num2));
		thread->Execute();
		while (!thread->IsReady());
		EXPECT_EQ(*num1, 11);
		EXPECT_EQ(*num2, 21);
		delete thread;
		delete num1;
		delete num2;
	}

	TEST(Thread, ExecuteClassMemberFunction) {
		gk::Thread* thread = new gk::Thread();
		TestThreadClass* obj = new TestThreadClass();
		obj->a = 5;
		EXPECT_EQ(obj->a, 5);
		thread->BindFunction(std::bind(&TestThreadClass::SomeFunc, obj, 10));
		EXPECT_EQ(obj->a, 5);
		thread->Execute();
		while (!thread->IsReady());
		EXPECT_EQ(obj->a, 10);
		delete obj;
		delete thread;
	}

	TEST(Thread, ExecuteDelayedFunctionWithDelete) {
		gk::Thread* thread = new gk::Thread();
		int* num1 = new int;
		*num1 = 10;
		thread->BindFunction(std::bind(DoDelayedWork, num1));
		EXPECT_EQ(*num1, 10);
		thread->Execute();
		delete thread;
		EXPECT_EQ(*num1, 11);
		delete num1;
	}


}