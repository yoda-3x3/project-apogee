# Deploys the Qt6 runtime, platform plugins, and MinGW runtime DLLs next to
# a Qt GUI target so the built exe is standalone on a plain Windows machine.
function(apogee_deploy_qt target)
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(WARNING "windeployqt not found; ${target} will not be self-contained")
        return()
    endif()
    # windeployqt --mingw locates the MinGW runtime DLLs via PATH, so point
    # it at the exact compiler CMake resolved rather than trusting ambient
    # PATH -- same "pin the exact toolchain" approach build.ps1 uses.
    get_filename_component(_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    # A literal ';' in a COMMAND argument is CMake's list separator, not a
    # PATH separator -- it silently splits this string into extra tokens.
    # $<SEMICOLON> survives to the actual command line intact.
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${_mingw_bin_dir}$<SEMICOLON>$ENV{PATH}"
                ${WINDEPLOYQT_EXECUTABLE}
                --compiler-runtime
                --no-translations
                --no-system-d3d-compiler
                --no-system-dxc-compiler
                $<TARGET_FILE:${target}>
        COMMENT "windeployqt: deploying Qt runtime + plugins next to ${target}"
    )
endfunction()
