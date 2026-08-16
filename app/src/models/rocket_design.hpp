#pragma once

#include <optional>

#include <QObject>

#include "core/rocket_definition.hpp"
#include "data/component_types.hpp"
#include "data/thrustcurve_types.hpp"

namespace apogee::app {

struct StabilityInfo {
    bool hasMinimumParts = false;  // true once nose cone + body tube + fin set are all selected
    double totalMassKg = 0;        // loaded (with motor, if selected) -- the safety-relevant figure
    double cgFromNoseM = 0;
    double cpFromNoseM = 0;
    double marginCalibers = 0;
    double totalLengthM = 0;
    core::RocketDefinition definition;  // usable directly by RocketDiagramWidget / later phases
};

// The rocket currently being assembled in the GUI: up to one selected part
// per slot (nose cone, body tube, fin set, motor mount, recovery) plus an
// optional motor, mirroring how every seeded kit is structured (exactly one
// of each). Derives a core::RocketDefinition via a simple linear-stacking
// assumption -- see rocket_design.cpp -- and the resulting CG/CP/stability
// margin, recomputed on demand (computeStability() is cheap: no simulation
// involved, just Barrowman + a mass-weighted CG sum).
class RocketDesign : public QObject {
    Q_OBJECT
public:
    explicit RocketDesign(QObject* parent = nullptr);

    void setNoseCone(const std::optional<data::ComponentWithDetail>& c);
    void setBodyTube(const std::optional<data::ComponentWithDetail>& c);
    void setFinSet(const std::optional<data::ComponentWithDetail>& c);
    void setMotorMount(const std::optional<data::ComponentWithDetail>& c);
    void setRecovery(const std::optional<data::ComponentWithDetail>& c);
    void setMotor(const std::optional<data::MotorSummary>& m);

    const std::optional<data::ComponentWithDetail>& noseCone() const { return noseCone_; }
    const std::optional<data::ComponentWithDetail>& bodyTube() const { return bodyTube_; }
    const std::optional<data::ComponentWithDetail>& finSet() const { return finSet_; }
    const std::optional<data::ComponentWithDetail>& motorMount() const { return motorMount_; }
    const std::optional<data::ComponentWithDetail>& recovery() const { return recovery_; }
    const std::optional<data::MotorSummary>& motor() const { return motor_; }

    StabilityInfo computeStability() const;

signals:
    void changed();

private:
    std::optional<data::ComponentWithDetail> noseCone_;
    std::optional<data::ComponentWithDetail> bodyTube_;
    std::optional<data::ComponentWithDetail> finSet_;
    std::optional<data::ComponentWithDetail> motorMount_;
    std::optional<data::ComponentWithDetail> recovery_;
    std::optional<data::MotorSummary> motor_;
};

}  // namespace apogee::app
