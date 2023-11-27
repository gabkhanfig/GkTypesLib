//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest_proxy.h"

int main(int argc, char** argv) {
	doctest::Context context;
	//context.setAsDefaultForAssertsOutOfTestCases();
	context.applyCommandLine(argc, argv);
	context.setOption("no-breaks", true);
	int res = context.run();
}