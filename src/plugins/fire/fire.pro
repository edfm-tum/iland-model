QT += xml
QT += script
QT += sql

TEMPLATE      = lib
CONFIG       += plugin static
INCLUDEPATH  += ../.. \
                ../../tools \
                ../../core
HEADERS       = \
    fireplugin.h \
    firemodule.h
SOURCES       = \
    fireplugin.cpp \
    firemodule.cpp
TARGET        = $$qtLibraryTarget(iland_fire)
DESTDIR       = ../../plugins
