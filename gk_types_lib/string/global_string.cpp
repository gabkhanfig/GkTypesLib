#include "global_string.h"
#include "string.h"
#include "../array/array_list.h"
#include "../hash/hashmap.h"
#include "../sync/rw_lock.h"


using gk::GlobalString;
using gk::u32;


namespace gk
{
	struct GlobalStringContainers {
		gk::HashMap<gk::String, u32> ids;
		gk::ArrayList<gk::String> strings;
		u32 nextId;

		GlobalStringContainers() : nextId(0) {}
	};

	static gk::RwLock<GlobalStringContainers>* initializeGlobalStringContainers() {
		GlobalStringContainers containers;
		containers.ids.insert(""_str, 0);
		containers.strings.push(""_str);
		containers.nextId = 1;
		return new gk::RwLock<GlobalStringContainers>(std::move(containers));
	}

	static GlobalString createNewEntryIfDoesntExist(gk::String&& inString, GlobalStringContainers& containers) // copy and move
	{
		gk::Option<u32*> value = containers.ids.insert(gk::String(inString), containers.nextId);

		if (value.none()) {
			GlobalString gstrOut = GlobalString::unsafeFromId(containers.nextId);
			containers.strings.push(std::move(inString));
			containers.nextId++;
			return gstrOut;
		}
		return GlobalString::unsafeFromId(*value.some());
	}

	static GlobalString createNewEntryIfDoesntExist(const gk::String& inString, GlobalStringContainers& containers) // copy
	{
		gk::Option<u32*> value = containers.ids.insert(inString, containers.nextId);

		if (value.none()) {
			GlobalString gstrOut = GlobalString::unsafeFromId(containers.nextId);
			containers.strings.push(inString);
			containers.nextId++;
			return gstrOut;
		}
		return GlobalString::unsafeFromId(*value.some());
	}

	static GlobalString createFromExistingEntryIfExist(const gk::String& inString, const GlobalStringContainers& containers)
	{
		gk::Option<const u32*> value = containers.ids.find(inString);

		if (value.none()) {
			return GlobalString();
		}
		return GlobalString::unsafeFromId(*value.some());
	}

	static gk::String toStringFromGlobalContainers(u32 stringId, const GlobalStringContainers& containers) {
		check_message(stringId < containers.strings.len(), "GlobalString stringId is not valid. Is outside of the range of the global strings array");
		return containers.strings[stringId];
	}

	forceinline static gk::RwLock<GlobalStringContainers>& getAllGlobalStrings() {
		static gk::RwLock<GlobalStringContainers>* globalStrings = initializeGlobalStringContainers();
		return *globalStrings;
	}
}


GlobalString gk::GlobalString::create(gk::String&& inString)
{
	gk::LockedWriter<GlobalStringContainers> lock = getAllGlobalStrings().write();
	GlobalStringContainers& containers = *lock.get();
	return createNewEntryIfDoesntExist(std::move(inString), containers);
}

GlobalString gk::GlobalString::create(const gk::String& inString)
{
	gk::LockedWriter<GlobalStringContainers> lock = getAllGlobalStrings().write();
	GlobalStringContainers& containers = *lock.get();
	return createNewEntryIfDoesntExist(inString, containers);
}

GlobalString gk::GlobalString::createIfExists(const gk::String& inString)
{
	gk::LockedReader<GlobalStringContainers> lock = getAllGlobalStrings().read();
	const GlobalStringContainers& containers = *lock.get();
	return createFromExistingEntryIfExist(inString, containers);
}

gk::String gk::GlobalString::toString() const
{
	gk::LockedReader<GlobalStringContainers> lock = getAllGlobalStrings().read();
	const GlobalStringContainers& containers = *lock.get();
	return toStringFromGlobalContainers(stringId, containers);
}

size_t gk::GlobalString::hash() const
{
	return gk::hash(this->stringId);
}

bool gk::GlobalString::doesStringExistInGlobalMap(const String& string)
{
	gk::LockedReader<GlobalStringContainers> lock = getAllGlobalStrings().read();
	const GlobalStringContainers& containers = *lock.get();
	return !containers.ids.find(string).none();
}

bool gk::GlobalString::isStringIdValid(u32 id)
{
	gk::LockedReader<GlobalStringContainers> lock = getAllGlobalStrings().read();
	const GlobalStringContainers& containers = *lock.get();
	return id < containers.strings.len();
}


#if GK_TYPES_LIB_TEST
#include "../job/job_system.h"

using gk::JobSystem;

namespace gk
{
	namespace unitTests
	{
		static void multithreadAddGlobalString(int num) {
			gk::GlobalString str = gk::GlobalString::create(gk::String::from(num));
			check_ne(str.toString(), ""_str);
			check_eq(str.toString(), gk::String::from(num));
		}

		static void multithreadIfExistsGlobalString(int num) {
			gk::GlobalString str = gk::GlobalString::createIfExists(gk::String::from(num));
			check_ne(str.toString(), ""_str);
			check_eq(str.toString(), gk::String::from(num));
		}
	}
}

test_case("DefaultConstruct") {
	gk::GlobalString str;
	check_eq(str.toString(), ""_str);
}

test_case("CreateCopy") {
	gk::String a = "hello world!"_str;
	gk::GlobalString str = gk::GlobalString::create(a);
	check_eq(str.toString(), a);
}

test_case("CreateMove") {
	gk::GlobalString str = gk::GlobalString::create("hello world again!"_str);
	check_eq(str.toString(), "hello world again!"_str);
}

test_case("CreateIfExists") {
	gk::String a = "hello world!"_str;
	gk::GlobalString str = gk::GlobalString::create(a);
	gk::GlobalString str2 = gk::GlobalString::createIfExists(a);
	check_eq(str, str2);
}

test_case("CreateIfExistsDoesntExist") {
	gk::String a = "hello world!"_str;
	gk::GlobalString str = gk::GlobalString::create(a);
	gk::GlobalString str2 = gk::GlobalString::createIfExists("something else"_str);
	check_ne(str, str2);
	check_eq(str2.toString(), ""_str);
}

test_case("MultithreadCreate") {
	JobSystem* jobSystem = new JobSystem(8);

	for (int i = 0; i < 500; i++) {
		jobSystem->runJob(gk::unitTests::multithreadAddGlobalString, (int)i);
	}
	delete jobSystem;
}

test_case("MultithreadCreateIfExists") {
	JobSystem* jobSystem = new JobSystem(8);
	for (int i = 0; i < 500; i++) {
		gk::GlobalString::create(gk::String::from(i));
	}
	for (int i = 0; i < 500; i++) {
		jobSystem->runJob(gk::unitTests::multithreadIfExistsGlobalString, (int)i);
	}
	delete jobSystem;
}

#endif