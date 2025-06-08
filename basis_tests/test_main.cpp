#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../compiler.h"

TEST_CASE("Hello tests") {
    CHECK(8 == 8);
}