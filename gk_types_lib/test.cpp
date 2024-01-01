//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest_proxy.h"
#include "string/string.h"
#include <string>
#include <vector>

namespace gk {
	static std::vector<gk::String> loadEnglishWords() {
		std::vector<gk::String> balls;
		balls.reserve(480000);

		std::ifstream t(GK_TYPES_LIB_LOCAL_PATH "words.txt");
		const std::string str((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());

		size_t pos = 0;
		size_t prev = 0;
		while ((pos = str.find('\n', prev)) != std::string::npos) {
			std::string sub = str.substr(prev, pos - prev);
			prev = pos + 1;

			gk::Str slice = gk::Str::fromNullTerminated(sub.c_str());
			gk::String word = slice;
			balls.push_back(word);
		}
		return balls;
	}

	static void runTests(int argc, char** argv) {
		doctest::Context context;
		context.applyCommandLine(argc, argv);
		context.setOption("no-breaks", true);
		int res = context.run();
	}
}

gk::Str j = "{\"field\": []}"_str;

int main(int argc, char** argv) {
	gk::runTests(argc, argv);
}