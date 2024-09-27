
add_library(probelib-psprobe SHARED)

# Import libpsprobe
corrosion_import_crate(MANIFEST_PATH ${CMAKE_SOURCE_DIR}/lib/libpsprobe/Cargo.toml)
target_include_directories(psprobe INTERFACE ${CMAKE_SOURCE_DIR}/lib/libpsprobe/inc)

# FIXME:

set_target_properties(psprobe PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
)

FILE(GLOB_RECURSE SOURCES *.cpp)
qm_configure_target(probelib-psprobe
    SOURCES ${SOURCES}
    
    LINKS_PRIVATE
        psprobe # Rust part
        probelib-includes
    
    QT_LINKS Core
)

install(TARGETS probelib-psprobe LIBRARY DESTINATION probelibs)
