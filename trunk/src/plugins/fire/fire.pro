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
    fireplugin.h \
    firemodule.h \
    fireout.h
SOURCES       = \
    fireplugin.cpp \
    firemodule.cpp \
    fireout.cpp
TARGET        = $$qtLibraryTarget(iland_fire)
DESTDIR       = ../../plugins


