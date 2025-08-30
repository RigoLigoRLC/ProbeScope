
# Because on Windows/macOS qt_generate_deploy_app_script doesn't deploy other dependencies
# FIXME: macOS is not tested
if(WIN32)
    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR deps
            UNRESOLVED_DEPENDENCIES_VAR not_found
            EXECUTABLES \$<TARGET_FILE:probescope>
            POST_EXCLUDE_REGEXES \".*system32/.*\\\\.dll\"
        )
        foreach(dep \${deps})
            file(INSTALL
                DESTINATION \"\${CMAKE_INSTALL_PREFIX}\"
                TYPE FILE
                FILES \"\${dep}\"
            )
        endforeach()
    ")
endif()

# QADS installs all of its headers and libs to our folder which is unwanted
# They provide no way to turn it off so we had to do it manually
install(CODE "file(REMOVE_RECURSE
        ${CMAKE_INSTALL_PREFIX}/include/qtadvanceddocking-qt6
        ${CMAKE_INSTALL_PREFIX}/lib/cmake/qtadvanceddocking-qt6
        ${CMAKE_INSTALL_PREFIX}/lib/qtadvanceddocking-qt6_static.lib
    )"
)

if(WIN32)
    set(deploy_tool_options_arg --no-compiler-runtime)
endif()

set(QT_DEPLOY_BIN_DIR .)
qt_generate_deploy_app_script(
    TARGET probescope
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
)
install(SCRIPT ${deploy_script})
