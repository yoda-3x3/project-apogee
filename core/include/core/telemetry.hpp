#pragma once

#include <vector>

#include "core/flight_phase.hpp"
#include "core/quaternion.hpp"
#include "core/vec3.hpp"

namespace apogee::core {

struct TelemetrySample {
    double timeS = 0;
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Quaternion orientation;
    Vec3 angularVelocity;
    double machNumber = 0;
    double dynamicPressurePa = 0;
    double gForce = 0;
    double stabilityMarginCalibers = 0;
    FlightPhase phase = FlightPhase::OnRail;
};

struct SummaryStats {
    double apogeeM = 0;
    double apogeeTimeS = 0;
    double maxVelocityMs = 0;
    double maxMachNumber = 0;
    double maxAccelerationG = 0;
    double maxDynamicPressurePa = 0;
    double burnoutVelocityMs = 0;
    double burnoutAltitudeM = 0;
    double descentRateMainMs = 0;
    double flightDurationS = 0;
    double driftDistanceM = 0;
    Vec3 landingOffsetM;  // x=East, y=North; z unused
    double minStabilityMarginCalibers = 0;
    bool railExitStable = false;
};

struct Telemetry {
    std::vector<TelemetrySample> samples;
    SummaryStats summary;
};

}  // namespace apogee::core
