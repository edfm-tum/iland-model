#-------------------------------------------------
#
# iLand console version project file. See iland-model.org
#
#-------------------------------------------------

include(../iland_core.pri)

QT -= gui
INCLUDEPATH += ../ilandc

TARGET = ilandc
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SOURCES += \
    main.cpp \
    consoleshell.cpp

HEADERS += \
    consoleshell.h \
    stable.h
