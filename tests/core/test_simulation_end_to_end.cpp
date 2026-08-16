#include <catch_amalgamated.hpp>

#include "core/simulation.hpp"
#include "fixtures.hpp"

using namespace apogee::core;

namespace {
LaunchConditions makeCalmLaunch() {
    LaunchConditions launch;
    launch.railLengthM = 1.0;
    launch.railAngleFromVerticalDeg = 0.0;
    launch.railAzimuthDeg = 0.0;
    launch.launchSiteElevationM = 0.0;
    launch.wind = WindField::powerLawShear(0.0, 0.0);
    return launch;
}

SimulationConfig makeConfig() {
    SimulationConfig config;
    config.timeStepS = 0.001;
    // A single 12in chute reused for both drogue+main (see fixtures.hpp)
    // gives a gentle ~3.4 m/s descent from a ~400-450m apogee, i.e. a
    // ~130s full flight -- confirmed via tools/fly_demo -- so this needs
    // real headroom, not just apogee/burnout time.
    config.maxSimTimeS = 200.0;
    config.ejectionDelayS = 5.0;  // real C6-5 delay grade
    return config;
}
}  // namespace

TEST_CASE("a real Estes Alpha III + C6-5 combo flies to a plausible apogee, no wind",
          "[core][simulation]") {
    const RocketDefinition rocket = makeAlphaIIILikeRocket();
    const MotorModel motor = makeEstesC6MotorModel();
    const Telemetry telemetry = Simulation::run(rocket, motor, makeCalmLaunch(), makeConfig());

    // Real documented Alpha III + C6 flights land in roughly the 250-400m
    // range; this generous [100,600] band confirms the sim isn't wildly
    // broken (e.g. off by an order of magnitude) without pinning to a
    // number this project can't independently verify to the meter.
    CHECK(telemetry.summary.apogeeM > 100.0);
    CHECK(telemetry.summary.apogeeM < 600.0);
    CHECK(telemetry.summary.apogeeTimeS > 0.0);

    CHECK(telemetry.summary.railExitStable);
    CHECK(telemetry.summary.minStabilityMarginCalibers > 0.0);  // stable throughout

    CHECK(telemetry.summary.burnoutVelocityMs > 0.0);
    CHECK(telemetry.summary.burnoutAltitudeM > 0.0);
    CHECK(telemetry.summary.burnoutAltitudeM < telemetry.summary.apogeeM);

    // Should complete the flight (reach Landed) well within the time budget.
    REQUIRE_FALSE(telemetry.samples.empty());
    CHECK(telemetry.samples.back().phase == FlightPhase::Landed);
    CHECK(telemetry.summary.flightDurationS < makeConfig().maxSimTimeS);

    // Chute should meaningfully slow descent versus free-fall (a C6 rocket
    // in free-fall from ~300m would be moving far faster than this by
    // landing).
    CHECK(telemetry.summary.descentRateMainMs > 0.0);
    CHECK(telemetry.summary.descentRateMainMs < 15.0);

    for (const TelemetrySample& sample : telemetry.samples) {
        CHECK(sample.orientation.norm() == Catch::Approx(1.0).margin(1e-6));
    }
}

TEST_CASE("a zero-wind, perfectly vertical launch stays essentially over the pad",
          "[core][simulation]") {
    const RocketDefinition rocket = makeAlphaIIILikeRocket();
    const MotorModel motor = makeEstesC6MotorModel();
    const Telemetry telemetry = Simulation::run(rocket, motor, makeCalmLaunch(), makeConfig());

    // No wind and a symmetric rocket starting exactly vertical should never
    // develop meaningful angle of attack, so horizontal drift should be
    // tiny (not exactly zero -- floating point and the rail-exit instant
    // introduce negligible asymmetry).
    CHECK(telemetry.summary.driftDistanceM < 5.0);
}

TEST_CASE("a stable rocket drifts downwind in a crosswind (weathercocking sign check)",
          "[core][simulation]") {
    const RocketDefinition rocket = makeAlphaIIILikeRocket();
    const MotorModel motor = makeEstesC6MotorModel();

    LaunchConditions launch = makeCalmLaunch();
    // Wind FROM the west (270 deg) blows TOWARD the east -- a correctly
    // signed weathercocking/drift model should land the rocket east of the
    // pad, not west or wildly off-axis from a tumble.
    launch.wind = WindField::powerLawShear(5.0, 270.0);

    const Telemetry telemetry = Simulation::run(rocket, motor, launch, makeConfig());

    CHECK(telemetry.summary.railExitStable);
    CHECK(telemetry.summary.minStabilityMarginCalibers > 0.0);
    CHECK(telemetry.summary.landingOffsetM.x > 0.0);  // drifted east (downwind)
    CHECK(telemetry.summary.driftDistanceM > 0.0);

    for (const TelemetrySample& sample : telemetry.samples) {
        CHECK(sample.orientation.norm() == Catch::Approx(1.0).margin(1e-6));
    }
}
