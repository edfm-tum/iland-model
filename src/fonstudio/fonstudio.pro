# -------------------------------------------------
# iLand Lightroom (fonstudio) project file
# -------------------------------------------------
QT += xml
QT += widgets
QT += qml
TARGET = fonstudio
TEMPLATE = app

INCLUDEPATH += ../core \
    ../tools \
    ../output

CONFIG += exceptions
CONFIG += c++17

SOURCES += main.cpp \
    mainwindow.cpp \
    ../core/grid.cpp \
    solarradiation.cpp \
    hemigrid.cpp \
    paintarea.cpp \
    ../tools/expression.cpp \
    ../tools/helper.cpp \
    lightroom.cpp \
    ../core/stamp.cpp \
    ../core/stampcontainer.cpp \
    version.cpp \
    ../tools/randomgenerator.cpp \
    ../tools/debugtimer.cpp


HEADERS += mainwindow.h \
    stable.h \
    ../core/grid.h \
    solarradiation.h \
    hemigrid.h \
    paintarea.h \
    ../tools/expression.h \
    ../tools/helper.h \
    lightroom.h \
    ../core/exception.h \
    ../core/global.h \
    ../core/stamp.h \
    ../core/stampcontainer.h \
    ../3rdparty/MersenneTwister.h \
    version.h \
    ../tools/randomgenerator.h \
    ../tools/debugtimer.h


FORMS += mainwindow.ui
RESOURCES += res/fonstudio.qrc
DEFINES += FONSTUDIO
win32: DEFINES += _USE_MATH_DEFINES
