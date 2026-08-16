#pragma once

#include "core/motor_model.hpp"
#include "core/rocket_definition.hpp"

// A hardcoded rocket+motor combo standing in for a real Estes Alpha III +
// C6-5: thrust samples below are the REAL Estes C6 curve (copied from
// tests/fixtures/thrustcurve_download_samples.json, itself recorded from a
// live thrustcurve.org query -- see data-layer tests). Airframe geometry
// mirrors data/seed/components.json's pnc-50/bt-50/fin-50-3 dimensions.
// dryMassKg/dryCgFromNoseM/motorCgFromNoseM/moments-of-inertia are estimated
// (not from a real data sheet) but hand-checked to give a sane, stable
// design: Barrowman CP ~0.286m from nose, loaded CG ~0.248m -> ~1.5
// calibers stability margin at liftoff, ~2.1 at burnout (margin should
// increase as the tail-mounted motor's mass burns away, moving CG forward).

inline apogee::core::MotorModel makeEstesC6MotorModel() {
    using apogee::core::ThrustSample;
    std::vector<ThrustSample> samples = {
        {0.031, 0.946},  {0.092, 4.826}, {0.139, 9.936}, {0.192, 14.09}, {0.209, 11.446},
        {0.231, 7.381},  {0.248, 6.151}, {0.292, 5.489}, {0.37, 4.921},  {0.475, 4.448},
        {0.671, 4.258},  {0.702, 4.542}, {0.723, 4.164}, {0.85, 4.448},  {1.063, 4.353},
        {1.211, 4.353},  {1.242, 4.069}, {1.303, 4.258}, {1.468, 4.353}, {1.656, 4.448},
        {1.821, 4.448},  {1.834, 2.933}, {1.847, 1.325}, {1.86, 0},
    };
    // Real Estes C6 spec: totalWeightG 24.1, propWeightG 10.8 -> casing 13.3g.
    return apogee::core::MotorModel(std::move(samples), 0.0108, 0.0133);
}

inline apogee::core::RocketDefinition makeAlphaIIILikeRocket() {
    apogee::core::RocketDefinition rocket;
    rocket.referenceDiameterM = 0.0248;  // BT-50

    rocket.noseShape = apogee::core::NoseShape::Ogive;
    rocket.noseLengthM = 0.0683;  // PNC-50

    rocket.finCount = 3;
    rocket.finRootChordM = 0.076;
    rocket.finTipChordM = 0.025;
    rocket.finSemiSpanM = 0.045;
    rocket.finSweepLengthM = 0.040;
    rocket.finRootLeadingEdgeFromNoseM = 0.28;

    rocket.dryMassKg = 0.026;
    rocket.dryCgFromNoseM = 0.20;
    rocket.transverseMomentOfInertiaKgM2 = 0.000524;
    rocket.axialMomentOfInertiaKgM2 = 0.00000384;

    rocket.motorCgFromNoseM = 0.30;

    rocket.dragMultiplier = 1.0;

    // Single-deployment kit (typical for this class): the same 12in chute
    // for both stages, with mainDeployAltitudeAglM set high so Drogue -> Main
    // happens essentially immediately after ejection -- see fixtures.hpp
    // comment in simulation.cpp's test for why this models a single-deploy
    // recovery within the two-stage Drogue/Main state machine.
    const double chuteDiameterM = 0.305;  // 12in
    const double chuteAreaM2 = 3.14159265358979323846 * (chuteDiameterM * 0.5) * (chuteDiameterM * 0.5);
    rocket.drogueCd = 0.75;
    rocket.drogueAreaM2 = chuteAreaM2;
    rocket.mainCd = 0.75;
    rocket.mainAreaM2 = chuteAreaM2;
    rocket.mainDeployAltitudeAglM = 1.0e6;

    return rocket;
}
