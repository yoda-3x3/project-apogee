#pragma once

namespace apogee::core {

enum class FlightPhase { OnRail, Boost, Coast, Apogee, Drogue, Main, Landed };

struct PhaseTransitionContext {
    double altitudeAglM = 0;
    double verticalVelocityMs = 0;
    double railDisplacementM = 0;  // distance traveled along the rail axis since launch
    double railLengthM = 0;
    bool motorBurnedOut = false;
    double timeSinceBurnoutS = 0;
    double ejectionDelayS = 0;  // motor's selected delay charge
    double mainDeployAltitudeAglM = 0;
};

// Pure function: OnRail -> Boost -> Coast -> Apogee -> Drogue -> Main ->
// Landed. The ejection charge fires at timeSinceBurnoutS >= ejectionDelayS
// regardless of whether true apogee (velocity sign change) has happened yet
// -- a real, common failure mode is a delay that's too short, ejecting
// while still ascending, which this models by allowing Coast to jump
// straight to Drogue without ever visiting Apogee if the delay elapses
// first.
FlightPhase computeNextPhase(FlightPhase current, const PhaseTransitionContext& ctx);

}  // namespace apogee::core
