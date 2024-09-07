
add_library(WatchExprParser STATIC)
qm_configure_target(WatchExprParser
    SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/parser.c

    INCLUDE_PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)