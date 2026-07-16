# iLand project file for the wind module. See iland-model.org
QT += xml
QT += qml
QT += sql

TEMPLATE      = lib
CONFIG       += plugin static
CONFIG += exceptions

# make sure to remove AGL (build with 6.8 in July 2026 - probably not necessary with >qt6.10
macx {
    LIBS -= -framework AGL
    QMAKE_LIBS_OPENGL -= -framework AGL
}


INCLUDEPATH  += ../.. \
                ../../tools \
                ../../output \
                ../../core
HEADERS       = \
        windplugin.h \
    windmodule.h \
    windscript.h \
    windout.h
SOURCES       = \
        windplugin.cpp \
    windmodule.cpp \
    windscript.cpp \
    windout.cpp
TARGET        = $$qtLibraryTarget(iland_wind)
DESTDIR       = ../../plugins

DEFINES += NO_DEBUG_MSGS









