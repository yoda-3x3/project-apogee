#pragma once

#include <vector>

namespace apogee::core {

// core's own minimal thrust-sample type (zero Qt dependency) -- the app
// layer converts from data::ThrustSample when building a MotorModel from a
// cached ThrustCurve.org simfile.
struct ThrustSample {
    double timeS = 0;
    double thrustN = 0;
};

// A motor's thrust curve plus propellant/casing mass, giving thrust(t) and
// instantaneous total mass(t) during the burn.
class MotorModel {
public:
    MotorModel(std::vector<ThrustSample> samples, double propellantMassKg, double casingMassKg);

    // Piecewise-linear interpolation over the recorded samples; 0 before
    // ignition or after burnout.
    double thrust(double timeSinceIgnitionS) const;

    double burnTimeS() const { return burnTimeS_; }
    double totalImpulseNs() const { return totalImpulseNs_; }
    double propellantMassKg() const { return propellantMassKg_; }
    double casingMassKg() const { return casingMassKg_; }

    // casing + remaining propellant, propellant burned in proportion to
    // cumulative delivered impulse (not simple linear-in-time, since thrust
    // isn't constant).
    double totalMassKg(double timeSinceIgnitionS) const;

private:
    std::vector<ThrustSample> samples_;
    double propellantMassKg_;
    double casingMassKg_;
    double totalImpulseNs_;

    double burnTimeS_;

    // Cumulative delivered impulse from ignition through timeSinceIgnitionS.
    double deliveredImpulseNs(double timeSinceIgnitionS) const;
};

}  // namespace apogee::core
