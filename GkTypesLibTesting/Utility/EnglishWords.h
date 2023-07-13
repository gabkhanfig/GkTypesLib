#pragma once
#include "../../GkTypesLib/GkTypes/Array/DynamicArray.h"
#include "../../GkTypesLib/GkTypes/String/String.h"
#include <vector>


class EnglishWords {

public:

	static gk::darray<gk::String> LoadAllEnglishWordsToStrings();

	static std::vector<gk::String> VectorLoadAllEnglishWordsToStrings();
};