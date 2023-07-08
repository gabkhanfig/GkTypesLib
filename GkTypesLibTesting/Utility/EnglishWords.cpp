#include "EnglishWords.h"
#include "../../GkTypesLib/GkTypes/File/FileLoader.h"

gk::darray<gk::string> EnglishWords::LoadAllEnglishWordsToStrings()
{
	const char* wordsPath = "C:/Users/Admin/Documents/Code/GkTypesLib/GkTypesLibTesting/Resources/words.txt";
	const gk::string wordsString = gk::FileLoader::LoadFile(wordsPath);
	gk::darray<gk::string> words;
	words.Reserve(467000);

	const char* str = wordsString.CStr();
	const char* start = str;
	const char* end = start;
	for (size_t i = 0; i < wordsString.Len(); i++) {
		char c = str[i];
		if (c == '\n') {
			end = &str[i - 1];
			words.Add(gk::string(start, end));
			start = &str[i + 1];
		}
	}

	return words;
}

std::vector<gk::string> EnglishWords::VectorLoadAllEnglishWordsToStrings()
{
	const char* wordsPath = "C:/Users/Admin/Documents/Code/GkTypesLib/GkTypesLibTesting/Resources/words.txt";
	const gk::string wordsString = gk::FileLoader::LoadFile(wordsPath);
	std::vector<gk::string> words;
	words.reserve(467000);

	const char* str = wordsString.CStr();
	const char* start = str;
	const char* end = start;
	for (size_t i = 0; i < wordsString.Len(); i++) {
		char c = str[i];
		if (c == '\n') {
			end = &str[i - 1];
			words.push_back(gk::string(start, end));
			start = &str[i + 1];
		}
	}

	return words;
}


