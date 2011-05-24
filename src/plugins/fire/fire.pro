QT += xml
QT += script
QT += sql

TEMPLATE      = lib
CONFIG       += plugin static
INCLUDEPATH  += ../.. \
                ../../tools \
                ../../core
HEADERS       = \
    fireplugin.h
SOURCES       = \
    fireplugin.cpp
TARGET        = $$qtLibraryTarget(iland_fire)
DESTDIR       = ../../plugins
