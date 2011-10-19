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
    fireout.h \
    firescript.h
SOURCES       = \
    fireplugin.cpp \
    firemodule.cpp \
    fireout.cpp \
    firescript.cpp
TARGET        = $$qtLibraryTarget(iland_fire)
DESTDIR       = ../../plugins




