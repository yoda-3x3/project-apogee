#include <catch_amalgamated.hpp>

#include "core/quaternion.hpp"

using namespace apogee::core;

TEST_CASE("identity quaternion leaves vectors unchanged", "[core][quaternion]") {
    const Vec3 v{1, 2, 3};
    const Vec3 rotated = Quaternion::identity().rotate(v);
    CHECK(rotated.x == Catch::Approx(v.x));
    CHECK(rotated.y == Catch::Approx(v.y));
    CHECK(rotated.z == Catch::Approx(v.z));
}

TEST_CASE("90-degree rotation about Z maps +X to +Y", "[core][quaternion]") {
    const Quaternion q = Quaternion::fromAxisAngle(Vec3{0, 0, 1}, 3.14159265358979323846 / 2.0);
    const Vec3 rotated = q.rotate(Vec3{1, 0, 0});
    CHECK(rotated.x == Catch::Approx(0.0).margin(1e-9));
    CHECK(rotated.y == Catch::Approx(1.0).margin(1e-9));
    CHECK(rotated.z == Catch::Approx(0.0).margin(1e-9));
}

TEST_CASE("unrotate is the inverse of rotate for a unit quaternion", "[core][quaternion]") {
    const Quaternion q = Quaternion::fromAxisAngle(Vec3{1, 1, 0}, 0.73);
    const Vec3 v{4, -2, 7};
    const Vec3 roundTrip = q.unrotate(q.rotate(v));
    CHECK(roundTrip.x == Catch::Approx(v.x).margin(1e-9));
    CHECK(roundTrip.y == Catch::Approx(v.y).margin(1e-9));
    CHECK(roundTrip.z == Catch::Approx(v.z).margin(1e-9));
}

TEST_CASE("normalized() produces a unit quaternion", "[core][quaternion]") {
    const Quaternion q{2, 0, 0, 0};
    const Quaternion n = q.normalized();
    CHECK(n.norm() == Catch::Approx(1.0));
    CHECK(n.w == Catch::Approx(1.0));
}

TEST_CASE("a quaternion composed with its conjugate has zero vector part", "[core][quaternion]") {
    const Quaternion q = Quaternion::fromAxisAngle(Vec3{0, 1, 0}, 1.2).normalized();
    const Quaternion product = q.multiply(q.conjugate());
    CHECK(product.w == Catch::Approx(1.0).margin(1e-9));
    CHECK(product.x == Catch::Approx(0.0).margin(1e-9));
    CHECK(product.y == Catch::Approx(0.0).margin(1e-9));
    CHECK(product.z == Catch::Approx(0.0).margin(1e-9));
}
