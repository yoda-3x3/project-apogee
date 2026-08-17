#include "workers/simulation_worker.hpp"

#include <utility>

namespace apogee::app {

SimulationWorker::SimulationWorker(QObject* parent) : QThread(parent) {}

void SimulationWorker::setInputs(core::RocketDefinition rocket, core::MotorModel motor,
                                  core::LaunchConditions launch, core::SimulationConfig config) {
    rocket_ = rocket;
    motor_ = std::move(motor);
    launch_ = launch;
    config_ = config;
}

void SimulationWorker::run() {
    if (!motor_) return;
    result_ = core::Simulation::run(rocket_, *motor_, launch_, config_);
}

}  // namespace apogee::app
