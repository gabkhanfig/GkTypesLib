#include "../pch.h"
#include "../../GkTypesLib/GkTypes/Thread/Thread.h"

void DoSomeWork() {
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
		ASSERT_EQ(*num1, 11);
		ASSERT_EQ(*num2, 21);
		delete thread;
	}
}