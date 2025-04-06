
add_library(QCustomPlot STATIC)

qm_configure_target(QCustomPlot
    SOURCES qcustomplot.cpp inc/qcustomplot.h
    INCLUDE_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc
    QT_LINKS Core Gui Widgets PrintSupport
)
target_include_directories(QCustomPlot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

set(QCUSTOMPLOT_LINK_LIBRARIES QCustomPlot PARENT_SCOPE)
