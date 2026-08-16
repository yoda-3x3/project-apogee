qt_add_executable(apogee_studio WIN32
    src/main.cpp
    src/main_window.hpp
    src/main_window.cpp
    src/theme_manager.hpp
    src/theme_manager.cpp
    resources/themes.qrc
)

target_link_libraries(apogee_studio PRIVATE
    rocket_core
    rocket_data
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
    Qt6::SvgWidgets
)
target_compile_features(apogee_studio PUBLIC cxx_std_20)
apogee_apply_warnings(apogee_studio)

apogee_deploy_qt(apogee_studio)
