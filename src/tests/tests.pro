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
    test_grid.cpp \
    test_xmlhelper.cpp \
    test_data_stats.cpp \
    test_random_stamps.cpp \
    test_core_entities.cpp \
    test_script_engine.cpp

HEADERS += \
    test_expression.h \
    test_grid.h \
    test_xmlhelper.h \
    test_data_stats.h \
    test_random_stamps.h \
    test_core_entities.h \
    test_script_engine.h
