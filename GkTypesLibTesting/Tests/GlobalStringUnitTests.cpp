#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/String/GlobalString.h"
#include "../../GkTypesLib/GkTypes/Array/DynamicArray.h"
#include "../../GkTypesLib/GkTypes/Job/JobInfo.h"
#include "../../GkTypesLib/GkTypes/Job/JobSystem.h"
#include "../GkTest.h"

void globalStringMultithreadFunc(gk::JobRunDataBuffer* buf) {
	gk::GlobalString str = gk::GlobalString::create(gk::String::from(buf->get<int>()));
}

namespace	UnitTests
{
	TEST(GlobalString, DefaultConstruct) {
		gk::GlobalString str;
		EXPECT_EQ(str.toString(), ""_str);
	}

	TEST(GlobalString, CreateCopy) {
		gk::String a = "hello world!"_str;
		gk::GlobalString str = gk::GlobalString::create(a);
		EXPECT_EQ(str.toString(), a);
	}

	TEST(GlobalString, CreateMove) {
		gk::GlobalString str = gk::GlobalString::create("hello world again!"_str);
		EXPECT_EQ(str.toString(), "hello world again!"_str);
	}

	TEST(GlobalString, CreateIfExists) {
		gk::String a = "hello world!"_str;
		gk::GlobalString str = gk::GlobalString::create(a);
		gk::GlobalString str2 = gk::GlobalString::createIfExists(a);
		EXPECT_EQ(str, str2);
	}

	TEST(GlobalString, CreateIfExistsDoesntExist) {
		gk::String a = "hello world!"_str;
		gk::GlobalString str = gk::GlobalString::create(a);
		gk::GlobalString str2 = gk::GlobalString::createIfExists("something else"_str);
		EXPECT_NE(str, str2);
		EXPECT_EQ(str2.toString(), ""_str);
	}

	TEST(GlobalString, Multithread) {
		gk::darray<gk::JobData> jobs;
		for (int i = 0; i < 500; i++) {
			gk::JobRunDataBuffer buf;
			buf.store<int>(i);
			gk::JobData data;
			data.jobFunc = globalStringMultithreadFunc;
			data.data = std::move(buf);
			jobs.Add(std::move(data));
		}

		gk::JobSystem::queueJobs(std::move(jobs));
		gk::JobSystem::executeQueue();
		gk::JobSystem::wait();

		for (int i = 0; i < 500; i++) {
			gk::GlobalString str = gk::GlobalString::createIfExists(gk::String::from(i));
			ASSERT_NE(str.toString(), ""_str);
			EXPECT_EQ(str.toString(), gk::String::from(i));
		}
	}

}