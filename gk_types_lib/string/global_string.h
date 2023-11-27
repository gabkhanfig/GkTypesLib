#pragma once

#include "../basic_types.h"
#include "../hash/hash.h"

namespace gk 
{
	struct String;

	/**
	* Runtime thread-safe global immutable strings compressed into 4 byte integers.
	* Comparisons are extremely fast, because it is just an int comparison.
	* Construction can be slow due to thread safety, so storing these as constructed once global
	* is very useful.
	*/
	struct GlobalString 
	{
		/** 
		* Default initializes to an empty string. 
		*/ 
		GlobalString()
			: stringId(0)
		{}

		GlobalString(const GlobalString&) = default;
		GlobalString(GlobalString&&) = default;
		GlobalString& operator = (const GlobalString&) = default;
		GlobalString& operator = (GlobalString&&) = default;
		~GlobalString() = default;

		/**
		* If the string already exists in the map, uses that id, or creates a new entry.
		* Is thread safe.
		* Will take write ownership of the RwLock.
		* If a brand new entry is not necessary, prefer `createIfExists()` as it only reads from the RwLock
		* 
		* @param inString: String to potentially add by move.
		*/
		static GlobalString create(struct gk::String&& inString);

		/**
		* If the string already exists in the map, uses that id, or creates a new entry.
		* Is thread safe.
		* Will take write ownership of the RwLock.
		* If a brand new entry is not necessary, prefer `createIfExists()` as it only reads from the RwLock
		*
		* @param inString: String to potentially add by copy.
		*/
		static GlobalString create(const gk::String& inString);
		
		/**
		* If the string already exists in the map, uses that id.
		* If it doesn't exist, the GlobalString will be empty string, or id 0.
		* Will read the RwLock.
		* 
		* @param inString: String to check if exists.
		*/
		static GlobalString createIfExists(const gk::String& inString);

		/**
		* Creates a new GlobalString using a string id.
		* It is the programmers responsibility to ensure that the string id is valid.
		* The `stringId` validity can be checked with `isStringIdValid()`.
		* 
		* @param stringId: String id to construct a new GlobalString with.
		*/
		inline static GlobalString unsafeFromId(u32 stringId) {
			GlobalString out;
			out.stringId = stringId;
			return out;
		}

		/**
		* Get the internal string id used by this GlobalString. Can be useful for SIMD.
		*/
		u32 getId() const { return stringId; }

		/**
		* Compare two GlobalString's. Is essentially just comparing two integers.
		*/
		bool operator == (const GlobalString& other) const {
			return stringId == other.stringId;
		}

		/**
		* Create a copy of the string referenced by this GlobalString.
		*/
		gk::String toString() const;

		/**
		* Calculate the hash value of this GlobalString using the pre-existing gk::hash<u32>() specialization 
		*/
		size_t hash() const;

		/**
		* Check if a string exists within the global map.
		* This function has a relatively high cost due to thread safety, and thus should only be used when necessary.
		*/
		static bool doesStringExistInGlobalMap(const String& string);

		/**
		* Check if an id is a valid string is within the global map.
		* This function has a relatively high cost due to thread safety, and thus should only be used when necessary.
		*/
		static bool isStringIdValid(u32 id);

	private:

		u32 stringId;

	};

	template<>
	inline size_t hash<gk::GlobalString>(const gk::GlobalString& key) {
		return key.hash();
	}
}

//namespace std
//{
//	template<>
//	struct hash<gk::GlobalString>
//	{
//		size_t operator()(const gk::GlobalString& str) const {
//			return str.hash();
//		}
//
//	};
//}

