#pragma once

#include "String.h"
#include <unordered_map>

namespace gk 
{
	/* Runtime global immutable strings. GlobalString internally only holds a pointer, so const-referencing is not necessary. */
	struct GlobalString
	{
		typedef std::unordered_map < gk::string, gk::string* > GlobalStringMapType;

	private:

		inline static GlobalStringMapType stringMap;

		gk::string* internalString;

	private:

		/* Will attempt to add a string to the map. If the string map already contains the passed in string, it will instead just get the contained string. */
		static gk::string* AddStringToMap(const gk::string& str) 
		{
			auto search = stringMap.find(str);
			if (search != stringMap.end()) {
				return search->second;
			}

			gk::string* mapStr = new gk::string(str);
			stringMap.insert({ str, mapStr });
			return mapStr;
		}

	public:

		/* Check if the map contains a specified string. */
		static bool DoesMapContainString(const gk::string& str) 
		{
			return stringMap.contains(str);
		}

		GlobalString()
		{
			internalString = AddStringToMap(gk::_emptyString);
		}

		GlobalString(const gk::GlobalString& other)
		{
			internalString = other.internalString;
		}

		GlobalString(const gk::string& str) 
		{
			internalString = AddStringToMap(str);
		}

		GlobalString(const char* str) 
		{
			internalString = AddStringToMap(str);
		}

		~GlobalString() 
		{

		}

		GlobalString& operator = (const gk::string& str) 
		{
			internalString = AddStringToMap(str);
			return *this;
		}

		GlobalString& operator = (GlobalString str) 
		{
			internalString = str.internalString;
			return *this;
		}

		bool operator == (const gk::string& str) 
		{
			auto search = stringMap.find(str);
			if (search == stringMap.end()) {
				return false;
			}

			return internalString == search->second;
		}

		bool operator == (GlobalString str) 
		{
			return internalString == str.internalString;
		}

		gk::string ToString() 
		{
			return *internalString;
		}

	};
}

