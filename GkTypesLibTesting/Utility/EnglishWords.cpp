#include "EnglishWords.h"
#include "../../GkTypesLib/GkTypes/File/FileLoader.h"

gk::darray<gk::String> EnglishWords::LoadAllEnglishWordsToStrings()
{
	const auto wordsPath = "C:/Users/Admin/Documents/Code/GkTypesLib/GkTypesLibTesting/Resources/words.txt"_str;
	const gk::String wordsString = gk::FileLoader::LoadFile(wordsPath);
	gk::darray<gk::String> words;
	words.Reserve(467000);

	const char* str = wordsString.cstr();
	const char* start = str;
	const char* end = start;
	for (size_t i = 0; i < wordsString.len(); i++) {
		char c = str[i];
		if (c == '\n') {
			end = &str[i - 1];
			words.Add(wordsString.substring((uint64)(start - str), (uint64)(end - str)));
			start = &str[i + 1];
		}
	}

	return words;
}


