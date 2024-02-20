
if (DETECTED_VCPKG)
    # Use vcpkg and its target
    find_package(libdwarf CONFIG REQUIRED)

    set(${libdwarf_LINK_LIBRARIES} libdwarf::dwarf)
else()
    # Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libdwarf REQUIRED IMPORTED_TARGET libdwarf)
    if (${libdwarf_FOUND})
        message("ProbeScope: FOUND libdwarf with PkgConfig")
    endif()
    include_directories(${libdwarf_INCLUDE_DIRS})
endif()
