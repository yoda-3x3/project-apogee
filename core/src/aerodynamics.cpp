#include "core/aerodynamics.hpp"

namespace apogee::core {

double dragCoefficient(double mach) {
    if (mach < 0.8) return 0.45;
    if (mach < 1.1) {
        const double t = (mach - 0.8) / (1.1 - 0.8);
        return 0.45 + t * (0.90 - 0.45);
    }
    if (mach < 2.0) {
        const double t = (mach - 1.1) / (2.0 - 1.1);
        return 0.90 - t * (0.90 - 0.55);
    }
    return 0.55;
}

double pitchYawDampingCoefficient(double airDensityKgM3, double relativeVelocityMs,
                                   double referenceAreaM2, double finCnAlphaPerRad,
                                   double finCpToCgDistanceM) {
    return 0.5 * airDensityKgM3 * relativeVelocityMs * finCnAlphaPerRad * referenceAreaM2 *
           finCpToCgDistanceM * finCpToCgDistanceM;
}

}  // namespace apogee::core
