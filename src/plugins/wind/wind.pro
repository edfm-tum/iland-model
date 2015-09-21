QT += xml
QT += qml
QT += sql

TEMPLATE      = lib
CONFIG       += plugin static
CONFIG += exceptions

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










