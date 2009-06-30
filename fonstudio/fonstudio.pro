# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:47:01
# -------------------------------------------------
QT += xml
QT += script
TARGET = fonstudio
TEMPLATE = app
# Use Precompiled headers (PCH)
PRECOMPILED_HEADER = stable.h

SOURCES += main.cpp \
    mainwindow.cpp \
    ../core/grid.cpp \
    ../core/solarradiation.cpp \
    ../core/hemigrid.cpp \
    stamp.cpp \
    tree.cpp \
    paintarea.cpp \
    expression.cpp \
    helper.cpp
HEADERS += mainwindow.h \
    stable.h \
    ../core/grid.h \
    ../core/solarradiation.h \
    ../core/hemigrid.h \
    stamp.h \
    tree.h \
    paintarea.h \
    expression.h \
    helper.h
FORMS += mainwindow.ui
RESOURCES += res/fonstudio.qrc
