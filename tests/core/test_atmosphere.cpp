#include <catch_amalgamated.hpp>

#include "core/atmosphere.hpp"

using namespace apogee::core;

TEST_CASE("standardAtmosphere matches known ISA sea-level values", "[core][atmosphere]") {
    const AtmosphericState s = standardAtmosphere(0.0);
    CHECK(s.temperatureK == Catch::Approx(288.15));
    CHECK(s.pressurePa == Catch::Approx(101325.0));
    CHECK(s.densityKgM3 == Catch::Approx(1.225).margin(0.001));
    CHECK(s.speedOfSoundMs == Catch::Approx(340.3).margin(0.2));
}

TEST_CASE("standardAtmosphere matches known ISA tropopause (11km) values", "[core][atmosphere]") {
    const AtmosphericState s = standardAtmosphere(11000.0);
    CHECK(s.temperatureK == Catch::Approx(216.65).margin(0.01));
    CHECK(s.pressurePa == Catch::Approx(22632.0).margin(50.0));
}

TEST_CASE("density and temperature decrease monotonically with altitude through the troposphere",
          "[core][atmosphere]") {
    const AtmosphericState low = standardAtmosphere(0.0);
    const AtmosphericState mid = standardAtmosphere(3000.0);
    const AtmosphericState high = standardAtmosphere(10000.0);

    CHECK(low.temperatureK > mid.temperatureK);
    CHECK(mid.temperatureK > high.temperatureK);
    CHECK(low.densityKgM3 > mid.densityKgM3);
    CHECK(mid.densityKgM3 > high.densityKgM3);
}

TEST_CASE("temperature is constant through the stratosphere layer modeled (11-20km)",
          "[core][atmosphere]") {
    const AtmosphericState a = standardAtmosphere(12000.0);
    const AtmosphericState b = standardAtmosphere(18000.0);
    CHECK(a.temperatureK == Catch::Approx(b.temperatureK).margin(0.01));
    CHECK(a.pressurePa > b.pressurePa);  // pressure still drops even though temperature doesn't
}
