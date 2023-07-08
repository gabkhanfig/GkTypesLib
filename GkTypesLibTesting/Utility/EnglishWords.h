#pragma once
#include "../../GkTypesLib/GkTypes/Array/DynamicArray.h"
#include "../../GkTypesLib/GkTypes/String/String.h"
#include <vector>


class EnglishWords {

public:

	static gk::darray<gk::string> LoadAllEnglishWordsToStrings();

	static std::vector<gk::string> VectorLoadAllEnglishWordsToStrings();
};