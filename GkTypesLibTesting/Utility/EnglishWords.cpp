#include "EnglishWords.h"
#include "../../GkTypesLib/GkTypes/File/FileLoader.h"

gk::darray<gk::String> EnglishWords::LoadAllEnglishWordsToStrings()
{
	const char* wordsPath = "C:/Users/Admin/Documents/Code/GkTypesLib/GkTypesLibTesting/Resources/words.txt";
	const gk::String wordsString = gk::FileLoader::LoadFile(wordsPath);
	gk::darray<gk::String> words;
	words.Reserve(467000);

	const char* str = wordsString.CStr();
	const char* start = str;
	const char* end = start;
	for (size_t i = 0; i < wordsString.Len(); i++) {
		char c = str[i];
		if (c == '\n') {
			end = &str[i - 1];
			words.Add(gk::String(start, end));
			start = &str[i + 1];
		}
	}

	return words;
}

std::vector<gk::String> EnglishWords::VectorLoadAllEnglishWordsToStrings()
{
	const char* wordsPath = "C:/Users/Admin/Documents/Code/GkTypesLib/GkTypesLibTesting/Resources/words.txt";
	const gk::String wordsString = gk::FileLoader::LoadFile(wordsPath);
	std::vector<gk::String> words;
	words.reserve(467000);

	const char* str = wordsString.CStr();
	const char* start = str;
	const char* end = start;
	for (size_t i = 0; i < wordsString.Len(); i++) {
		char c = str[i];
		if (c == '\n') {
			end = &str[i - 1];
			words.push_back(gk::String(start, end));
			start = &str[i + 1];
		}
	}

	return words;
}


