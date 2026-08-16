#include "core/flight_phase.hpp"

namespace apogee::core {

FlightPhase computeNextPhase(FlightPhase current, const PhaseTransitionContext& ctx) {
    switch (current) {
        case FlightPhase::OnRail:
            return ctx.railDisplacementM >= ctx.railLengthM ? FlightPhase::Boost
                                                              : FlightPhase::OnRail;

        case FlightPhase::Boost:
            return ctx.motorBurnedOut ? FlightPhase::Coast : FlightPhase::Boost;

        case FlightPhase::Coast:
            if (ctx.verticalVelocityMs <= 0.0) return FlightPhase::Apogee;
            if (ctx.timeSinceBurnoutS >= ctx.ejectionDelayS) return FlightPhase::Drogue;
            return FlightPhase::Coast;

        case FlightPhase::Apogee:
            return ctx.timeSinceBurnoutS >= ctx.ejectionDelayS ? FlightPhase::Drogue
                                                                 : FlightPhase::Apogee;

        case FlightPhase::Drogue:
            return ctx.altitudeAglM <= ctx.mainDeployAltitudeAglM ? FlightPhase::Main
                                                                    : FlightPhase::Drogue;

        case FlightPhase::Main:
            return ctx.altitudeAglM <= 0.0 ? FlightPhase::Landed : FlightPhase::Main;

        case FlightPhase::Landed:
            return FlightPhase::Landed;
    }
    return current;
}

}  // namespace apogee::core
