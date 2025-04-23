
add_library(probelib-includes INTERFACE)
target_include_directories(probelib-includes INTERFACE
    ${CMAKE_SOURCE_DIR}/inc
)

# ===== Add plugins below =====

add_subdirectory(probelib-psprobe)

# ===== Add plugins above =====

qm_collect_targets(BUILT_PLUGINS DIRECTORY ${CMAKE_CURRENT_LIST_DIR} SHARED)
foreach(plugin ${BUILT_PLUGINS})
    get_target_property(LINK_LIBS ${plugin} LINK_LIBRARIES)
    if(LINK_LIBS)
        list(FIND LINK_LIBS probelib-includes index)
        if(NOT index EQUAL -1)
            set_target_properties(${plugin} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
            )
            # TODO: This may need a change on macOS and Linux targets
            install(TARGETS ${plugin} DESTINATION ${CMAKE_INSTALL_PREFIX}/probelibs/)
            message(STATUS "Plugin ${plugin} has been configured as a probelib.")
        endif()
    endif()
endforeach()
