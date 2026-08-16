# Static linking for non-Qt targets (core, tests): zero MinGW runtime DLL
# dependency for the pure-C++ physics engine. The Qt GUI target stays
# dynamically linked against Qt (see app/CMakeLists.txt) and is deployed via
# windeployqt instead (see DeployQt.cmake).
#
# Also explicitly requests the console subsystem (-mconsole). CMake normally
# infers this from the WIN32_EXECUTABLE target property, but the Qt-bundled
# MinGW 13.1.0 toolchain (Qt/Tools/mingw1310_64) doesn't get an explicit
# -mconsole/-mwindows flag from CMake's generated link line here, and its
# ld defaults to the GUI (WinMain) CRT startup instead of console (main) --
# confirmed via a real "undefined reference to WinMain" link failure on a
# plain add_executable() console target. -mconsole is a no-op (safely
# ignored) when applied to a static library target like rocket_core.
function(apogee_apply_static_runtime target)
    if(MINGW)
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++ -mconsole)
    endif()
endfunction()
