# iLand project file for the fire module. See iland-model.org
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

# make sure to remove AGL (build with 6.8 in July 2026 - probably not necessary with >qt6.10
macx {
    LIBS -= -framework AGL
    QMAKE_LIBS_OPENGL -= -framework AGL
}


CONFIG += exceptions
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

DEFINES += NO_DEBUG_MSGS



