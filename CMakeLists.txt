
cmake_minimum_required(VERSION 3.19)

project(`)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Widgets Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Svg REQUIRED)

# Setup host tools
include(cmake/InplaceSetupQmsetup.cmake)
add_subdirectory(cmake/corrosion)

# Import libpsprobe
corrosion_import_crate(MANIFEST_PATH lib/libpsprobe/Cargo.toml)
target_include_directories(psprobe INTERFACE lib/libpsprobe/inc)

# Create ProbeScope executable
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
add_executable(probescope ${SOURCES})

target_link_libraries(probescope PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    psprobe
)
