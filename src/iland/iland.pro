# -------------------------------------------------
# iLand project file (GUI version). See iland-model.org
# -------------------------------------------------

include(../iland_core.pri)

QT += widgets
# charts for the function plotter
QT += charts
# quick: for QML based user interface
QT += quick

TARGET = iland
TEMPLATE = app
CONFIG += precompile_header

INCLUDEPATH += ../iland

# Handle application icons
macx {
    ICON = res/iLand.icns
    LIBS -= -framework AGL
    QMAKE_LIBS_OPENGL -= -framework AGL
}
win32 {
    RC_ICONS = res/iLand.ico
}

# This is the UI version of iLand!
DEFINES += ILAND_GUI

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    paintarea.cpp \
    tests.cpp \
    aboutdialog.cpp \
    jstextarea.cpp \
    ../tools/viewport.cpp \
    ../tools/colors.cpp \
    ui/dialogchangedvalues.cpp \
    ui/dialogcomment.cpp \
    ui/dialogfunctionplotter.cpp \
    ui/genericinputwidget.cpp \
    ui/linkxmlqt.cpp \
    ui/settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    ../3rdparty/FreeImage/FreeImage.h \
    stable.h \
    paintarea.h \
    tests.h \
    aboutdialog.h \
    jstextarea.h \
    ../tools/viewport.h \
    ../tools/colors.h \
    ui/dialogchangedvalues.h \
    ui/dialogcomment.h \
    ui/dialogfunctionplotter.h \
    ui/genericinputwidget.h \
    ui/linkxmlqt.h \
    ui/settingsdialog.h

FORMS += \
    mainwindow.ui \
    aboutdialog.ui \
    ui/dialogchangedvalues.ui \
    ui/dialogcomment.ui \
    ui/dialogfunctionplotter.ui \
    ui/settingsTestDialog.ui

RESOURCES += \
    qml_res.qrc

OTHER_FILES += \
    maindoc.cpp \
    qml/ruler.qml \
    ../apidoc/abe/abe_doc.js \
    ../apidoc/abe/treelist_doc.js \
    ../apidoc/iLand/globals_doc.js \
    ../apidoc/abe/abe_context_doc.js

DISTFILES += \
    ../../.github/workflows/build.yaml \
    ../3rdparty/FreeImage/FreeImage.dll \
    ../3rdparty/FreeImage/FreeImage.lib \
    ../abe-lib/ABE-library.js \
    ../abe-lib/harvest/femel.js \
    ../abe-lib/harvest/harvest.js \
    ../abe-lib/harvest/salvage.js \
    ../abe-lib/lib_helper.js \
    ../abe-lib/planting/planting.js \
    ../abe-lib/thinning/selective.js \
    ../abe-lib/thinning/thinning.js \
    ../apidoc/ABE/abe_context_doc.js \
    ../apidoc/ABE/abe_doc.js \
    ../apidoc/ABE/abe_patches.js \
    ../apidoc/ABE/deadtreelist_doc.js \
    ../apidoc/ABE/saplinglist_doc.js \
    ../apidoc/ABE/treelist_doc.js \
    ../apidoc/iLand/grid_doc.js \
    ../apidoc/iLand/map_doc.js \
    ../apidoc/iLand/factory_doc.js \
    ../apidoc/iLand/spatialAnalysis_doc.js \
    ../apidoc/iLand/management_doc.js \
    ../apidoc/iLand/iLand_doc.js \
    ../apidoc/ABE/abe_stand.js \
    ../apidoc/iLand/csvfie_doc.js \
    ../apidoc/iLand/tree_doc.js \
    ../apidoc/iLand/treeexpr_doc.js \
    res/iland.desktop
