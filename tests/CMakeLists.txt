add_executable(apogee_tests
    ${CMAKE_SOURCE_DIR}/vendor/catch2/catch_amalgamated.cpp
    main.cpp
    core/test_version.cpp
    core/test_quaternion.cpp
    core/test_rk4_integrator.cpp
    core/test_atmosphere.cpp
    core/test_barrowman.cpp
    core/test_motor_model.cpp
    core/test_flight_phase.cpp
    core/test_simulation_end_to_end.cpp
    data/test_schema.cpp
    data/test_seed_loader.cpp
    data/test_thrustcurve_adapter.cpp
    data/test_weather_adapters.cpp
)

target_include_directories(apogee_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/catch2
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(apogee_tests PRIVATE
    APOGEE_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

target_link_libraries(apogee_tests PRIVATE rocket_core rocket_data)

# main.cpp supplies its own main() (a QCoreApplication is required for the
# QSqlDatabase-based data-layer tests -- see main.cpp), superseding
# catch_amalgamated.cpp's default one. That default main() would otherwise
# auto-emit a wmain() entry point instead of main() on this toolchain
# whenever Qt's -DUNICODE/-D_UNICODE defines are present (confirmed via `nm`
# showing a `wmain` symbol and a "undefined reference to WinMain" link
# failure) -- CATCH_AMALGAMATED_CUSTOM_MAIN sidesteps that entirely.
target_compile_definitions(apogee_tests PRIVATE CATCH_AMALGAMATED_CUSTOM_MAIN)

apogee_apply_static_runtime(apogee_tests)

add_test(NAME apogee_tests COMMAND apogee_tests)
