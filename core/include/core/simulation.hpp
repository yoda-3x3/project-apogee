#pragma once

#include "core/motor_model.hpp"
#include "core/rocket_definition.hpp"
#include "core/telemetry.hpp"
#include "core/wind_field.hpp"

namespace apogee::core {

struct LaunchConditions {
    double railLengthM = 1.0;
    double railAngleFromVerticalDeg = 0;  // 0 = straight up
    double railAzimuthDeg = 0;            // compass bearing the rail tips toward
    double launchSiteElevationM = 0;      // for ASL altitude in the atmosphere model
    WindField wind;
};

struct SimulationConfig {
    double timeStepS = 0.001;
    double maxSimTimeS = 300.0;
    double ejectionDelayS = 3.0;
    double telemetryIntervalS = 0.02;  // RK4 still steps at timeStepS; this downsamples recording
};

class Simulation {
public:
    // Pure function: no Qt, no I/O, fully unit-testable headlessly.
    static Telemetry run(const RocketDefinition& rocket, const MotorModel& motor,
                          const LaunchConditions& launch, const SimulationConfig& config);
};

}  // namespace apogee::core
