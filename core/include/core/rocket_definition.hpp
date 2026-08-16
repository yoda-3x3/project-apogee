#pragma once

namespace apogee::core {

enum class NoseShape { Ogive, Conical };

// A plain description of the airframe (everything except the motor, which
// is a separate MotorModel). Phase 4's rocket builder constructs one of
// these from data:: component records; this struct itself has zero
// knowledge of the database.
//
// Simplifications explicitly made for this MVP (see plan's "open risks"):
// mass properties (moments of inertia, dry CG) are constants, not derived
// per-component; the motor is treated as a point mass at a fixed axial
// station (only its mass, not its own CG, shifts as propellant burns) --
// the rocket's overall CG still shifts correctly via the mass-weighted
// average with the dry airframe (see barrowman.cpp / simulation.cpp).
struct RocketDefinition {
    double referenceDiameterM = 0;  // for dynamic-pressure reference area (A_ref = pi*(d/2)^2)

    // Nose
    NoseShape noseShape = NoseShape::Ogive;
    double noseLengthM = 0;

    // Fins: one aggregate through-body fin set (MVP -- Barrowman's fin
    // formula as given in the plan assumes exactly this).
    int finCount = 0;
    double finRootChordM = 0;
    double finTipChordM = 0;
    double finSemiSpanM = 0;
    double finSweepLengthM = 0;             // axial distance, root LE to tip LE
    double finRootLeadingEdgeFromNoseM = 0;  // where the fin root chord starts, from nose tip

    // Mass properties (dry airframe, i.e. everything except the motor)
    double dryMassKg = 0;
    double dryCgFromNoseM = 0;
    double transverseMomentOfInertiaKgM2 = 0;  // about an axis through the CG, perpendicular to body axis
    double axialMomentOfInertiaKgM2 = 0;       // about the body's long axis

    // Where the motor sits (point-mass approximation, see class comment)
    double motorCgFromNoseM = 0;

    // Drag (simplified: aerodynamics.cpp supplies the Mach-dependent shape
    // of Cd(Mach); this is a per-rocket multiplier for finish/shape quality,
    // 1.0 = as-modeled)
    double dragMultiplier = 1.0;

    // Recovery
    double drogueCd = 0.75;
    double drogueAreaM2 = 0;
    double mainCd = 0.75;
    double mainAreaM2 = 0;
    double mainDeployAltitudeAglM = 200.0;
};

}  // namespace apogee::core
