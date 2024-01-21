find_package(qmsetup QUIET)

if (NOT TARGET qmsetup::library)

    message(STATUS "qastool not found, building from source and deploying...")

    # Modify this variable according to your project structure
    set(_source_dir ${CMAKE_SOURCE_DIR}/host-tools/qmsetup)

    # Import install function
    include("${_source_dir}/cmake/modules/InstallPackage.cmake")

    # Install package in place
    set(_package_path)
    qm_install_package(qmsetup
        SOURCE_DIR ${_source_dir}
        BUILD_TYPE Release
        RESULT_PATH _package_path
    )

    # Find package again
    find_package(qmsetup REQUIRED PATHS ${_package_path})

    # Update import path
    set(qmsetup_DIR ${_package_path} CACHE PATH "" FORCE)
endif()
