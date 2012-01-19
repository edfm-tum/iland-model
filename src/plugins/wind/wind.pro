QT += xml
QT += script
QT += sql

TEMPLATE      = lib
CONFIG       += plugin static
INCLUDEPATH  += ../.. \
                ../../tools \
                ../../output \
                ../../core
HEADERS       = \
        windplugin.h \
    windmodule.h \
    windscript.h
SOURCES       = \
        windplugin.cpp \
    windmodule.cpp \
    windscript.cpp
TARGET        = $$qtLibraryTarget(iland_wind)
DESTDIR       = ../../plugins








