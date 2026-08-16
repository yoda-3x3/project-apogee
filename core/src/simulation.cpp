#include "core/simulation.hpp"

#include <algorithm>
#include <cmath>

#include "core/aerodynamics.hpp"
#include "core/atmosphere.hpp"
#include "core/barrowman.hpp"
#include "core/flight_phase.hpp"
#include "core/rk4_integrator.hpp"

namespace apogee::core {

namespace {
constexpr double kGravityMs2 = 9.80665;
constexpr double kPi = 3.14159265358979323846;

double degToRad(double deg) { return deg * kPi / 180.0; }

Vec3 railDirectionWorld(double angleFromVerticalDeg, double azimuthDeg) {
    const double theta = degToRad(angleFromVerticalDeg);
    const double phi = degToRad(azimuthDeg);
    return Vec3{std::sin(theta) * std::sin(phi), std::sin(theta) * std::cos(phi), std::cos(theta)};
}

// Rotation taking body +Z (0,0,1) to railDir, by the shortest arc.
Quaternion railOrientation(const Vec3& railDir) {
    const Vec3 up{0, 0, 1};
    const double cosAngle = std::clamp(up.dot(railDir), -1.0, 1.0);
    const double angle = std::acos(cosAngle);
    if (angle < 1e-9) return Quaternion::identity();
    Vec3 axis = up.cross(railDir);
    if (axis.norm() < 1e-9) axis = Vec3{1, 0, 0};  // 180-degree case, arbitrary axis
    return Quaternion::fromAxisAngle(axis, angle);
}

}  // namespace

Telemetry Simulation::run(const RocketDefinition& rocket, const MotorModel& motor,
                           const LaunchConditions& launch, const SimulationConfig& config) {
    const BarrowmanResult barrowman = computeBarrowman(rocket);
    const double halfDiameter = rocket.referenceDiameterM * 0.5;
    const double referenceAreaM2 = kPi * halfDiameter * halfDiameter;

    const Vec3 railDir = railDirectionWorld(launch.railAngleFromVerticalDeg, launch.railAzimuthDeg);

    WindField wind = launch.wind;  // local mutable copy owns this run's gust state

    FlightPhase phase = FlightPhase::OnRail;
    double burnoutTimeS = -1.0;
    bool recordedBurnout = false;
    bool apogeeSeen = false;

    auto massAndCg = [&](double t) -> std::pair<double, double> {
        const double motorMass = motor.totalMassKg(t);
        const double totalMass = rocket.dryMassKg + motorMass;
        const double cg =
            (rocket.dryMassKg * rocket.dryCgFromNoseM + motorMass * rocket.motorCgFromNoseM) /
            totalMass;
        return {totalMass, cg};
    };

    // Stashed from the most recent derivative evaluation (RK4's k4 stage,
    // essentially at the new accepted state) for telemetry/phase-transition
    // bookkeeping -- an MVP approximation, not a rigorous "at this exact
    // instant" value.
    double lastMachNumber = 0.0;
    double lastDynamicPressurePa = 0.0;
    double lastStabilityMarginCalibers = 0.0;
    double lastGForce = 0.0;

    DerivativeFn derivative = [&](const StateVector& s, double t) -> StateVector {
        const auto [totalMass, cgFromNoseM] = massAndCg(t);

        const double altitudeAglM = s.position.z;
        const double altitudeMslM = launch.launchSiteElevationM + altitudeAglM;
        const AtmosphericState atmo = standardAtmosphere(altitudeMslM);

        const Vec3 windWorld = wind.windAt(altitudeAglM);
        const Vec3 vRelWorld = s.velocity - windWorld;
        const double vRelMag = vRelWorld.norm();
        const double mach = atmo.speedOfSoundMs > 0.0 ? vRelMag / atmo.speedOfSoundMs : 0.0;
        const double dynamicPressurePa = 0.5 * atmo.densityKgM3 * vRelMag * vRelMag;

        Vec3 forceWorld{0, 0, -totalMass * kGravityMs2};
        forceWorld += s.orientation.rotate(Vec3{0, 0, motor.thrust(t)});

        Vec3 torqueBody{0, 0, 0};

        if (vRelMag > 1e-6) {
            const double cd = dragCoefficient(mach) * rocket.dragMultiplier;
            forceWorld += vRelWorld.normalized() * (-dynamicPressurePa * cd * referenceAreaM2);

            if (phase != FlightPhase::OnRail && barrowman.totalCnAlphaPerRad > 0.0) {
                const Vec3 vRelBody = s.orientation.unrotate(vRelWorld);
                const double vRelBodyMag = vRelBody.norm();
                if (vRelBodyMag > 1e-6) {
                    const Vec3 vRelBodyHat = vRelBody * (1.0 / vRelBodyMag);
                    const double cosAlpha = std::clamp(vRelBodyHat.z, -1.0, 1.0);
                    const double alpha = std::acos(cosAlpha);
                    const Vec3 perpBody = vRelBodyHat - Vec3{0, 0, 1} * cosAlpha;
                    const double perpMag = perpBody.norm();

                    if (perpMag > 1e-9) {
                        const Vec3 perpBodyHat = perpBody * (1.0 / perpMag);
                        const double normalForceMag = dynamicPressurePa * referenceAreaM2 *
                                                       barrowman.totalCnAlphaPerRad * alpha;
                        // Force points AWAY from perpBodyHat (i.e. away from
                        // the relative-wind side), not toward it: verified
                        // by hand-deriving the resulting torque direction
                        // for a tilted-nose case and a crosswind case --
                        // the +perpBodyHat sign produced a torque that
                        // rotates the nose AWAY from vRel (destabilizing,
                        // positive feedback -- confirmed by an actual
                        // exponential blowup in a crosswind simulation
                        // before this fix), not toward it.
                        const Vec3 normalForceBody = perpBodyHat * (-normalForceMag);

                        const Vec3 rCpFromCgBody{0, 0, cgFromNoseM - barrowman.centerOfPressureFromNoseM};
                        torqueBody += rCpFromCgBody.cross(normalForceBody);
                        forceWorld += s.orientation.rotate(normalForceBody);
                    }
                }

                const double finArmM = std::abs(barrowman.finCenterOfPressureFromNoseM - cgFromNoseM);
                const double kDamp = pitchYawDampingCoefficient(
                    atmo.densityKgM3, vRelMag, referenceAreaM2, barrowman.totalCnAlphaPerRad, finArmM);
                torqueBody += Vec3{-kDamp * s.angularVelocity.x, -kDamp * s.angularVelocity.y, 0.0};
            }
        }

        if (phase == FlightPhase::Drogue || phase == FlightPhase::Main) {
            const Vec3 relVelocity = s.velocity - windWorld;
            const double relSpeed = relVelocity.norm();
            if (relSpeed > 1e-6) {
                const double cd = phase == FlightPhase::Drogue ? rocket.drogueCd : rocket.mainCd;
                const double area = phase == FlightPhase::Drogue ? rocket.drogueAreaM2 : rocket.mainAreaM2;
                forceWorld += relVelocity.normalized() *
                              (-0.5 * atmo.densityKgM3 * relSpeed * relSpeed * cd * area);
            }
        }

        Vec3 accelerationWorld = forceWorld * (1.0 / totalMass);

        // Euler's equation for an axisymmetric body (I_xx=I_yy=transverse,
        // I_zz=axial, products of inertia zero -- no roll torque modeled).
        Vec3 angularAcceleration{0, 0, 0};
        const double iT = rocket.transverseMomentOfInertiaKgM2;
        const double iA = rocket.axialMomentOfInertiaKgM2;
        if (iT > 0.0 && iA > 0.0) {
            const Vec3& w = s.angularVelocity;
            angularAcceleration.x = (torqueBody.x - (iA - iT) * w.y * w.z) / iT;
            angularAcceleration.y = (torqueBody.y - (iT - iA) * w.z * w.x) / iT;
            angularAcceleration.z = torqueBody.z / iA;
        }

        Vec3 angularVelocityForOrientation = s.angularVelocity;
        if (phase == FlightPhase::OnRail) {
            // Rail-constrained: translation only along the rail axis, no
            // rotation.
            accelerationWorld = railDir * accelerationWorld.dot(railDir);
            angularAcceleration = Vec3{0, 0, 0};
            angularVelocityForOrientation = Vec3{0, 0, 0};
        }

        StateVector derivativeState;
        derivativeState.position = s.velocity;
        derivativeState.velocity = accelerationWorld;
        const Quaternion omegaQuat{0, angularVelocityForOrientation.x, angularVelocityForOrientation.y,
                                    angularVelocityForOrientation.z};
        derivativeState.orientation = s.orientation.multiply(omegaQuat) * 0.5;
        derivativeState.angularVelocity = angularAcceleration;

        lastMachNumber = mach;
        lastDynamicPressurePa = dynamicPressurePa;
        lastGForce = accelerationWorld.norm() / kGravityMs2;
        lastStabilityMarginCalibers =
            rocket.referenceDiameterM > 0.0
                ? (barrowman.centerOfPressureFromNoseM - cgFromNoseM) / rocket.referenceDiameterM
                : 0.0;

        return derivativeState;
    };

    RK4Integrator integrator;
    StateVector state;
    state.orientation = railOrientation(railDir);

    Telemetry telemetry;
    double t = 0.0;
    double lastTelemetryT = -1.0;
    double minStability = 1e9;
    bool railExitStable = false;
    double maxVelocity = 0.0, maxMach = 0.0, maxG = 0.0, maxDynamicPressure = 0.0;
    double burnoutVelocity = 0.0, burnoutAltitude = 0.0;
    double apogeeAltitudeM = 0.0, apogeeTimeS = 0.0;
    double landingTimeS = -1.0;
    double landingDescentRateMs = 0.0;

    while (t < config.maxSimTimeS && phase != FlightPhase::Landed) {
        wind.advanceGust(config.timeStepS);

        state = integrator.step(state, t, config.timeStepS, derivative);
        t += config.timeStepS;

        const double altitudeAglM = state.position.z;
        const double verticalVelocityMs = state.velocity.z;
        const double railDisplacementM = state.position.dot(railDir);
        const bool motorBurnedOut = t >= motor.burnTimeS();

        if (motorBurnedOut && !recordedBurnout) {
            burnoutTimeS = motor.burnTimeS();
            burnoutVelocity = state.velocity.norm();
            burnoutAltitude = altitudeAglM;
            recordedBurnout = true;
        }

        if (!apogeeSeen && verticalVelocityMs <= 0.0 && phase != FlightPhase::OnRail &&
            phase != FlightPhase::Boost) {
            apogeeSeen = true;
            apogeeAltitudeM = altitudeAglM;
            apogeeTimeS = t;
        }

        PhaseTransitionContext ctx;
        ctx.altitudeAglM = altitudeAglM;
        ctx.verticalVelocityMs = verticalVelocityMs;
        ctx.railDisplacementM = railDisplacementM;
        ctx.railLengthM = launch.railLengthM;
        ctx.motorBurnedOut = motorBurnedOut;
        ctx.timeSinceBurnoutS = recordedBurnout ? (t - burnoutTimeS) : 0.0;
        ctx.ejectionDelayS = config.ejectionDelayS;
        ctx.mainDeployAltitudeAglM = rocket.mainDeployAltitudeAglM;

        const FlightPhase previousPhase = phase;
        phase = computeNextPhase(phase, ctx);

        if (previousPhase == FlightPhase::OnRail && phase != FlightPhase::OnRail) {
            railExitStable = lastStabilityMarginCalibers >= 1.0;
        }
        if (phase == FlightPhase::Landed && previousPhase != FlightPhase::Landed) {
            landingTimeS = t;
            landingDescentRateMs = -verticalVelocityMs;
        }

        minStability = std::min(minStability, lastStabilityMarginCalibers);
        maxVelocity = std::max(maxVelocity, state.velocity.norm());
        maxMach = std::max(maxMach, lastMachNumber);
        maxG = std::max(maxG, lastGForce);
        maxDynamicPressure = std::max(maxDynamicPressure, lastDynamicPressurePa);

        if (t - lastTelemetryT >= config.telemetryIntervalS || phase == FlightPhase::Landed) {
            TelemetrySample sample;
            sample.timeS = t;
            sample.position = state.position;
            sample.velocity = state.velocity;
            sample.orientation = state.orientation;
            sample.angularVelocity = state.angularVelocity;
            sample.machNumber = lastMachNumber;
            sample.dynamicPressurePa = lastDynamicPressurePa;
            sample.gForce = lastGForce;
            sample.stabilityMarginCalibers = lastStabilityMarginCalibers;
            sample.phase = phase;
            telemetry.samples.push_back(sample);
            lastTelemetryT = t;
        }
    }

    SummaryStats summary;
    summary.apogeeM = apogeeAltitudeM;
    summary.apogeeTimeS = apogeeTimeS;
    summary.maxVelocityMs = maxVelocity;
    summary.maxMachNumber = maxMach;
    summary.maxAccelerationG = maxG;
    summary.maxDynamicPressurePa = maxDynamicPressure;
    summary.burnoutVelocityMs = burnoutVelocity;
    summary.burnoutAltitudeM = burnoutAltitude;
    summary.descentRateMainMs = landingDescentRateMs;
    summary.flightDurationS = landingTimeS >= 0.0 ? landingTimeS : t;
    summary.driftDistanceM = Vec3{state.position.x, state.position.y, 0}.norm();
    summary.landingOffsetM = Vec3{state.position.x, state.position.y, 0};
    summary.minStabilityMarginCalibers = minStability;
    summary.railExitStable = railExitStable;

    telemetry.summary = summary;
    return telemetry;
}

}  // namespace apogee::core
