#include "core/wind_field.hpp"

#include <algorithm>
#include <cmath>

namespace apogee::core {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kReferenceHeightM = 10.0;  // standard met reference height for power-law shear
constexpr double kGustMeanReversionPerS = 0.5;  // ~2s time constant

double degToRad(double deg) { return deg * kPi / 180.0; }

// Shortest-path linear interpolation between two compass bearings.
double interpolateAngleDeg(double a, double b, double t) {
    double diff = std::fmod(b - a + 540.0, 360.0) - 180.0;
    double result = std::fmod(a + diff * t, 360.0);
    if (result < 0) result += 360.0;
    return result;
}
}  // namespace

WindField WindField::powerLawShear(double groundSpeedMs, double groundDirectionDeg,
                                    double exponent) {
    WindField w;
    w.mode_ = Mode::PowerLaw;
    w.groundSpeedMs_ = groundSpeedMs;
    w.groundDirectionDeg_ = groundDirectionDeg;
    w.shearExponent_ = exponent;
    return w;
}

WindField WindField::fromLevels(std::vector<WindLevel> levels) {
    std::sort(levels.begin(), levels.end(),
              [](const WindLevel& a, const WindLevel& b) { return a.altitudeM < b.altitudeM; });
    WindField w;
    w.mode_ = Mode::Levels;
    w.levels_ = std::move(levels);
    return w;
}

void WindField::setGustStdDevMs(double stdDevMs, std::uint32_t seed) {
    gustStdDevMs_ = stdDevMs;
    currentGustMs_ = 0.0;
    gustRng_.seed(seed);
}

void WindField::advanceGust(double dt) {
    if (gustStdDevMs_ <= 0.0 || dt <= 0.0) return;
    // Ornstein-Uhlenbeck: mean-reverting, bounded random walk.
    const double sigma = gustStdDevMs_ * std::sqrt(2.0 * kGustMeanReversionPerS);
    currentGustMs_ += -kGustMeanReversionPerS * currentGustMs_ * dt +
                       sigma * std::sqrt(dt) * gustNormal_(gustRng_);
}

double WindField::baseSpeedAt(double altitudeAglM) const {
    if (mode_ == Mode::PowerLaw) {
        const double h = std::max(altitudeAglM, 0.1);
        return groundSpeedMs_ * std::pow(h / kReferenceHeightM, shearExponent_);
    }

    if (levels_.empty()) return 0.0;
    if (altitudeAglM <= levels_.front().altitudeM) return levels_.front().speedMs;
    if (altitudeAglM >= levels_.back().altitudeM) return levels_.back().speedMs;

    for (std::size_t i = 1; i < levels_.size(); ++i) {
        if (altitudeAglM <= levels_[i].altitudeM) {
            const WindLevel& lo = levels_[i - 1];
            const WindLevel& hi = levels_[i];
            const double t = (altitudeAglM - lo.altitudeM) / (hi.altitudeM - lo.altitudeM);
            return lo.speedMs + t * (hi.speedMs - lo.speedMs);
        }
    }
    return levels_.back().speedMs;
}

double WindField::directionAt(double altitudeAglM) const {
    if (mode_ == Mode::PowerLaw) return groundDirectionDeg_;

    if (levels_.empty()) return 0.0;
    if (altitudeAglM <= levels_.front().altitudeM) return levels_.front().directionDeg;
    if (altitudeAglM >= levels_.back().altitudeM) return levels_.back().directionDeg;

    for (std::size_t i = 1; i < levels_.size(); ++i) {
        if (altitudeAglM <= levels_[i].altitudeM) {
            const WindLevel& lo = levels_[i - 1];
            const WindLevel& hi = levels_[i];
            const double t = (altitudeAglM - lo.altitudeM) / (hi.altitudeM - lo.altitudeM);
            return interpolateAngleDeg(lo.directionDeg, hi.directionDeg, t);
        }
    }
    return levels_.back().directionDeg;
}

Vec3 WindField::windAt(double altitudeAglM) const {
    const double speed = std::max(baseSpeedAt(altitudeAglM) + currentGustMs_, 0.0);
    const double directionDeg = directionAt(altitudeAglM);

    // Meteorological convention: directionDeg is where the wind is FROM, so
    // it blows TOWARD directionDeg + 180, clockwise from north.
    const double towardAzimuthRad = degToRad(directionDeg + 180.0);
    return Vec3{speed * std::sin(towardAzimuthRad), speed * std::cos(towardAzimuthRad), 0.0};
}

}  // namespace apogee::core
