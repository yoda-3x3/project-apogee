# rocket_core: pure C++20 physics engine, zero Qt includes, fully
# unit-testable standalone (Phase 3 fills this in: vec3/quaternion/rk4/
# atmosphere/barrowman/motor_model/simulation).
add_library(rocket_core STATIC
    src/version.cpp
)
target_include_directories(rocket_core PUBLIC include)
target_compile_features(rocket_core PUBLIC cxx_std_20)
apogee_apply_warnings(rocket_core)
apogee_apply_static_runtime(rocket_core)
