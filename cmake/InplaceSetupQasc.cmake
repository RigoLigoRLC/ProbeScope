find_package(qastool QUIET)

if (NOT QASTOOL_INCLUDE_DIRS)
    message(STATUS "qastool not found, building from source and deploying...")

    # Modify this variable according to your project structure
    set(_source_dir ${CMAKE_SOURCE_DIR}/host-tools/qt-json-autogen)

    # Import install function
    include("${CMAKE_SOURCE_DIR}/host-tools/qmsetup/cmake/modules/private/InstallPackage.cmake")

    # Install package in place
    set(_package_path)
    qm_install_package(qastool
        SOURCE_DIR ${_source_dir}
        BUILD_TYPE Release
        CONFIGURE_ARGS -DQAS_BUILD_EXAMPLES=OFF
        RESULT_PATH _package_path
    )
    message("Package path for qmsetup: ${_package_path}")

    # Find package again
    find_package(QasTool REQUIRED PATHS ${_package_path})

    # Update import path
    set(qastool_DIR ${_package_path} CACHE PATH "" FORCE)
endif()
