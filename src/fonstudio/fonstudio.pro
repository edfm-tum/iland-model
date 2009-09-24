# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:47:01
# -------------------------------------------------
QT += xml
QT += script
QT += sql
TARGET = fonstudio
TEMPLATE = app

# includepath: adds directories to the standard include (no directory needed when #include a file).
INCLUDEPATH += ../core \
    ../tools

# Use Precompiled headers (PCH)
PRECOMPILED_HEADER = stable.h
SOURCES += main.cpp \
    mainwindow.cpp \
    ../core/grid.cpp \
    solarradiation.cpp \
    hemigrid.cpp \
    imagestamp.cpp \
    ../core/tree.cpp \
    paintarea.cpp \
    ../tools/expression.cpp \
    ../tools/helper.cpp \
    lightroom.cpp \
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
    ../output/outputmanager.cpp \
    ../output/output.cpp \
    ../tools/expressionwrapper.cpp \
    ../output/treeout.cpp \
    ../core/management.cpp \
    ../core/standstatistics.cpp \
    ../core/modelsettings.cpp \
    ../core/speciesresponse.cpp
HEADERS += mainwindow.h \
    ../stable.h \
    ../core/grid.h \
    solarradiation.h \
    hemigrid.h \
    imagestamp.h \
    ../core/tree.h \
    paintarea.h \
    ../tools/expression.h \
    ../tools/helper.h \
    lightroom.h \
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
    ../output/outputmanager.h \
    ../output/output.h \
    ../core/management.h \
    ../core/standstatistics.h \
    ../core/modelsettings.h \
    ../core/speciesresponse.h

FORMS += mainwindow.ui
RESOURCES += res/fonstudio.qrc
DEFINES += FONSTUDIO
