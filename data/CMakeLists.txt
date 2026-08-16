# rocket_data: Qt Core/Sql/Network dependent -- SQLite schema/repositories
# and the ThrustCurve.org / Open-Meteo HTTP clients (Phase 2 fills this in).
add_library(rocket_data STATIC
    src/version.cpp
)
target_include_directories(rocket_data PUBLIC include)
target_link_libraries(rocket_data PUBLIC rocket_core Qt6::Core Qt6::Sql Qt6::Network)
target_compile_features(rocket_data PUBLIC cxx_std_20)
apogee_apply_warnings(rocket_data)
