#include "core/barrowman.hpp"

#include <cmath>

namespace apogee::core {

namespace {
// Nose Cn_alpha is shape-independent under linear Barrowman theory; only
// the CP fraction of nose length varies by shape.
constexpr double kNoseCnAlphaPerRad = 2.0;

double noseCpFraction(NoseShape shape) {
    switch (shape) {
        case NoseShape::Ogive: return 0.466;
        case NoseShape::Conical: return 0.666;
    }
    return 0.466;
}
}  // namespace

BarrowmanResult computeBarrowman(const RocketDefinition& rocket) {
    BarrowmanResult result;

    const double noseCpFromNoseM = noseCpFraction(rocket.noseShape) * rocket.noseLengthM;

    double finCnAlpha = 0.0;
    double finCpFromNoseM = 0.0;
    if (rocket.finCount > 0 && rocket.referenceDiameterM > 0.0) {
        const double s = rocket.finSemiSpanM;
        const double d = rocket.referenceDiameterM;
        const double r = d * 0.5;
        const double cr = rocket.finRootChordM;
        const double ct = rocket.finTipChordM;
        const double lm = rocket.finSweepLengthM;

        const double kfb = 1.0 + r / (s + r);
        const double sweepTerm = std::sqrt(1.0 + (2.0 * lm / (cr + ct)) * (2.0 * lm / (cr + ct)));
        finCnAlpha =
            kfb * (4.0 * rocket.finCount * (s / d) * (s / d)) / (1.0 + sweepTerm);

        const double xf = (lm / 3.0) * (cr + 2.0 * ct) / (cr + ct) +
                           (1.0 / 6.0) * ((cr + ct) - (cr * ct) / (cr + ct));
        finCpFromNoseM = rocket.finRootLeadingEdgeFromNoseM + xf;
    }

    result.totalCnAlphaPerRad = kNoseCnAlphaPerRad + finCnAlpha;
    result.finCenterOfPressureFromNoseM = finCpFromNoseM;

    if (result.totalCnAlphaPerRad > 0.0) {
        result.centerOfPressureFromNoseM =
            (kNoseCnAlphaPerRad * noseCpFromNoseM + finCnAlpha * finCpFromNoseM) /
            result.totalCnAlphaPerRad;
    }

    return result;
}

}  // namespace apogee::core
