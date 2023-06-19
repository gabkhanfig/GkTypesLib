#pragma once
#include "../../GkTypesLib/GkTypes/Array/DynamicArray.h"
#include "../../GkTypesLib/GkTypes/String/String.h"


class EnglishWords {

public:

	static gk::darray<gk::string> LoadAllEnglishWordsToStrings();
};