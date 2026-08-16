cmake_minimum_required(VERSION 3.24)
project(project_apogee CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(APOGEE_BUILD_TESTS "Build Catch2 unit tests" ON)
option(APOGEE_BUILD_GUI "Build the Qt6 apogee_studio GUI target" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Warnings)
include(StaticRuntime)

# The data layer (SQLite + JSON + HTTP) always needs Qt Core/Sql/Network,
# independent of whether the Widgets GUI is being built -- Phase 2's demo is
# a headless console tool that exercises the data layer with no GUI at all.
find_package(Qt6 REQUIRED COMPONENTS Core Sql Network)
qt_standard_project_setup()

if(APOGEE_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Svg SvgWidgets PrintSupport Concurrent)

    # windeployqt ships next to qmake in the same Qt kit bin dir.
    get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    include(DeployQt)
endif()

add_subdirectory(core)
add_subdirectory(data)

if(APOGEE_BUILD_GUI)
    add_subdirectory(app)
endif()

if(APOGEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
