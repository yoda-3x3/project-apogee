#include "core/quaternion.hpp"

#include <cmath>

namespace apogee::core {

Quaternion Quaternion::fromAxisAngle(const Vec3& axis, double angleRad) {
    const Vec3 a = axis.normalized();
    const double halfAngle = angleRad * 0.5;
    const double s = std::sin(halfAngle);
    return {std::cos(halfAngle), a.x * s, a.y * s, a.z * s};
}

Quaternion Quaternion::multiply(const Quaternion& o) const {
    return {
        w * o.w - x * o.x - y * o.y - z * o.z,
        w * o.x + x * o.w + y * o.z - z * o.y,
        w * o.y - x * o.z + y * o.w + z * o.x,
        w * o.z + x * o.y - y * o.x + z * o.w,
    };
}

double Quaternion::norm() const { return std::sqrt(normSquared()); }

Quaternion Quaternion::normalized() const {
    const double n = norm();
    if (n <= 0.0) return Quaternion::identity();
    const double inv = 1.0 / n;
    return {w * inv, x * inv, y * inv, z * inv};
}

Vec3 Quaternion::rotate(const Vec3& v) const {
    // v' = q ⊗ (0,v) ⊗ q*, expanded via the standard closed-form to avoid
    // constructing intermediate quaternions.
    const Vec3 qv{x, y, z};
    const Vec3 t = qv.cross(v) * 2.0;
    return v + t * w + qv.cross(t);
}

}  // namespace apogee::core
