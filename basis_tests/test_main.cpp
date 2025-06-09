#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../compiler.h"

TEST_CASE("main read options") {
    CompileOptions options;
    std::vector<std::string> argv_good{"-file", "testfile"};
    CHECK(readCompileOptions(options, argv_good));
    options.filename.clear();
    std::vector<std::string> argv_bad1{"-file"};
    CHECK_FALSE(readCompileOptions(options, argv_bad1));
    options.filename.clear();
    std::vector<std::string> argv_bad2{"file", "testfile"};
    CHECK_FALSE(readCompileOptions(options, argv_bad2));
}
