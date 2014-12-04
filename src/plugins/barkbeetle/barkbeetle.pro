QT += xml
QT += qml
QT += sql


TEMPLATE      = lib
CONFIG       += plugin static
INCLUDEPATH  += ../.. \
                ../../tools \
                ../../output \
                ../../core

# to enable debug symbols in release code:
# uncomment then next two lines to enable debug information in release-mode executable
#QMAKE_CXXFLAGS_RELEASE += -g
#QMAKE_LFLAGS_RELEASE -= -Wl,-s

CONFIG += exceptions
HEADERS       = \
    barkbeetleplugin.h \
    barkbeetlemodule.h \
    barkbeetlescript.h \
    bbgenerations.h

SOURCES       = \
    barkbeetleplugin.cpp \
    barkbeetlemodule.cpp \
    barkbeetlescript.cpp \
    bbgenerations.cpp
TARGET        = $$qtLibraryTarget(iland_barkbeetle)
DESTDIR       = ../../plugins






