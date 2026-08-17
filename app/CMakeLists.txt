qt_add_executable(apogee_studio WIN32
    src/main.cpp
    src/main_window.hpp
    src/main_window.cpp
    src/theme_manager.hpp
    src/theme_manager.cpp
    src/models/rocket_design.hpp
    src/models/rocket_design.cpp
    src/panels/rocket_builder_panel.hpp
    src/panels/rocket_builder_panel.cpp
    src/panels/parts_browser_panel.hpp
    src/panels/parts_browser_panel.cpp
    src/widgets/rocket_diagram_widget.hpp
    src/widgets/rocket_diagram_widget.cpp
    resources/themes.qrc
    resources/app_icon.rc
)

target_include_directories(apogee_studio PRIVATE src)

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
