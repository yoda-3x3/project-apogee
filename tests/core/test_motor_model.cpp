#include <catch_amalgamated.hpp>

#include "core/motor_model.hpp"
#include "fixtures.hpp"

using namespace apogee::core;

TEST_CASE("a triangular thrust curve integrates and interpolates exactly (hand-computed)",
          "[core][motor]") {
    // Triangle: 0N at t=0, ramps to 10N at t=1, back to 0N at t=2.
    // Total impulse = area of triangle = 0.5*base*height = 0.5*2*10 = 10 Ns.
    std::vector<ThrustSample> samples = {{0.0, 0.0}, {1.0, 10.0}, {2.0, 0.0}};
    MotorModel motor(samples, /*propellantMassKg=*/1.0, /*casingMassKg=*/0.5);

    CHECK(motor.burnTimeS() == Catch::Approx(2.0));
    CHECK(motor.totalImpulseNs() == Catch::Approx(10.0));

    CHECK(motor.thrust(0.0) == Catch::Approx(0.0));
    CHECK(motor.thrust(0.5) == Catch::Approx(5.0));  // halfway up the ramp
    CHECK(motor.thrust(1.0) == Catch::Approx(10.0));  // peak
    CHECK(motor.thrust(1.5) == Catch::Approx(5.0));  // halfway down
    CHECK(motor.thrust(2.0) == Catch::Approx(0.0));
    CHECK(motor.thrust(-1.0) == Catch::Approx(0.0));  // before ignition
    CHECK(motor.thrust(3.0) == Catch::Approx(0.0));   // after burnout

    // At the midpoint by TIME (t=1.0), delivered impulse = area of the
    // first triangle half = 0.5*1*10 = 5 Ns = exactly half of total ->
    // exactly half the propellant should be gone.
    CHECK(motor.totalMassKg(0.0) == Catch::Approx(1.5));   // full: casing + propellant
    CHECK(motor.totalMassKg(1.0) == Catch::Approx(1.0));   // half propellant burned
    CHECK(motor.totalMassKg(2.0) == Catch::Approx(0.5));   // casing only
}

TEST_CASE("totalMassKg decreases monotonically through the real Estes C6 burn", "[core][motor]") {
    const MotorModel motor = makeEstesC6MotorModel();
    CHECK(motor.totalImpulseNs() == Catch::Approx(8.82).margin(0.05));  // matches ThrustCurve spec
    CHECK(motor.burnTimeS() == Catch::Approx(1.86));

    double previousMass = motor.totalMassKg(0.0);
    for (double t = 0.05; t <= motor.burnTimeS(); t += 0.05) {
        const double mass = motor.totalMassKg(t);
        CHECK(mass <= previousMass);
        previousMass = mass;
    }
    CHECK(motor.totalMassKg(motor.burnTimeS()) == Catch::Approx(0.0133).margin(0.0001));  // casing only
}
