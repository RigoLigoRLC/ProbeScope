
add_library(probelib-psprobe SHARED)

# Import libpsprobe
corrosion_import_crate(MANIFEST_PATH ${CMAKE_SOURCE_DIR}/lib/libpsprobe/Cargo.toml)
target_include_directories(psprobe INTERFACE ${CMAKE_SOURCE_DIR}/lib/libpsprobe/inc)

# FIXME:

set_target_properties(psprobe PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/probelibs"
)

# Due to https://github.com/corrosion-rs/corrosion/issues/64 , this doesn't work yet
# corrosion_install(TARGETS psprobe DESTINATION probelibs)

# You need to copy it over manually
install(CODE
    "file(COPY
        \"${CMAKE_BINARY_DIR}/probelibs/psprobe.dll\"
        DESTINATION \"${CMAKE_INSTALL_PREFIX}/probelibs\"
    )"
)

FILE(GLOB_RECURSE SOURCES *.cpp)
qm_configure_target(probelib-psprobe
    SOURCES ${SOURCES}
    
    LINKS_PRIVATE
        psprobe # Rust part
        probelib-includes
    
    QT_LINKS Core
)
