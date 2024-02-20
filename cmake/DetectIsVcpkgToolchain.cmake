
string(REGEX MATCH "vcpkg\\.cmake\$" _DETECT_VCPKG_STRING "${CMAKE_TOOLCHAIN_FILE}")
if (NOT _DETECT_VCPKG_STRING)
    set(DETECTED_VCPKG FALSE)
    message("ProbeScope: Project is NOT using vcpkg")
else()
    set(DETECTED_VCPKG TRUE)
    message("ProbeScope: Project IS using vcpkg")
endif()
