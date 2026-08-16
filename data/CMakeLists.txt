# rocket_data: Qt Core/Sql/Network dependent -- SQLite schema/repositories
# and the ThrustCurve.org / NWS / Open-Meteo HTTP clients.
add_library(rocket_data STATIC
    src/version.cpp
    src/schema.cpp
    src/database.cpp
    src/motor_repository.cpp
    src/component_repository.cpp
    src/kit_repository.cpp
    src/seed_loader.cpp
    src/network_http_transport.cpp
    src/thrustcurve_client.cpp
    src/nws_client.cpp
    src/open_meteo_client.cpp
    src/weather_service.cpp
    seed.qrc
)
target_include_directories(rocket_data PUBLIC include)
target_link_libraries(rocket_data PUBLIC rocket_core Qt6::Core Qt6::Sql Qt6::Network)
target_compile_features(rocket_data PUBLIC cxx_std_20)
apogee_apply_warnings(rocket_data)
