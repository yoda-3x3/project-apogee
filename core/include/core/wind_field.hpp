#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "core/vec3.hpp"

namespace apogee::core {

// One altitude band's wind reading -- shape mirrors data::WindLevel, but
// core has zero Qt/data dependency, so the app layer converts when building
// a WindField from a live data::WindProfile (Phase 6).
struct WindLevel {
    double altitudeM = 0;
    double speedMs = 0;
    double directionDeg = 0;  // meteorological convention: direction wind is FROM, clockwise from north
};

// Ground wind + power-law shear, or an explicit altitude-indexed profile
// (e.g. from live Open-Meteo data). Optional bounded gusts are layered on
// top via a seeded random walk, so a run is reproducible/replayable given
// the same seed.
//
// Gust state must be advanced explicitly via advanceGust(), exactly once
// per outer simulation timestep -- NOT once per RK4 sub-evaluation (RK4
// calls the derivative function, and thus windAt(), 4 times per step at
// t/t+dt/2/t+dt/2/t+dt; evolving stochastic state on each of those calls
// would double-process t+dt/2 and corrupt the gust process). windAt() itself
// is pure given the current gust value.
class WindField {
public:
    static WindField powerLawShear(double groundSpeedMs, double groundDirectionDeg,
                                    double exponent = 0.14);
    static WindField fromLevels(std::vector<WindLevel> levels);

    void setGustStdDevMs(double stdDevMs, std::uint32_t seed);
    void advanceGust(double dt);

    // Wind vector in ENU world frame at the given AGL altitude, including
    // whatever gust perturbation is currently active.
    Vec3 windAt(double altitudeAglM) const;

private:
    enum class Mode { PowerLaw, Levels };

    Mode mode_ = Mode::PowerLaw;
    double groundSpeedMs_ = 0;
    double groundDirectionDeg_ = 0;
    double shearExponent_ = 0.14;
    std::vector<WindLevel> levels_;

    double gustStdDevMs_ = 0;
    double currentGustMs_ = 0;
    mutable std::mt19937 gustRng_;
    mutable std::normal_distribution<double> gustNormal_{0.0, 1.0};

    double baseSpeedAt(double altitudeAglM) const;
    double directionAt(double altitudeAglM) const;
};

}  // namespace apogee::core
