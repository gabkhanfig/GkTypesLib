#pragma once

#include "String.h"
#include <unordered_map>

namespace gk 
{
	/* Runtime global immutable strings. GlobalString internally only holds a pointer, so const-referencing is not necessary. */
	struct GlobalString
	{
		typedef std::unordered_map < gk::String, gk::String* > GlobalStringMapType;

	private:

		inline static GlobalStringMapType stringMap;

		gk::String* internalString;

	private:

		/* Will attempt to add a string to the map. If the string map already contains the passed in string, it will instead just get the contained string. */
		static gk::String* AddStringToMap(const gk::String& str) 
		{
			auto search = stringMap.find(str);
			if (search != stringMap.end()) {
				return search->second;
			}
			gk::String* mapStr = new gk::String(str);
			stringMap.insert({ str, mapStr });
			return mapStr;
		}

	public:

		/* Check if the map contains a specified string. */
		static bool DoesMapContainString(const gk::String& str) 
		{
			return stringMap.contains(str);
		}

		GlobalString()
		{
			internalString = AddStringToMap("");
		}

		GlobalString(const gk::GlobalString& other)
		{
			internalString = other.internalString;
		}

		GlobalString(const gk::String& str) 
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

		GlobalString& operator = (const gk::String& str) 
		{
			internalString = AddStringToMap(str);
			return *this;
		}

		GlobalString& operator = (GlobalString str) 
		{
			internalString = str.internalString;
			return *this;
		}

		bool operator == (const char* str) const 
		{
			gk::String _str{ str };
			return *this == _str;
		}

		bool operator == (const gk::String& str) const
		{
			auto search = stringMap.find(str);
			if (search == stringMap.end()) {
				return false;
			}

			return internalString == search->second;
		}

		bool operator == (GlobalString str) const
		{
			return internalString == str.internalString;
		}

		gk::String ToString() const
		{
			return *internalString;
		}

		const char* CStr() const
		{
			return internalString->CStr();
		}

		size_t ComputeHash() const
		{
			return (size_t)internalString;
		}

		/* std::cout << GlobalString */
		friend std::ostream& operator << (std::ostream& os, const GlobalString& _string) {
			return os << _string.CStr();
		}

	};
}

namespace std
{
	template<>
	struct hash<gk::GlobalString>
	{
		size_t operator()(const gk::GlobalString& str) const {
			return str.ComputeHash();
		}

	};
}

