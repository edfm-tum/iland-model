# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:47:01
# -------------------------------------------------
QT += xml
QT += script
QT += sql
TARGET = iland
TEMPLATE = app
CONFIG += precompile_header

# includepath: adds directories to the standard include (no directory needed when #include a file).
INCLUDEPATH += ../core \
    ../tools

# Use Precompiled headers (PCH)
PRECOMPILED_HEADER = stable.h
SOURCES += main.cpp \
    mainwindow.cpp \
    paintarea.cpp \
    ../core/grid.cpp \
    ../core/tree.cpp \
    ../tools/expression.cpp \
    ../tools/helper.cpp \
    ../core/stamp.cpp \
    ../core/stampcontainer.cpp \
    ../core/species.cpp \
    ../tools/ticktack.cpp \
    ../tools/settingmetadata.cpp \
    ../tools/globalsettings.cpp \
    ../core/speciesset.cpp \
    ../core/model.cpp \
    ../core/resourceunit.cpp \
    ../tools/xmlhelper.cpp \
    ../core/modelcontroller.cpp \
    ../core/standloader.cpp \
    ../core/resourceunitspecies.cpp \
    ../core/production3pg.cpp \
    ../core/threadrunner.cpp \
    ../tools/expressionwrapper.cpp \
    tests.cpp \
    ../output/output.cpp \
    ../output/treeout.cpp \
    ../output/outputmanager.cpp \
    ../output/standout.cpp \
    ../core/standstatistics.cpp \
    ../output/dynamicstandout.cpp \
    management.cpp \
    ../core/speciesresponse.cpp \
    ../core/climate.cpp
HEADERS += mainwindow.h \
    ../stable.h \
    paintarea.h \
    ../core/grid.h \
    ../core/tree.h \
    ../tools/expression.h \
    ../tools/helper.h \
    ../core/exception.h \
    ../core/global.h \
    ../core/stamp.h \
    ../core/stampcontainer.h \
    ../core/species.h \
    ../tools/ticktack.h \
    ../tools/settingmetadata.h \
    ../tools/globalsettings.h \
    ../core/speciesset.h \
    ../core/model.h \
    ../core/resourceunit.h \
    ../tools/xmlhelper.h \
    ../core/modelcontroller.h \
    ../core/standloader.h \
    ../core/resourceunitspecies.h \
    ../core/production3pg.h \
    ../core/threadrunner.h \
    ../tools/expressionwrapper.h \
    tests.h \
    ../output/output.h \
    ../output/treeout.h \
    ../output/outputmanager.h \
    ../output/standout.h \
    ../core/standstatistics.h \
    ../output/dynamicstandout.h \
    management.h \
    ../core/speciesresponse.h \
    ../core/climate.h
FORMS += mainwindow.ui
RESOURCES += res/iland.qrc
