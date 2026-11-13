# -------------------------------------------------
# iLand automated C++ unit & component test runner
# -------------------------------------------------

include(../iland_core.pri)

QT -= gui
QT += testlib

INCLUDEPATH += ../tests

TARGET = iland_tests
TEMPLATE = app
CONFIG += console testcase
CONFIG -= app_bundle

SOURCES += \
    test_main.cpp \
    test_expression.cpp \
    test_grid.cpp

HEADERS += \
    test_expression.h \
    test_grid.h
