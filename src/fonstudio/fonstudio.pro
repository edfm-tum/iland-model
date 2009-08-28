# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:47:01
# -------------------------------------------------
QT += xml
QT += script
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
    ../core/solarradiation.cpp \
    ../core/hemigrid.cpp \
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
    ../core/speciesset.cpp
HEADERS += mainwindow.h \
    stable.h \
    ../core/grid.h \
    ../core/solarradiation.h \
    ../core/hemigrid.h \
    imagestamp.h \
    ../core/tree.h \
    paintarea.h \
    ../tools/expression.h \
    ../tools/helper.h \
    lightroom.h \
    global.h \
    ../core/stamp.h \
    ../core/stampcontainer.h \
    ../core/species.h \
    ../tools/ticktack.h \
    ../tools/settingmetadata.h \
    ../tools/globalsettings.h \
    ../core/speciesset.h
FORMS += mainwindow.ui
RESOURCES += res/fonstudio.qrc
