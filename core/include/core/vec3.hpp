#pragma once

#include <cmath>

namespace apogee::core {

// Plain 3D vector. Used both for physical quantities (ENU world frame,
// body frame) and as a generic 3-tuple wherever one is convenient (e.g. a
// raw torque or angular-acceleration value) -- no unit system is baked in,
// callers keep units consistent (this project uses SI throughout).
struct Vec3 {
    double x = 0;
    double y = 0;
    double z = 0;

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

    double dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    double normSquared() const { return dot(*this); }
    double norm() const { return std::sqrt(normSquared()); }
    Vec3 normalized() const {
        const double n = norm();
        return n > 0.0 ? (*this) * (1.0 / n) : Vec3{0, 0, 0};
    }
};

inline Vec3 operator*(double s, const Vec3& v) { return v * s; }

}  // namespace apogee::core
