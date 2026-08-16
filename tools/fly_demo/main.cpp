// Phase 3 demo tool: flies a hardcoded rocket+motor through the 6DOF
// simulation engine and prints apogee/max velocity/max G/etc -- fully
// numerically verifiable with no GUI. Pass --verbose to also dump a
// periodic trajectory trace (useful for debugging).
#include <cstdio>
#include <cstring>

#include "core/motor_model.hpp"
#include "core/rocket_definition.hpp"
#include "core/simulation.hpp"

using namespace apogee::core;

namespace {

// Real Estes C6 thrust curve (recorded live from thrustcurve.org, see
// tests/fixtures/thrustcurve_download_samples.json) paired with an
// Alpha-III-like BT-50/PNC-50/3-fin airframe -- see tests/core/fixtures.hpp
// for the full derivation of the estimated mass properties below.
MotorModel makeMotor() {
    std::vector<ThrustSample> samples = {
        {0.031, 0.946},  {0.092, 4.826}, {0.139, 9.936}, {0.192, 14.09}, {0.209, 11.446},
        {0.231, 7.381},  {0.248, 6.151}, {0.292, 5.489}, {0.37, 4.921},  {0.475, 4.448},
        {0.671, 4.258},  {0.702, 4.542}, {0.723, 4.164}, {0.85, 4.448},  {1.063, 4.353},
        {1.211, 4.353},  {1.242, 4.069}, {1.303, 4.258}, {1.468, 4.353}, {1.656, 4.448},
        {1.821, 4.448},  {1.834, 2.933}, {1.847, 1.325}, {1.86, 0},
    };
    return MotorModel(std::move(samples), 0.0108, 0.0133);
}

RocketDefinition makeRocket() {
    RocketDefinition rocket;
    rocket.referenceDiameterM = 0.0248;
    rocket.noseShape = NoseShape::Ogive;
    rocket.noseLengthM = 0.0683;
    rocket.finCount = 3;
    rocket.finRootChordM = 0.076;
    rocket.finTipChordM = 0.025;
    rocket.finSemiSpanM = 0.045;
    rocket.finSweepLengthM = 0.040;
    rocket.finRootLeadingEdgeFromNoseM = 0.28;
    rocket.dryMassKg = 0.026;
    rocket.dryCgFromNoseM = 0.20;
    rocket.transverseMomentOfInertiaKgM2 = 0.000524;
    rocket.axialMomentOfInertiaKgM2 = 0.00000384;
    rocket.motorCgFromNoseM = 0.30;
    rocket.dragMultiplier = 1.0;
    const double chuteDiameterM = 0.305;
    const double chuteAreaM2 = 3.14159265358979323846 * (chuteDiameterM * 0.5) * (chuteDiameterM * 0.5);
    rocket.drogueCd = 0.75;
    rocket.drogueAreaM2 = chuteAreaM2;
    rocket.mainCd = 0.75;
    rocket.mainAreaM2 = chuteAreaM2;
    rocket.mainDeployAltitudeAglM = 1.0e6;  // single-deploy kit, see fixtures.hpp
    return rocket;
}

const char* phaseName(FlightPhase p) {
    switch (p) {
        case FlightPhase::OnRail: return "OnRail";
        case FlightPhase::Boost: return "Boost";
        case FlightPhase::Coast: return "Coast";
        case FlightPhase::Apogee: return "Apogee";
        case FlightPhase::Drogue: return "Drogue";
        case FlightPhase::Main: return "Main";
        case FlightPhase::Landed: return "Landed";
    }
    return "?";
}

}  // namespace

int main(int argc, char** argv) {
    bool verbose = false;
    double windSpeedMs = 0.0;
    double windDirectionDeg = 0.0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verbose") == 0) verbose = true;
        if (std::strcmp(argv[i], "--wind") == 0 && i + 2 < argc) {
            windSpeedMs = std::atof(argv[i + 1]);
            windDirectionDeg = std::atof(argv[i + 2]);
        }
    }

    const RocketDefinition rocket = makeRocket();
    const MotorModel motor = makeMotor();

    LaunchConditions launch;
    launch.railLengthM = 1.0;
    launch.railAngleFromVerticalDeg = 0.0;
    launch.railAzimuthDeg = 0.0;
    launch.launchSiteElevationM = 0.0;
    launch.wind = WindField::powerLawShear(windSpeedMs, windDirectionDeg);

    SimulationConfig config;
    config.timeStepS = 0.001;
    config.maxSimTimeS = 300.0;
    config.ejectionDelayS = 5.0;
    config.telemetryIntervalS = verbose ? 0.01 : 0.02;

    const Telemetry telemetry = Simulation::run(rocket, motor, launch, config);

    if (verbose) {
        std::printf("%8s %10s %10s %10s %10s %8s\n", "t(s)", "alt(m)", "vel(m/s)", "mach", "stab(cal)",
                    "phase");
        FlightPhase lastPhase = FlightPhase::OnRail;
        for (const TelemetrySample& s : telemetry.samples) {
            if (s.phase != lastPhase) {
                std::printf("  -- phase change: %s -> %s at t=%.3f --\n", phaseName(lastPhase),
                            phaseName(s.phase), s.timeS);
                lastPhase = s.phase;
            }
            std::printf("%8.2f %10.2f %10.2f %10.3f %10.2f %8s\n", s.timeS, s.position.z,
                        s.velocity.norm(), s.machNumber, s.stabilityMarginCalibers, phaseName(s.phase));
        }
        std::printf("\n");
    }

    const SummaryStats& sum = telemetry.summary;
    std::printf("Apogee:              %.1f m at t=%.2f s\n", sum.apogeeM, sum.apogeeTimeS);
    std::printf("Max velocity:        %.1f m/s (Mach %.3f)\n", sum.maxVelocityMs, sum.maxMachNumber);
    std::printf("Max acceleration:    %.1f G\n", sum.maxAccelerationG);
    std::printf("Max dynamic pressure:%.1f Pa\n", sum.maxDynamicPressurePa);
    std::printf("Burnout:             %.1f m/s at %.1f m altitude\n", sum.burnoutVelocityMs,
                sum.burnoutAltitudeM);
    std::printf("Rail exit stable:    %s (min margin %.2f calibers)\n",
                sum.railExitStable ? "yes" : "no", sum.minStabilityMarginCalibers);
    std::printf("Landing descent rate:%.1f m/s\n", sum.descentRateMainMs);
    std::printf("Flight duration:     %.1f s\n", sum.flightDurationS);
    std::printf("Drift distance:      %.1f m (offset %.1f E, %.1f N)\n", sum.driftDistanceM,
                sum.landingOffsetM.x, sum.landingOffsetM.y);
    std::printf("Final phase:         %s\n",
                telemetry.samples.empty() ? "?" : phaseName(telemetry.samples.back().phase));

    return 0;
}
