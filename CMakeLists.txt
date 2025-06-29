
cmake_minimum_required(VERSION 3.19)

################################################################################
#           Configure Options
################################################################################
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

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
set(CMAKE_CXX_STANDARD 20)

# Setup absolutely necessary Qt stuff
set(QT_VERSION_MAJOR 6)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
qm_find_qt(Core Widgets PrintSupport Svg LinguistTools)

# Import QTermWidget
set(BUILD_STATIC ON)
set(BUILD_EXAMPLES OFF)
# add_subdirectory(lib/libqtermwidget)

# Import Qt ADS
add_subdirectory(lib/Qt-Advanced-Docking-System)
set_target_properties(qtadvanceddocking-qt6 PROPERTIES
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

# Import QCustomPlot
add_subdirectory(lib/QCustomPlot)

# Import atomic_queue
find_path(ATOMIC_QUEUE_INCLUDE_DIRS "atomic_queue/atomic_queue.h")

# Build the internal probelibs
add_subdirectory(plugins)

# Add include directories
include_directories(src ui)

# Create ProbeScope executable
file(GLOB_RECURSE SOURCES
    inc/*.h

    src/*.cpp
    src/*.c
    src/*.h

    ui/*.cpp
    ui/*.h
    ui/*.ui

    res/*.qrc
    lang/*.qrc
)

add_executable(probescope WIN32)

file(GLOB_RECURSE QAS_HEADERS
    inc/serialization/*.h
)
qas_wrap_cpp(QAS_SOURCES ${QAS_HEADERS} TARGET probescope)

qm_configure_target(probescope
    SOURCES
        ${SOURCES}
        ${QAS_SOURCES}

    LINKS_PRIVATE
        # qtermwidget
        ads::qtadvanceddocking-qt6
        SingleApplication::SingleApplication
        ${libdwarf_LINK_LIBRARIES}
        ${QCUSTOMPLOT_LINK_LIBRARIES}
        unofficial::tree-sitter::tree-sitter
        WatchExprParser

    INCLUDE_PRIVATE
        inc
        inc/probelib-interface
        src/qtermwidget
        ${QASTOOL_INCLUDE_DIRS}
        ${ATOMIC_QUEUE_INCLUDE_DIRS}

    QT_LINKS Core Widgets Svg
)

if (WIN32)
    target_link_libraries(probescope PRIVATE dbghelp.lib)
endif ()

install(TARGETS probescope)

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

# Unit tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
