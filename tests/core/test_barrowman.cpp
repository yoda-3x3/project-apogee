#include <catch_amalgamated.hpp>

#include "core/barrowman.hpp"
#include "fixtures.hpp"

using namespace apogee::core;

TEST_CASE("nose-only contribution matches the exact Barrowman formula", "[core][barrowman]") {
    // Isolate the nose term (no fins) so this is exactly hand-verifiable:
    // Cn_alpha,nose = 2/rad (shape-independent), X_nose = 0.466*L for an
    // ogive.
    RocketDefinition rocket;
    rocket.noseShape = NoseShape::Ogive;
    rocket.noseLengthM = 0.10;
    rocket.finCount = 0;

    const BarrowmanResult result = computeBarrowman(rocket);
    CHECK(result.totalCnAlphaPerRad == Catch::Approx(2.0));
    CHECK(result.centerOfPressureFromNoseM == Catch::Approx(0.466 * 0.10));
}

TEST_CASE("a conical nose has its CP further aft than an ogive of the same length",
          "[core][barrowman]") {
    RocketDefinition ogive;
    ogive.noseShape = NoseShape::Ogive;
    ogive.noseLengthM = 0.10;

    RocketDefinition conical = ogive;
    conical.noseShape = NoseShape::Conical;

    CHECK(computeBarrowman(conical).centerOfPressureFromNoseM >
          computeBarrowman(ogive).centerOfPressureFromNoseM);
}

TEST_CASE("adding fins moves the combined CP aft of the nose-alone CP", "[core][barrowman]") {
    const RocketDefinition rocket = makeAlphaIIILikeRocket();
    const BarrowmanResult withFins = computeBarrowman(rocket);

    RocketDefinition noseOnly = rocket;
    noseOnly.finCount = 0;
    const BarrowmanResult noseAlone = computeBarrowman(noseOnly);

    CHECK(withFins.totalCnAlphaPerRad > noseAlone.totalCnAlphaPerRad);
    CHECK(withFins.centerOfPressureFromNoseM > noseAlone.centerOfPressureFromNoseM);
    // CP should sit between the nose and the fins, closer to the fins since
    // their Cn_alpha dominates the nose's.
    CHECK(withFins.centerOfPressureFromNoseM < withFins.finCenterOfPressureFromNoseM);
}

TEST_CASE("moving fins further aft moves the combined CP further aft", "[core][barrowman]") {
    RocketDefinition rocket = makeAlphaIIILikeRocket();
    const double cpBefore = computeBarrowman(rocket).centerOfPressureFromNoseM;

    rocket.finRootLeadingEdgeFromNoseM += 0.05;
    const double cpAfter = computeBarrowman(rocket).centerOfPressureFromNoseM;

    CHECK(cpAfter > cpBefore);
}

TEST_CASE("stability margin sign: the Alpha-III-like fixture is stable at liftoff and "
          "more stable at burnout (hand-computed reference values)",
          "[core][barrowman]") {
    // Hand-computed for makeAlphaIIILikeRocket(): X_cp ~ 0.286m from nose.
    // Loaded CG (dry 26g @ 0.20m + motor 24.1g @ 0.30m) / 50.1g ~ 0.248m ->
    // margin ~0.038m ~ 1.5 calibers. Burnout CG (dry 26g @ 0.20m + casing
    // 13.3g @ 0.30m) / 39.3g ~ 0.234m -> margin ~0.052m ~ 2.1 calibers.
    // Generous tolerances below since these are hand-arithmetic reference
    // values, not exact -- the point is confirming sign and the
    // burnout-more-stable-than-liftoff direction, not pinning digits.
    const RocketDefinition rocket = makeAlphaIIILikeRocket();
    const BarrowmanResult barrowman = computeBarrowman(rocket);
    CHECK(barrowman.centerOfPressureFromNoseM == Catch::Approx(0.286).margin(0.01));

    const double loadedMotorMassKg = 0.0241;   // full propellant
    const double burnoutMotorMassKg = 0.0133;  // casing only

    const double loadedCg =
        (rocket.dryMassKg * rocket.dryCgFromNoseM + loadedMotorMassKg * rocket.motorCgFromNoseM) /
        (rocket.dryMassKg + loadedMotorMassKg);
    const double burnoutCg =
        (rocket.dryMassKg * rocket.dryCgFromNoseM + burnoutMotorMassKg * rocket.motorCgFromNoseM) /
        (rocket.dryMassKg + burnoutMotorMassKg);

    const double loadedMarginCalibers =
        (barrowman.centerOfPressureFromNoseM - loadedCg) / rocket.referenceDiameterM;
    const double burnoutMarginCalibers =
        (barrowman.centerOfPressureFromNoseM - burnoutCg) / rocket.referenceDiameterM;

    CHECK(loadedMarginCalibers > 0.0);        // stable (CP aft of CG)
    CHECK(loadedMarginCalibers == Catch::Approx(1.5).margin(0.3));
    CHECK(burnoutMarginCalibers > loadedMarginCalibers);  // CG moves forward as motor burns
    CHECK(burnoutMarginCalibers == Catch::Approx(2.1).margin(0.3));
}
