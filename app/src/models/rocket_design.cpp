#include "models/rocket_design.hpp"

#include <algorithm>
#include <cmath>

#include "core/barrowman.hpp"

namespace apogee::app {

namespace {
constexpr double kPi = 3.14159265358979323846;
double mmToM(double mm) { return mm / 1000.0; }
double gToKg(double g) { return g / 1000.0; }
}  // namespace

RocketDesign::RocketDesign(QObject* parent) : QObject(parent) {}

void RocketDesign::setNoseCone(const std::optional<data::ComponentWithDetail>& c) {
    noseCone_ = c;
    emit changed();
}
void RocketDesign::setBodyTube(const std::optional<data::ComponentWithDetail>& c) {
    bodyTube_ = c;
    emit changed();
}
void RocketDesign::setFinSet(const std::optional<data::ComponentWithDetail>& c) {
    finSet_ = c;
    emit changed();
}
void RocketDesign::setMotorMount(const std::optional<data::ComponentWithDetail>& c) {
    motorMount_ = c;
    emit changed();
}
void RocketDesign::setRecovery(const std::optional<data::ComponentWithDetail>& c) {
    recovery_ = c;
    emit changed();
}
void RocketDesign::setMotor(const std::optional<data::MotorSummary>& m) {
    motor_ = m;
    emit changed();
}

StabilityInfo RocketDesign::computeStability() const {
    StabilityInfo info;
    if (!noseCone_ || !bodyTube_ || !finSet_) return info;  // hasMinimumParts stays false
    info.hasMinimumParts = true;

    core::RocketDefinition def;
    def.referenceDiameterM = mmToM(bodyTube_->bodyTube.outerDiameterMm);
    def.noseShape =
        noseCone_->noseCone.shape == "conical" ? core::NoseShape::Conical : core::NoseShape::Ogive;
    def.noseLengthM = mmToM(noseCone_->noseCone.lengthMm);

    def.finCount = finSet_->finSet.finCount;
    def.finRootChordM = mmToM(finSet_->finSet.rootChordMm);
    def.finTipChordM = mmToM(finSet_->finSet.tipChordMm);
    def.finSemiSpanM = mmToM(finSet_->finSet.semiSpanMm);
    def.finSweepLengthM = mmToM(finSet_->finSet.sweepLengthMm);

    // Linear-stacking assumption, matching how every seeded kit is actually
    // built: the nose's shoulder inserts into the body tube, so the tube's
    // front face sits at (noseLength - shoulderLength); fins and the motor
    // mount sit flush with the tail. Each component's own center of mass is
    // approximated as the midpoint of its axial extent -- a simplification
    // (real nose cones/fin sets aren't uniform density), acceptable for an
    // MVP live readout.
    const double noseShoulderM = mmToM(noseCone_->noseCone.shoulderLengthMm);
    const double bodyTubeFrontFaceM = def.noseLengthM - noseShoulderM;
    const double bodyTubeLengthM = mmToM(bodyTube_->bodyTube.lengthMm);
    const double bodyTubeEndM = bodyTubeFrontFaceM + bodyTubeLengthM;

    def.finRootLeadingEdgeFromNoseM = bodyTubeEndM - def.finRootChordM;

    double dryMass = 0.0;
    double dryMassMoment = 0.0;
    auto addDryComponent = [&](double massKg, double cgM) {
        dryMass += massKg;
        dryMassMoment += massKg * cgM;
    };

    addDryComponent(gToKg(noseCone_->summary.massG), def.noseLengthM / 2.0);
    addDryComponent(gToKg(bodyTube_->summary.massG), bodyTubeFrontFaceM + bodyTubeLengthM / 2.0);
    addDryComponent(gToKg(finSet_->summary.massG), def.finRootLeadingEdgeFromNoseM + def.finRootChordM / 2.0);
    if (motorMount_) {
        const double mountLengthM = mmToM(motorMount_->motorMount.mountLengthMm);
        addDryComponent(gToKg(motorMount_->summary.massG), bodyTubeEndM - mountLengthM / 2.0);
    }
    if (recovery_) {
        // Packed near the front of the body tube, ahead of the motor mount.
        addDryComponent(gToKg(recovery_->summary.massG), bodyTubeFrontFaceM + bodyTubeLengthM * 0.1);
    }

    def.dryMassKg = dryMass;
    def.dryCgFromNoseM = dryMass > 0.0 ? dryMassMoment / dryMass : 0.0;

    // The motor sits at the tail, inside the motor mount if one is
    // selected, else approximated flush with the tail using the motor's
    // own length.
    double loadedMass = dryMass;
    double loadedMassMoment = dryMassMoment;
    if (motor_) {
        const double motorLengthM =
            motorMount_ ? mmToM(motorMount_->motorMount.mountLengthMm) : mmToM(motor_->lengthMm);
        def.motorCgFromNoseM = bodyTubeEndM - motorLengthM / 2.0;
        const double motorMassKg = gToKg(motor_->totalWeightG);
        loadedMass += motorMassKg;
        loadedMassMoment += motorMassKg * def.motorCgFromNoseM;
    } else {
        def.motorCgFromNoseM = bodyTubeEndM;  // no motor selected; unused since totalMassKg has no motor term
    }

    const double halfDiameter = def.referenceDiameterM / 2.0;
    const double inertiaMass = std::max(loadedMass, 0.001);  // guard div-by-zero with nothing selected yet
    def.transverseMomentOfInertiaKgM2 =
        inertiaMass * (3.0 * halfDiameter * halfDiameter + bodyTubeEndM * bodyTubeEndM) / 12.0;
    def.axialMomentOfInertiaKgM2 = inertiaMass * halfDiameter * halfDiameter / 2.0;

    if (recovery_) {
        if (recovery_->summary.type == "parachute") {
            def.drogueCd = def.mainCd = recovery_->parachute.cd;
            const double r = mmToM(recovery_->parachute.diameterMm) / 2.0;
            def.drogueAreaM2 = def.mainAreaM2 = kPi * r * r;
        } else if (recovery_->summary.type == "streamer") {
            def.drogueCd = def.mainCd = recovery_->streamer.cd;
            def.drogueAreaM2 = def.mainAreaM2 = mmToM(recovery_->streamer.lengthMm) * mmToM(recovery_->streamer.widthMm);
        }
    }
    // Single-deployment MVP (matches every seeded kit): same recovery device
    // for both stages, deployed at the ejection charge, no separate main
    // deploy altitude.
    def.mainDeployAltitudeAglM = 1.0e6;

    info.definition = def;
    info.totalLengthM = bodyTubeEndM;
    info.totalMassKg = loadedMass;
    info.cgFromNoseM = loadedMass > 0.0 ? loadedMassMoment / loadedMass : 0.0;

    const core::BarrowmanResult barrowman = core::computeBarrowman(def);
    info.cpFromNoseM = barrowman.centerOfPressureFromNoseM;
    info.marginCalibers =
        def.referenceDiameterM > 0.0 ? (info.cpFromNoseM - info.cgFromNoseM) / def.referenceDiameterM : 0.0;

    return info;
}

}  // namespace apogee::app
