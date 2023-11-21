#include "../pch.h"
#include <Windows.h>
#include "../../GkTypesLib/GkTypes/String/GlobalString.h"
#include "../../GkTypesLib/GkTypes/Array/DynamicArray.h"
#include "../../GkTypesLib/GkTypes/Job/JobSystem.h"
#include "../GkTest.h"
using gk::JobSystem;

//void globalStringMultithreadFunc(gk::JobRunDataBuffer* buf) {
//	gk::GlobalString str = gk::GlobalString::create(gk::String::from(buf->get<int>()));
//}

static void multithreadAddGlobalString(int num) {
	gk::GlobalString str = gk::GlobalString::create(gk::String::from(num));
	ASSERT_NE(str.toString(), ""_str);
	EXPECT_EQ(str.toString(), gk::String::from(num));
}

static void multithreadIfExistsGlobalString(int num) {
	gk::GlobalString str = gk::GlobalString::createIfExists(gk::String::from(num));
	ASSERT_NE(str.toString(), ""_str);
	EXPECT_EQ(str.toString(), gk::String::from(num));
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

	TEST(GlobalString, MultithreadCreate) {
		JobSystem* jobSystem = new JobSystem(8);

		for (int i = 0; i < 500; i++) {
			jobSystem->runJob(multithreadAddGlobalString, (int)i);
		}
		delete jobSystem;
	}

	TEST(GlobalString, MultithreadCreateIfExists) {
		JobSystem* jobSystem = new JobSystem(8);
		for (int i = 0; i < 500; i++) {
			gk::GlobalString::create(gk::String::from(i));
		}
		for (int i = 0; i < 500; i++) {
			jobSystem->runJob(multithreadIfExistsGlobalString, (int)i);
		}
		delete jobSystem;
	}

}