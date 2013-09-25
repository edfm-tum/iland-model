# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:47:01
# -------------------------------------------------
QT += xml
QT += qml
QT += sql
QT += widgets

TARGET = iland
TEMPLATE = app
CONFIG += precompile_header

LIBS += -lQt5Concurrent

# includepath: adds directories to the standard include (no directory needed when #include a file).
INCLUDEPATH += ../core \
    ../tools \
    ../output \
    ../iland

DEPENDPATH += plugins
CONFIG += exceptions
CONFIG += rtti

CONFIG(debug, debug|release) {
win32-msvc*:{
#debug msvc
PRE_TARGETDEPS += ../plugins/iland_fired.lib
PRE_TARGETDEPS += ../plugins/iland_windd.lib
LIBS += -L../plugins -liland_fired -liland_windd
}
}

## win32-msvc*:contains(QMAKE_TARGET.arch, x86_64):{ ... } nur 64bit

CONFIG(release, debug|release) {
# release stuff
#PRE_TARGETDEPS += ../plugins/libiland_fire.a
#PRE_TARGETDEPS += ../plugins/libiland_wind.a
#LIBS += -L../plugins -liland_fire -liland_wind
win32-msvc*:{
#debug msvc
PRE_TARGETDEPS += ../plugins/iland_fire.lib
PRE_TARGETDEPS += ../plugins/iland_wind.lib
LIBS += -L../plugins -liland_fire -liland_wind
}
}

DEFINES += ILAND_GUI

# to enable debug symbols in release code
# debug information in release-mode executable
#QMAKE_CXXFLAGS_RELEASE += -g
#QMAKE_LFLAGS_RELEASE -= -Wl,-s

### Flag to allow 3GB on Win 32
### you also need to modify boot.ini ... not necessary for 64bit
#QMAKE_LFLAGS_WINDOWS += -Wl,--large-address-aware

# Use Precompiled headers (PCH)
PRECOMPILED_HEADER = stable.h
SOURCES += main.cpp \
    mainwindow.cpp \
    paintarea.cpp \
    ../core/grid.cpp \
    ../core/tree.cpp \
    ../tools/expression.cpp \
    ../tools/helper.cpp \
    ../core/stamp.cpp \
    ../core/stampcontainer.cpp \
    ../core/species.cpp \
    ../tools/ticktack.cpp \
    ../tools/settingmetadata.cpp \
    ../tools/globalsettings.cpp \
    ../core/speciesset.cpp \
    ../core/model.cpp \
    ../core/resourceunit.cpp \
    ../tools/xmlhelper.cpp \
    ../core/modelcontroller.cpp \
    ../core/standloader.cpp \
    ../core/resourceunitspecies.cpp \
    ../core/production3pg.cpp \
    ../core/threadrunner.cpp \
    ../tools/expressionwrapper.cpp \
    tests.cpp \
    ../output/output.cpp \
    ../output/treeout.cpp \
    ../output/outputmanager.cpp \
    ../output/standout.cpp \
    ../core/standstatistics.cpp \
    ../output/dynamicstandout.cpp \
    ../core/management.cpp \
    ../core/speciesresponse.cpp \
    ../core/climate.cpp \
    ../core/modelsettings.cpp \
    ../core/phenology.cpp \
    ../tools/floatingaverage.cpp \
    ../output/productionout.cpp \
    ../core/watercycle.cpp \
    ../tools/climateconverter.cpp \
    ../tools/csvfile.cpp \
    ../tools/scriptglobal.cpp \
    ../output/standdeadout.cpp \
    ../core/environment.cpp \
    ../output/managementout.cpp \
    ../tools/sqlhelper.cpp \
    ../tools/random.cpp \
    aboutdialog.cpp \
    version.cpp \
    ../core/timeevents.cpp \
    ../core/seeddispersal.cpp \
    ../core/establishment.cpp \
    ../core/soil.cpp \
    ../core/sapling.cpp \
    ../core/snag.cpp \
    ../output/saplingout.cpp \
    ../tools/gisgrid.cpp \
    ../tools/mapgrid.cpp \
    ../output/carbonout.cpp \
    ../output/carbonflowout.cpp \
    ../tools/modules.cpp \
    ../tools/dem.cpp \
    ../3rdparty/SimpleRNG.cpp \
    ../output/snapshot.cpp \
    ../tools/randomgenerator.cpp \
    ../tools/spatialanalysis.cpp
HEADERS += mainwindow.h \
    stable.h \
    paintarea.h \
    version.h \
    ../core/grid.h \
    ../core/tree.h \
    ../tools/expression.h \
    ../tools/helper.h \
    ../core/exception.h \
    ../core/global.h \
    ../core/stamp.h \
    ../core/stampcontainer.h \
    ../core/species.h \
    ../tools/ticktack.h \
    ../tools/settingmetadata.h \
    ../tools/globalsettings.h \
    ../core/speciesset.h \
    ../core/model.h \
    ../core/resourceunit.h \
    ../tools/xmlhelper.h \
    ../core/modelcontroller.h \
    ../core/standloader.h \
    ../core/resourceunitspecies.h \
    ../core/production3pg.h \
    ../core/threadrunner.h \
    ../tools/expressionwrapper.h \
    tests.h \
    ../output/output.h \
    ../output/treeout.h \
    ../output/outputmanager.h \
    ../output/standout.h \
    ../core/standstatistics.h \
    ../output/dynamicstandout.h \
    ../core/management.h \
    ../core/speciesresponse.h \
    ../core/climate.h \
    ../core/modelsettings.h \
    ../core/phenology.h \
    ../tools/floatingaverage.h \
    ../output/productionout.h \
    ../core/watercycle.h \
    ../tools/climateconverter.h \
    ../tools/csvfile.h \
    ../tools/scriptglobal.h \
    ../output/standdeadout.h \
    ../core/environment.h \
    ../output/managementout.h \
    ../tools/sqlhelper.h \
    ../tools/random.h \
    ../3rdparty/MersenneTwister.h \
    aboutdialog.h \
    ../core/timeevents.h \
    ../core/seeddispersal.h \
    ../core/establishment.h \
    ../core/soil.h \
    ../core/sapling.h \
    ../core/snag.h \
    ../output/saplingout.h \
    ../tools/gisgrid.h \
    ../tools/mapgrid.h \
    ../output/carbonout.h \
    ../output/carbonflowout.h \
    ../core/plugin_interface.h \
    ../tools/modules.h \
    ../tools/dem.h \
    ../core/layeredgrid.h \
    ../3rdparty/SimpleRNG.h \
    ../output/snapshot.h \
    ../tools/randomgenerator.h \
    ../tools/spatialanalysis.h
FORMS += mainwindow.ui \
    aboutdialog.ui
RESOURCES += ./res/iland.qrc

# QMAKE_EXTRA_TARGETS += revtarget
# PRE_TARGETDEPS += version.h
# revtarget.target = version.h
# SVNREV = $$system(svnversion .)
# revtarget.commands = @echo \
# "const char *version = \"Alpha 1.1.0\";" \
# "const char *svn_revision = \"$$SVNREV\";" \
# >  $$revtarget.target
# revtarget.depends = $$SOURCES \
# $$HEADERS \
# $$FORMS
OTHER_FILES += maindoc.cpp






