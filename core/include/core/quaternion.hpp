#pragma once

#include "core/vec3.hpp"

namespace apogee::core {

// Hamilton convention, w + xi + yj + zk. Represents body->world orientation
// throughout this codebase. Also doubles as a plain 4-tuple for RK4's
// intermediate derivative values (dq/dt is not itself a unit quaternion --
// see RK4Integrator), which is why arithmetic operators are provided
// alongside the rotation-specific methods.
struct Quaternion {
    double w = 1;
    double x = 0;
    double y = 0;
    double z = 0;

    static Quaternion identity() { return {1, 0, 0, 0}; }
    static Quaternion fromAxisAngle(const Vec3& axis, double angleRad);

    Quaternion operator+(const Quaternion& o) const { return {w + o.w, x + o.x, y + o.y, z + o.z}; }
    Quaternion operator*(double s) const { return {w * s, x * s, y * s, z * s}; }

    // Hamilton product (this ⊗ o), NOT commutative.
    Quaternion multiply(const Quaternion& o) const;

    double normSquared() const { return w * w + x * x + y * y + z * z; }
    double norm() const;
    Quaternion normalized() const;
    Quaternion conjugate() const { return {w, -x, -y, -z}; }

    // Rotates v from body frame to world frame (this quaternion is the
    // body->world rotation).
    Vec3 rotate(const Vec3& v) const;

    // Inverse rotation: world frame to body frame. Equivalent to
    // conjugate().rotate(v) for a unit quaternion.
    Vec3 unrotate(const Vec3& v) const { return conjugate().rotate(v); }
};

}  // namespace apogee::core
