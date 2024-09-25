
add_library(probelib-psprobe SHARED)

FILE(GLOB_RECURSE SOURCES *.cpp)
qm_configure_target("probelib-psprobe"
    SOURCES ${SOURCES}
    
    LINKS_PRIVATE
        probelib-includes
    
    QT_LINKS Core
)

install(TARGETS probelib-psprobe LIBRARY DESTINATION probelibs)
