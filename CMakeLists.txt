
cmake_minimum_required(VERSION 3.19)

project(probescope)

# Setup host tools
include(cmake/InplaceSetupQmsetup.cmake)
include(cmake/InplaceSetupQasc.cmake)
add_subdirectory(cmake/corrosion)

# Setup absolutely necessary Qt stuff
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
qm_find_qt(LinguistTools)

# Import libpsprobe
corrosion_import_crate(MANIFEST_PATH lib/libpsprobe/Cargo.toml)
target_include_directories(psprobe INTERFACE lib/libpsprobe/inc)

# Import QTermWidget
add_subdirectory(lib/libqtermwidget)

# Create ProbeScope executable
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c src/*.h)
add_executable(probescope)

qm_configure_target(probescope
    SOURCES ${SOURCES}
    LINKS_PRIVATE psprobe qtermwidget
    INCLUDE_PRIVATE inc src/qtermwidget

    QT_LINKS Core Widgets Svg
)
