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
# qt_standard_project_setup() only defaults CMAKE_AUTOMOC/CMAKE_AUTOUIC, not
# AUTORCC (confirmed by reading its actual implementation after a .qrc
# compiled into a plain add_library() target -- rocket_data's seed.qrc --
# silently never got processed at all: no qrc_seed.cpp generated, no
# qInitResources_seed() to link against). qt_add_executable() (used by
# app/CMakeLists.txt) handles its own .qrc sources internally regardless,
# which is why Phase 1's themes.qrc worked without this.
set(CMAKE_AUTORCC ON)

get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
get_filename_component(_qt_install_prefix "${_qt_bin_dir}" DIRECTORY)

# QSQLITE (like every Qt SQL driver) ships as a plugin, not compiled into
# Qt6Sql.dll -- confirmed the hard way: QSqlDatabase::open() segfaults deep
# inside Qt6Sql.dll when the plugin can't be found (only reproduces without
# a live QCoreApplication; see tests/main.cpp for that half of the fix).
# windeployqt deploys plugins next to a GUI target, but apogee_tests and
# apogee_seed_tool are plain console executables that never go through it.
# qt.conf is Qt's own supported mechanism for "look for plugins somewhere
# else" -- point every executable in the shared flat runtime output
# directory (set by qt_standard_project_setup() above) straight at the Qt
# kit's own plugins folder.
file(WRITE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qt.conf"
    "[Paths]\nPlugins=${_qt_install_prefix}/plugins\n")

if(APOGEE_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Svg SvgWidgets PrintSupport Concurrent)

    # windeployqt ships next to qmake in the same Qt kit bin dir.
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    include(DeployQt)
endif()

add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(tools/seed_tool)
add_subdirectory(tools/fly_demo)

if(APOGEE_BUILD_GUI)
    add_subdirectory(app)
endif()

if(APOGEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
