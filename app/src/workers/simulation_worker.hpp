#pragma once

#include <optional>

#include <QThread>

#include "core/motor_model.hpp"
#include "core/rocket_definition.hpp"
#include "core/simulation.hpp"
#include "core/telemetry.hpp"

namespace apogee::app {

// QThread wrapper around core::Simulation::run so a flight sim doesn't
// freeze the UI. This is a one-shot, fire-and-forget calculation with no
// need for its own event loop, so run() is overridden directly rather than
// using the worker-object/moveToThread pattern. The result is read via
// result() from QThread's own finished() signal rather than a custom
// signal carrying Telemetry by value -- that would need the type
// registered with the Qt metatype system for the cross-thread queued
// connection, which fought with moc's generated code for this
// std::vector-bearing plain struct; this sidesteps it entirely.
class SimulationWorker : public QThread {
    Q_OBJECT
public:
    explicit SimulationWorker(QObject* parent = nullptr);

    void setInputs(core::RocketDefinition rocket, core::MotorModel motor,
                   core::LaunchConditions launch, core::SimulationConfig config);

    const core::Telemetry& result() const { return result_; }

protected:
    void run() override;

private:
    core::RocketDefinition rocket_;
    std::optional<core::MotorModel> motor_;
    core::LaunchConditions launch_;
    core::SimulationConfig config_;
    core::Telemetry result_;
};

}  // namespace apogee::app
