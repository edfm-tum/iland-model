# iLand project file for the flow module. See iland-model.org
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
    flowmodel.h \
    flowmodule.h \
    flowplugin.h \
    flowscript.h

SOURCES       = \
    flowmodel.cpp \
    flowmodule.cpp \
    flowplugin.cpp \
    flowscript.cpp

TARGET        = $$qtLibraryTarget(iland_flow)
DESTDIR       = ../../plugins

DEFINES += NO_DEBUG_MSGS

DISTFILES += \
    flowplugin.json





