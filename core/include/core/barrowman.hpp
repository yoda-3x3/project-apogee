#pragma once

#include "core/rocket_definition.hpp"

namespace apogee::core {

struct BarrowmanResult {
    double totalCnAlphaPerRad = 0;      // combined normal-force-coefficient slope
    double centerOfPressureFromNoseM = 0;
    double finCenterOfPressureFromNoseM = 0;  // fins' own CP, used for the damping estimate
};

// Standard Barrowman linear aerodynamic theory: nose + through-body fins
// contribution to Cn_alpha and center-of-pressure location, both measured
// from the nose tip. Mach/AoA-independent under linear theory, so this is
// computed once per simulation run, not per timestep.
BarrowmanResult computeBarrowman(const RocketDefinition& rocket);

}  // namespace apogee::core
