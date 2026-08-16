# rocket_core: pure C++20 physics engine, zero Qt includes, fully
# unit-testable standalone.
add_library(rocket_core STATIC
    src/version.cpp
    src/quaternion.cpp
    src/rk4_integrator.cpp
    src/atmosphere.cpp
    src/wind_field.cpp
    src/motor_model.cpp
    src/barrowman.cpp
    src/aerodynamics.cpp
    src/flight_phase.cpp
    src/simulation.cpp
)
target_include_directories(rocket_core PUBLIC include)
target_compile_features(rocket_core PUBLIC cxx_std_20)
apogee_apply_warnings(rocket_core)
apogee_apply_static_runtime(rocket_core)
