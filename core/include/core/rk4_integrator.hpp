#pragma once

#include <functional>

#include "core/state_vector.hpp"

namespace apogee::core {

using DerivativeFn = std::function<StateVector(const StateVector&, double t)>;

// Fixed-step classical RK4. The orientation quaternion is renormalized
// after every step to correct the drift that accumulates from treating
// dq/dt as a plain 4-vector during the intermediate stages.
class RK4Integrator {
public:
    StateVector step(const StateVector& state, double t, double dt, const DerivativeFn& f) const;
};

}  // namespace apogee::core
