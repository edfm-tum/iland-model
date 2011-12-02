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
    windmodule.h
SOURCES       = \
        windplugin.cpp \
    windmodule.cpp
TARGET        = $$qtLibraryTarget(iland_wind)
DESTDIR       = ../../plugins








