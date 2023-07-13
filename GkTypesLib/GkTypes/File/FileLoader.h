#pragma once

#include "../BasicTypes.h"
#include "../String/String.h"
#include <fstream>
#include <string>

namespace gk 
{
	class FileLoader 
	{
	public:

		/* Loads a file into a string. */
		static gk::String LoadFile(const gk::String& filePath)
		{
			std::ifstream in(filePath.CStr());
			if (!in.good()) {
				return gk::String();
			}
			std::string contents = std::string((std::istreambuf_iterator<char>(in)), {});

			return gk::String(contents.c_str());
		}

		/* Checks if a file exists at the specified path. Also works for folders! */
		static bool DoesFileExist(const gk::String& filePath)
		{
			struct stat buffer;
			return (stat(filePath.CStr(), &buffer) == 0);
		}

		/* Checks if a folder exists at the specified path. Also works for files! */
		static bool DoesFolderExist(const gk::String& folderPath) 
		{
			return DoesFileExist(folderPath);
		}
	};
}