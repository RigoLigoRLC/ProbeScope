
cmake_minimum_required(VERSION 3.19)

project(probescope)

# Add AddressSanitizer on MSVC Debug
if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("MSVC AddressSanitizer enabled")
    add_compile_options(/fsanitize=address)
endif()

# Setup host tools
include(cmake/DetectIsVcpkgToolchain.cmake)
include(cmake/InplaceSetupQmsetup.cmake)
include(cmake/InplaceSetupQasc.cmake)
add_subdirectory(cmake/corrosion)

# Set C++ standard  
set(CMAKE_CXX_STANDARD 17)

# Setup absolutely necessary Qt stuff
set(QT_VERSION_MAJOR 5)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
qm_find_qt(Core Widgets Svg LinguistTools)

# Import QTermWidget
set(BUILD_STATIC ON)
set(BUILD_EXAMPLES OFF)
add_subdirectory(lib/libqtermwidget)

# Import Qt ADS
add_subdirectory(lib/Qt-Advanced-Docking-System)
set_target_properties(qt5advanceddocking PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
)

# Import Qt Single Application library
set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")
add_subdirectory(lib/singleapplication)

# Import libdwarf
include(cmake/ImportLibdwarf.cmake)

# Import tree-sitter
find_package(unofficial-tree-sitter CONFIG REQUIRED)
add_subdirectory(lib/WatchExprParser)

# Build the internal probelibs
add_subdirectory(plugins)

# Add include directories
include_directories(src ui)

# Create ProbeScope executable
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.c
    src/*.h

    ui/*.cpp
    ui/*.h
    ui/*.ui

    res/*.qrc
    lang/*.qrc
)
add_executable(probescope)

qm_configure_target(probescope
    SOURCES ${SOURCES}
    LINKS_PRIVATE
        qtermwidget
        ads::qt5advanceddocking
        SingleApplication::SingleApplication
        ${libdwarf_LINK_LIBRARIES}
        unofficial::tree-sitter::tree-sitter
        WatchExprParser

    INCLUDE_PRIVATE
        inc
        inc/probelib-interface
        src/qtermwidget

    QT_LINKS Core Widgets Svg
)

# Deal with translations
qm_import(Translate)
qm_add_translation(probescope_translate
    LOCALES
        zh_CN
    PREFIX probescope
    # TARGETS probescope
    SOURCES ${SOURCES}
    TS_DIR ${CMAKE_CURRENT_LIST_DIR}/lang
    QM_DIR ${CMAKE_CURRENT_LIST_DIR}/lang/publish
    QM_DEPENDS probescope
)
