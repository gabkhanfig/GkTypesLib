#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/Job/JobSystem.h"
#include "../../GkTypesLib/GkTypes/String/String.h"
#include "../GkTest.h"

void randomJobFuncIncrement(gk::JobRunDataBuffer* buf) {
	int* ptr = buf->get<int*>(false);
	(*ptr)++;
}

namespace UnitTests
{
#pragma region Job_Run_Data_Buffer

	TEST(JobRunDataBuffer, DefaultCreation) {
		gk::JobRunDataBuffer b;
	}

	TEST(JobRunDataBuffer, StoreInt) {
		gk::JobRunDataBuffer b;
		b.store(257);
		int num = b.get<int>();
		EXPECT_EQ(num, 257);
	}

	TEST(JobRunDataBuffer, StoreFloat) {
		gk::JobRunDataBuffer b;
		b.store(1.5f);
		float num = b.get<float>();
		EXPECT_EQ(num, 1.5f);
	}

	TEST(JobRunDataBuffer, StoreChar) {
		gk::JobRunDataBuffer b;
		b.store('c');
		char c = b.get<char>();
		EXPECT_EQ(c, 'c');
	}

	TEST(JobRunDataBuffer, StoreSizeT) {
		gk::JobRunDataBuffer b;
		b.store(size_t(12345));
		size_t num = b.get<size_t>();
		EXPECT_EQ(num, 12345);
	}

	TEST(JobRunDataBuffer, StoreDouble) {
		gk::JobRunDataBuffer b;
		b.store(1.5);
		double num = b.get<double>();
		EXPECT_EQ(num, 1.5);
	}

	TEST(JobRunDataBuffer, StorePointer) {
		gk::JobRunDataBuffer b;
		int* ptr = new int;
		b.store(ptr, true);
		int* getPtr = b.get<int*>(false);
		EXPECT_EQ(getPtr, ptr);
	}

	TEST(JobRunDataBuffer, StorePointerNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		int* ptr = new int;
		b.store(ptr, true);
		int* getPtr = b.get<int*>(false);
		EXPECT_EQ(getPtr, ptr);
	}

	TEST(JobRunDataBuffer, StorePointerWithoutOwnership) {
		gk::JobRunDataBuffer b;
		int* ptr = new int;
		b.store(ptr, false);
		int* getPtr = b.get<int*>(true);
		EXPECT_EQ(getPtr, ptr);
		delete ptr;
	}

	TEST(JobRunDataBuffer, StorePointerWithoutOwnershipNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		int* ptr = new int;
		b.store(ptr, false);
		int* getPtr = b.get<int*>(true);
		EXPECT_EQ(getPtr, ptr);
		delete ptr;
	}

	TEST(JobRunDataBuffer, StoreStdVector) {
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(vec);
		std::vector<int>& vecRef = b.get<std::vector<int>>();
		EXPECT_EQ(vecRef[0], 1);
		EXPECT_EQ(vecRef[1], 2);
		EXPECT_EQ(vecRef[2], 3);
		EXPECT_EQ(vecRef[3], 4);
	}

	TEST(JobRunDataBuffer, StoreStdVectorNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(vec);
		std::vector<int>& vecRef = b.get<std::vector<int>>();
		EXPECT_EQ(vecRef[0], 1);
		EXPECT_EQ(vecRef[1], 2);
		EXPECT_EQ(vecRef[2], 3);
		EXPECT_EQ(vecRef[3], 4);
	}

	TEST(JobRunDataBuffer, StoreStdVectorGetCopy) {
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(vec);
		std::vector vecCopy = b.get<std::vector<int>>();
		EXPECT_EQ(vecCopy[0], 1);
		EXPECT_EQ(vecCopy[1], 2);
		EXPECT_EQ(vecCopy[2], 3);
		EXPECT_EQ(vecCopy[3], 4);
	}

	TEST(JobRunDataBuffer, StoreStdVectorGetCopyNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(vec);
		std::vector vecCopy = b.get<std::vector<int>>();
		EXPECT_EQ(vecCopy[0], 1);
		EXPECT_EQ(vecCopy[1], 2);
		EXPECT_EQ(vecCopy[2], 3);
		EXPECT_EQ(vecCopy[3], 4);
	}

	TEST(JobRunDataBuffer, StoreMoveStdVector) {
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(std::move(vec));
		std::vector<int>& vecRef = b.get<std::vector<int>>();
		EXPECT_EQ(vecRef[0], 1);
		EXPECT_EQ(vecRef[1], 2);
		EXPECT_EQ(vecRef[2], 3);
		EXPECT_EQ(vecRef[3], 4);
	}

	TEST(JobRunDataBuffer, StoreMoveStdVectorNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(std::move(vec));
		std::vector<int>& vecRef = b.get<std::vector<int>>();
		EXPECT_EQ(vecRef[0], 1);
		EXPECT_EQ(vecRef[1], 2);
		EXPECT_EQ(vecRef[2], 3);
		EXPECT_EQ(vecRef[3], 4);
	}

	TEST(JobRunDataBuffer, StoreMoveStdVectorGetCopy) {
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(std::move(vec));
		std::vector vecCopy = b.get<std::vector<int>>();
		EXPECT_EQ(vecCopy[0], 1);
		EXPECT_EQ(vecCopy[1], 2);
		EXPECT_EQ(vecCopy[2], 3);
		EXPECT_EQ(vecCopy[3], 4);
	}

	TEST(JobRunDataBuffer, StoreMoveStdVectorGetCopyNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		std::vector<int> vec {1, 2, 3, 4};
		b.store(std::move(vec));
		std::vector vecCopy = b.get<std::vector<int>>();
		EXPECT_EQ(vecCopy[0], 1);
		EXPECT_EQ(vecCopy[1], 2);
		EXPECT_EQ(vecCopy[2], 3);
		EXPECT_EQ(vecCopy[3], 4);
	}

	TEST(JobRunDataBuffer, StoreMaxSize) {
		gk::JobRunDataBuffer b;
		gk::String str = "ghupyiaswiphugyiasiagspuyhigaspyuii"_str;
		b.store(str);
		gk::String& strRef = b.get<gk::String>();
		EXPECT_EQ(strRef, "ghupyiaswiphugyiasiagspuyhigaspyuii"_str);
	}

	TEST(JobRunDataBuffer, StoreMaxSizeNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobRunDataBuffer b;
		gk::String str = "ghupyiaswiphugyiasiagspuyhigaspyuii"_str;
		b.store(str);
		gk::String& strRef = b.get<gk::String>();
		EXPECT_EQ(strRef, "ghupyiaswiphugyiasiagspuyhigaspyuii"_str);
	}

#pragma endregion

#pragma region Job_Data

	TEST(JobData, ExecuteJob) {
		gk::JobData job;
		job.jobFunc = randomJobFuncIncrement;
		int* ptr = new int;
		*ptr = 10;
		gk::JobRunDataBuffer buf;
		buf.store(ptr, false);
		job.data = std::move(buf);

		job.jobFunc.invoke(&job.data);
		EXPECT_EQ(*ptr, 11);
		delete ptr;
	}

	TEST(JobData, ExecuteJobNoMemoryLeak) {
		MemoryLeakDetector leakDetector;
		gk::JobData job;
		job.jobFunc = randomJobFuncIncrement;
		int* ptr = new int;
		*ptr = 10;
		gk::JobRunDataBuffer buf;
		buf.store(ptr, true);
		job.data = std::move(buf);

		job.jobFunc.invoke(&job.data);
	}

#pragma endregion



}
