#pragma once

#include "String.h"
#include "../Array/DynamicArray.h"
#include "../Hash/Hashmap.h"
#include "../Thread/Mutex.h"

namespace gk 
{
	/* Runtime thread-safe global immutable strings compressed into 4 byte integers. */
	struct GlobalString
	{
	private:

		struct GlobalStringContainers {
			gk::HashMap<gk::String, uint32> ids;
			gk::darray<gk::String> strings;
			uint32 nextId;
		};

		static gk::Mutex<GlobalStringContainers> initializeGlobalStringContainers() {
			GlobalStringContainers containers;
			containers.ids.insert(""_str, 0);
			containers.strings.Add(""_str);
			containers.nextId = 1;
			return gk::Mutex<GlobalStringContainers>(std::move(containers));
		}

		forceinline static gk::Mutex<GlobalStringContainers>& getAllGlobalStrings() {
			static gk::Mutex<GlobalStringContainers> globalStrings = initializeGlobalStringContainers();
			return globalStrings;
		}

	public:

		/* Default initializes to an empty string. */ 
		GlobalString()
			: stringId(0)
		{}
	
		/* Creates a new entry into the global string map if the specified string doesn't exist. 
		If it does exist, uses the pre-existing one. Will copy the string parameter.
		Setting the template parameter to ThreadSafety::Unsafe will not lock the global string mutex. */
		template<ThreadSafety safety = ThreadSafety::Safe>
		static GlobalString create(gk::String inString) {
			if constexpr (safety == ThreadSafety::Safe) {
				gk::LockedMutex<GlobalStringContainers> lock = getAllGlobalStrings().lock();
				GlobalStringContainers& containers = *lock;
				return createNewEntryIfDoesntExist(std::move(inString), containers);
			}
			// thread unsafe
			GlobalStringContainers& containers = getAllGlobalStrings().getDataNoLock();
			return createNewEntryIfDoesntExist(std::move(inString), containers);
		}

		/* Does not create a new entry. Will be empty string if the string doesn't already exist. 
		Setting the template parameter to ThreadSafety::Unsafe will not lock the global string mutex. */
		template<ThreadSafety safety = ThreadSafety::Safe>
		static GlobalString createIfExists(const gk::String& inString) {
			GlobalString gstrOut;
			if constexpr (safety == ThreadSafety::Safe) {
				gk::LockedMutex<GlobalStringContainers> lock = getAllGlobalStrings().lock();
				const GlobalStringContainers& containers = *lock;
				return createFromExistingEntryIfExist(inString, containers);
			}
			// thread unsafe
			const GlobalStringContainers& containers = getAllGlobalStrings().getDataNoLock();
			return createFromExistingEntryIfExist(inString, containers);
		}

		GlobalString(const GlobalString&) = default;
		GlobalString(GlobalString&&) = default;
		GlobalString& operator = (const GlobalString&) = default;
		GlobalString& operator = (GlobalString&&) = default;

		bool operator == (const GlobalString& other) const {
			return stringId == other.stringId;
		}

		/* Get an immutable reference to the global string referenced by this instance. 
		Setting the template parameter to ThreadSafety::Unsafe will not lock the global string mutex. */
		template<ThreadSafety safety = ThreadSafety::Safe>
		const gk::String& toString() const {
			if constexpr (safety == ThreadSafety::Safe) {
				gk::LockedMutex<GlobalStringContainers> lock = getAllGlobalStrings().lock();
				const GlobalStringContainers& containers = *lock;
				return toStringFromGlobalContainers(containers);
			}
			// thread unsafe
			const GlobalStringContainers& containers = getAllGlobalStrings().getDataNoLock();
			return toStringFromGlobalContainers(containers);
		}

		/* Calculate the hash value of this GlobalString using the pre-existing gk::hash<uint32>() specialization */
		forceinline size_t hash() const {
			return gk::hash(stringId);
		}
	
	private:

		static GlobalString createNewEntryIfDoesntExist(gk::String inString, GlobalStringContainers& containers) {
			GlobalString gstrOut;
			gk::Option<const uint32*> value = containers.ids.insert(inString, containers.nextId);

			if (value.none()) {
				gstrOut.stringId = containers.nextId;
				containers.strings.Add(std::move(inString));
				containers.nextId++;
				return gstrOut;
			}
			gstrOut.stringId = *value.some();
			return gstrOut;
		}

		static GlobalString createFromExistingEntryIfExist(const gk::String& inString, const GlobalStringContainers& containers) {
			GlobalString gstrOut;
			gk::Option<const uint32*> value = containers.ids.find(inString);

			if (value.none()) {
				return gstrOut;
			}
			gstrOut.stringId = *value.some();
			return gstrOut;
		}

		const gk::String& toStringFromGlobalContainers(const GlobalStringContainers& containers) const {
			gk_assertm(stringId < containers.strings.Size(), "GlobalString stringId is not valid. Is outside of the range of the global strings array");
			return containers.strings[stringId];
		}

	private:

		uint32 stringId;

	};

	template<>
	inline size_t hash<gk::GlobalString>(const gk::GlobalString& key) {
		return key.hash();
	}
}

namespace std
{
	template<>
	struct hash<gk::GlobalString>
	{
		size_t operator()(const gk::GlobalString& str) const {
			return str.hash();
		}

	};
}

