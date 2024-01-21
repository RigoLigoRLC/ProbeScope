
file(GLOB_RECURSE SOURCES *.cpp *.h)
file(GLOB_RECURSE UIFILES *.ui)
file(GLOB_RECURSE RESFILES *.qrc)

# Add library
add_library(qtermwidget STATIC)
target_include_directories(qtermwidget PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/autogen
)
qm_configure_target(qtermwidget
    SOURCES ${SOURCES} ${UIFILES} ${RESFILES}

    DEFINES_PRIVATE
        "KB_LAYOUT_DIR=\":/qtermwidget/kblayouts/\""
        "COLORSCHEMES_DIR=\":/qtermwidget/color-schemes/\""
        "TRANSLATIONS_DIR=\":/qtermwidget/translations/\""
    
    INCLUDE_PRIVATE

    QT_LINKS Core Gui Widgets
)

# Deal with translations
qm_import(Translate)
qm_add_translation(qtermwidget_translate
    LOCALES
        ar arn ast bg ca cs cy da de_CH de el es et fi fr gl
        he hr hu it ja ko lt nb_NO nl oc pl pt_BR pt ru si sk
        tr uk zh_CN zh_TW
    PREFIX qtermwidget
    TARGETS qtermwidget
    TS_FILES ${QM_FILES}
    TS_DIR ${CMAKE_CURRENT_LIST_DIR}/translations
    QM_DIR ${CMAKE_CURRENT_LIST_DIR}/translations/deploy
    QM_DEPENDS qtermwidget
)

# And this export header thingy
include(GenerateExportHeader)
generate_export_header(qtermwidget
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/autogen/qtermwidget_export.h"
    BASE_NAME QTERMWIDGET
)

# And this version header thingy
set(QTERMWIDGET_VERSION_MAJOR "1")
set(QTERMWIDGET_VERSION_MINOR "4")
set(QTERMWIDGET_VERSION_PATCH "0")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/qtermwidget_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/autogen/qtermwidget_version.h"
    @ONLY
)
