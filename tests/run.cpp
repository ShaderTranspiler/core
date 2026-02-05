#include <Test.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("sample test case", "[core]") {
    REQUIRE(stc::Test::retOne() == 1);
}
