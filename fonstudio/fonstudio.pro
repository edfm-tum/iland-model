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
    core/grid.cpp \
    core/solarradiation.cpp \
    core/hemigrid.cpp \
    imagestamp.cpp \
    tree.cpp \
    paintarea.cpp \
    tools/expression.cpp \
    tools/helper.cpp \
    lightroom.cpp \
    core/stamp.cpp
HEADERS += mainwindow.h \
    stable.h \
    core/grid.h \
    core/solarradiation.h \
    core/hemigrid.h \
    imagestamp.h \
    tree.h \
    paintarea.h \
    tools/expression.h \
    tools/helper.h \
    lightroom.h \
    global.h \
    core/stamp.h
FORMS += mainwindow.ui
RESOURCES += res/fonstudio.qrc
