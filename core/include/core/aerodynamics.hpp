#pragma once

namespace apogee::core {

// Simplified piecewise Cd(Mach) curve, representative of a smooth-finish,
// finned model rocket: roughly constant subsonic, rising through the
// transonic drag rise toward Mach 1, easing off supersonic. Not shape- or
// finish-specific -- RocketDefinition::dragMultiplier is the per-rocket
// adjustment knob.
double dragCoefficient(double mach);

// Approximate pitch/yaw aerodynamic damping torque coefficient (a
// simplified Cmq-style estimate): a rocket rotating at body rate omega
// gives its fins an extra local velocity component of
// omega * (finCp - cg), which linearizes to an effective angle-of-attack
// contribution of (omega * armM / relativeVelocityMs) -- one factor of
// velocity cancels against the dynamic-pressure term, leaving damping
// torque linear in both omega and |v|, not |v|^2. Returns k such that
// damping torque = -k * omega (per axis, body frame).
double pitchYawDampingCoefficient(double airDensityKgM3, double relativeVelocityMs,
                                   double referenceAreaM2, double finCnAlphaPerRad,
                                   double finCpToCgDistanceM);

}  // namespace apogee::core
