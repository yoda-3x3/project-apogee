#include "core/rk4_integrator.hpp"

namespace apogee::core {

StateVector RK4Integrator::step(const StateVector& state, double t, double dt,
                                 const DerivativeFn& f) const {
    const StateVector k1 = f(state, t);
    const StateVector k2 = f(state + k1 * (dt * 0.5), t + dt * 0.5);
    const StateVector k3 = f(state + k2 * (dt * 0.5), t + dt * 0.5);
    const StateVector k4 = f(state + k3 * dt, t + dt);

    StateVector next = state + (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (dt / 6.0);
    next.orientation = next.orientation.normalized();
    return next;
}

}  // namespace apogee::core
