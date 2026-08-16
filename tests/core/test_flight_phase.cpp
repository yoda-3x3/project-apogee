#include <catch_amalgamated.hpp>

#include "core/flight_phase.hpp"

using namespace apogee::core;

TEST_CASE("OnRail transitions to Boost only once rail displacement reaches rail length",
          "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.railLengthM = 1.0;
    ctx.railDisplacementM = 0.5;
    CHECK(computeNextPhase(FlightPhase::OnRail, ctx) == FlightPhase::OnRail);

    ctx.railDisplacementM = 1.0;
    CHECK(computeNextPhase(FlightPhase::OnRail, ctx) == FlightPhase::Boost);
}

TEST_CASE("Boost transitions to Coast on motor burnout", "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.motorBurnedOut = false;
    CHECK(computeNextPhase(FlightPhase::Boost, ctx) == FlightPhase::Boost);

    ctx.motorBurnedOut = true;
    CHECK(computeNextPhase(FlightPhase::Boost, ctx) == FlightPhase::Coast);
}

TEST_CASE("Coast transitions to Apogee on a velocity sign change", "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.verticalVelocityMs = 5.0;
    ctx.timeSinceBurnoutS = 0.5;
    ctx.ejectionDelayS = 5.0;  // delay hasn't elapsed
    CHECK(computeNextPhase(FlightPhase::Coast, ctx) == FlightPhase::Coast);

    ctx.verticalVelocityMs = -0.1;
    CHECK(computeNextPhase(FlightPhase::Coast, ctx) == FlightPhase::Apogee);
}

TEST_CASE("Coast jumps straight to Drogue if the ejection delay elapses before apogee "
          "(a premature-ejection failure mode)",
          "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.verticalVelocityMs = 20.0;  // still climbing fast
    ctx.timeSinceBurnoutS = 5.0;
    ctx.ejectionDelayS = 3.0;  // delay already elapsed
    CHECK(computeNextPhase(FlightPhase::Coast, ctx) == FlightPhase::Drogue);
}

TEST_CASE("Apogee transitions to Drogue once the ejection delay elapses", "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.timeSinceBurnoutS = 2.0;
    ctx.ejectionDelayS = 3.0;
    CHECK(computeNextPhase(FlightPhase::Apogee, ctx) == FlightPhase::Apogee);

    ctx.timeSinceBurnoutS = 3.0;
    CHECK(computeNextPhase(FlightPhase::Apogee, ctx) == FlightPhase::Drogue);
}

TEST_CASE("Drogue transitions to Main at the configured deploy altitude", "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.mainDeployAltitudeAglM = 200.0;
    ctx.altitudeAglM = 500.0;
    CHECK(computeNextPhase(FlightPhase::Drogue, ctx) == FlightPhase::Drogue);

    ctx.altitudeAglM = 200.0;
    CHECK(computeNextPhase(FlightPhase::Drogue, ctx) == FlightPhase::Main);
}

TEST_CASE("Main transitions to Landed at ground level", "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    ctx.altitudeAglM = 10.0;
    CHECK(computeNextPhase(FlightPhase::Main, ctx) == FlightPhase::Main);

    ctx.altitudeAglM = 0.0;
    CHECK(computeNextPhase(FlightPhase::Main, ctx) == FlightPhase::Landed);
}

TEST_CASE("Landed is terminal", "[core][flight_phase]") {
    PhaseTransitionContext ctx;
    CHECK(computeNextPhase(FlightPhase::Landed, ctx) == FlightPhase::Landed);
}
