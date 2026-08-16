#pragma once

#include "core/quaternion.hpp"
#include "core/vec3.hpp"

namespace apogee::core {

// The 13-scalar ODE state (3 position + 3 velocity + 4 orientation +
// 3 angular velocity). Mass is deliberately not part of the state -- it's a
// deterministic lookup from burn time via MotorModel, avoiding an extra
// coupled ODE state for something that isn't actually independent.
//
// Also used, with the exact same shape, to hold RK4's intermediate
// derivative values: derivative.orientation there is dq/dt, not a unit
// quaternion -- see RK4Integrator.
struct StateVector {
    Vec3 position;             // m, launch-site ENU, origin = rail base
    Vec3 velocity;              // m/s, ENU
    Quaternion orientation = Quaternion::identity();  // unit quaternion, body->world
    Vec3 angularVelocity;       // rad/s, body frame

    StateVector operator+(const StateVector& o) const {
        return {position + o.position, velocity + o.velocity, orientation + o.orientation,
                angularVelocity + o.angularVelocity};
    }
    StateVector operator*(double s) const {
        return {position * s, velocity * s, orientation * s, angularVelocity * s};
    }
};

}  // namespace apogee::core
