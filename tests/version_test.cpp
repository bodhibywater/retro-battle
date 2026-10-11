#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "retro/version.hpp"

TEST_CASE("version reports the current application version") {
    REQUIRE(retro::version() == std::string_view{"0.1.0"});
}