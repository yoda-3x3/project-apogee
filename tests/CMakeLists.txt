add_executable(apogee_tests
    ${CMAKE_SOURCE_DIR}/vendor/catch2/catch_amalgamated.cpp
    core/test_version.cpp
)

target_include_directories(apogee_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/catch2
)

target_link_libraries(apogee_tests PRIVATE rocket_core rocket_data)

# rocket_data pulls in Qt's -DUNICODE -D_UNICODE compile definitions (needed
# for Qt's own Win32 API calls). Catch2's amalgamated main() auto-detects
# that combination and emits a wmain() entry point instead of main() -- which
# this exact MinGW 13.1.0 toolchain does not resolve to a valid CRT startup,
# failing with "undefined reference to WinMain" at link time (confirmed via
# `nm` showing a `wmain` symbol and no `main` in the built object archive).
# Keep Catch2 on its normal main().
target_compile_definitions(apogee_tests PRIVATE CATCH_CONFIG_NO_WCHAR)

apogee_apply_static_runtime(apogee_tests)

add_test(NAME apogee_tests COMMAND apogee_tests)
