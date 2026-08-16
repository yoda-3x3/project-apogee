#include <catch_amalgamated.hpp>

#include "core/version.hpp"
#include "data/version.hpp"

TEST_CASE("core and data libraries report a version string", "[scaffold]") {
    REQUIRE(std::string(apogee::core::version()) == "0.1.0");
    REQUIRE(std::string(apogee::data::version()) == "0.1.0");
}
