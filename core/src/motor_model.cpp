#include "core/motor_model.hpp"

#include <algorithm>

namespace apogee::core {

namespace {
double trapezoidalIntegral(const std::vector<ThrustSample>& samples) {
    double total = 0.0;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        const double dt = samples[i].timeS - samples[i - 1].timeS;
        total += 0.5 * (samples[i].thrustN + samples[i - 1].thrustN) * dt;
    }
    return total;
}
}  // namespace

MotorModel::MotorModel(std::vector<ThrustSample> samples, double propellantMassKg,
                        double casingMassKg)
    : samples_(std::move(samples)),
      propellantMassKg_(propellantMassKg),
      casingMassKg_(casingMassKg),
      totalImpulseNs_(trapezoidalIntegral(samples_)),
      burnTimeS_(samples_.empty() ? 0.0 : samples_.back().timeS) {}

double MotorModel::thrust(double timeSinceIgnitionS) const {
    if (samples_.empty() || timeSinceIgnitionS < 0.0 || timeSinceIgnitionS > burnTimeS_) return 0.0;

    const auto it = std::upper_bound(
        samples_.begin(), samples_.end(), timeSinceIgnitionS,
        [](double t, const ThrustSample& s) { return t < s.timeS; });

    if (it == samples_.begin()) return samples_.front().thrustN;
    if (it == samples_.end()) return samples_.back().thrustN;

    const ThrustSample& hi = *it;
    const ThrustSample& lo = *(it - 1);
    const double t = (timeSinceIgnitionS - lo.timeS) / (hi.timeS - lo.timeS);
    return lo.thrustN + t * (hi.thrustN - lo.thrustN);
}

double MotorModel::deliveredImpulseNs(double timeSinceIgnitionS) const {
    if (samples_.empty() || timeSinceIgnitionS <= 0.0) return 0.0;
    if (timeSinceIgnitionS >= burnTimeS_) return totalImpulseNs_;

    double impulse = 0.0;
    for (std::size_t i = 1; i < samples_.size(); ++i) {
        if (samples_[i].timeS >= timeSinceIgnitionS) {
            const double partialThrust = thrust(timeSinceIgnitionS);
            const double dt = timeSinceIgnitionS - samples_[i - 1].timeS;
            impulse += 0.5 * (samples_[i - 1].thrustN + partialThrust) * dt;
            break;
        }
        const double dt = samples_[i].timeS - samples_[i - 1].timeS;
        impulse += 0.5 * (samples_[i].thrustN + samples_[i - 1].thrustN) * dt;
    }
    return impulse;
}

double MotorModel::totalMassKg(double timeSinceIgnitionS) const {
    if (totalImpulseNs_ <= 0.0) return casingMassKg_ + propellantMassKg_;
    const double remainingFraction =
        1.0 - std::clamp(deliveredImpulseNs(timeSinceIgnitionS) / totalImpulseNs_, 0.0, 1.0);
    return casingMassKg_ + propellantMassKg_ * remainingFraction;
}

}  // namespace apogee::core
