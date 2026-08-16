#include <catch_amalgamated.hpp>

#include <cmath>

#include "core/rk4_integrator.hpp"

using namespace apogee::core;

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TEST_CASE("RK4Integrator integrates a simple harmonic oscillator accurately", "[core][rk4]") {
    // dx/dt = v, dv/dt = -k*x (position/velocity.x fields only; orientation
    // and angular velocity are left at their defaults and simply carried
    // through unchanged since their derivative is zero here).
    constexpr double k = 4.0;  // angular frequency omega = sqrt(k) = 2 rad/s
    const double omega = std::sqrt(k);
    const double x0 = 1.0;

    DerivativeFn f = [](const StateVector& s, double) -> StateVector {
        StateVector d;
        d.position = s.velocity;
        d.velocity = Vec3{-k * s.position.x, 0, 0};
        return d;
    };

    StateVector state;
    state.position = Vec3{x0, 0, 0};

    RK4Integrator integrator;
    const double dt = 0.001;
    const double quarterPeriod = (kPi / 2.0) / omega;  // time to reach x=0, v=-x0*omega
    int steps = static_cast<int>(quarterPeriod / dt);

    double t = 0.0;
    for (int i = 0; i < steps; ++i) {
        state = integrator.step(state, t, dt, f);
        t += dt;
    }

    // Analytic solution: x(t) = x0*cos(omega*t), v(t) = -x0*omega*sin(omega*t).
    CHECK(state.position.x == Catch::Approx(x0 * std::cos(omega * t)).margin(1e-4));
    CHECK(state.velocity.x == Catch::Approx(-x0 * omega * std::sin(omega * t)).margin(1e-4));

    // Energy conservation as an independent cross-check.
    const double energy = 0.5 * state.velocity.x * state.velocity.x + 0.5 * k * state.position.x * state.position.x;
    const double initialEnergy = 0.5 * k * x0 * x0;
    CHECK(energy == Catch::Approx(initialEnergy).margin(1e-6));
}

TEST_CASE("RK4Integrator renormalizes orientation after each step", "[core][rk4]") {
    DerivativeFn f = [](const StateVector& s, double) -> StateVector {
        StateVector d;
        // Constant angular velocity about Z (body frame == world frame here
        // since we start at identity orientation).
        const Quaternion omegaQuat{0, 0, 0, 1.0};
        d.orientation = s.orientation.multiply(omegaQuat) * 0.5;
        return d;
    };

    StateVector state;
    RK4Integrator integrator;
    double t = 0.0;
    for (int i = 0; i < 1000; ++i) {
        state = integrator.step(state, t, 0.01, f);
        t += 0.01;
        CHECK(state.orientation.norm() == Catch::Approx(1.0).margin(1e-9));
    }
}
