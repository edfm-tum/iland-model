# -------------------------------------------------
# iLand project file (GUI version). See iland-model.org
# -------------------------------------------------
QT += xml
QT += qml
QT += sql
QT += widgets
# charts for the function plotter
QT += charts
# quick: for QML based user interface
QT += quick
QT += concurrent


TARGET = iland
TEMPLATE = app
CONFIG += precompile_header
CONFIG += c++17
#LIBS += -lQt5Concurrent

# includepath: adds directories to the standard include (no directory needed when #include a file).
INCLUDEPATH += ../core \
    ../tools \
    ../output \
    ../abe \
    ../abe/output \
    ../bite \
    ../iland

DEPENDPATH += plugins
CONFIG += exceptions
CONFIG += rtti

# get the path of plugins build
# Define potential plugin locations relative to the build directory
PLUGIN_LOCATIONS = $$OUT_PWD/../plugins $$OUT_PWD/../../../plugins/build/plugins

# Find the actual plugin location
for(plugin_loc, PLUGIN_LOCATIONS) {
    !isEmpty(plugin_loc) {
        exists($$plugin_loc) {
            PLUGIN_PATH = $$plugin_loc
            break()
        }
    }
}


THIRDPARTY_LOCATIONS = $$OUT_PWD/../3rdparty $$OUT_PWD/../../../3rdparty

# Find the actual plugin location
for(plugin_loc, THIRDPARTY_LOCATIONS) {
    !isEmpty(plugin_loc) {
        exists($$plugin_loc) {
            THIRDPARTY_PATH = $$plugin_loc
            break()
        }
    }
}
message("Plugins path: " $$PLUGIN_PATH " 3rd party libs:" $$THIRDPARTY_PATH)

CONFIG(debug, debug|release) {
    BUILDS += debug
    PLUGIN_SUFFIX = d # Suffix for debug builds
} else {
    BUILDS += release
    PLUGIN_SUFFIX = # Empty suffix for release builds
}


*msvc*: {
    LIBPOST = .lib # .lib for Windows / MSVC
    LIBPRAE = iland_ # no lib as praefix
} else {
    LIBPRAE = libiland_
    LIBPOST = .a # .a for GCC on Windows/Linux
    PLUGIN_SUFFIX = # Empty suffix in any case for GCC
}

# build the full filename of the library file:
parts_to_join_fire = $$PLUGIN_PATH / $$LIBPRAE fire $$PLUGIN_SUFFIX $$LIBPOST
parts_to_join_wind = $$PLUGIN_PATH / $$LIBPRAE wind $$PLUGIN_SUFFIX $$LIBPOST
parts_to_join_barkbeetle = $$PLUGIN_PATH / $$LIBPRAE barkbeetle $$PLUGIN_SUFFIX $$LIBPOST

PRE_TARGETDEPS += $$join(parts_to_join_fire)
PRE_TARGETDEPS += $$join(parts_to_join_wind)
PRE_TARGETDEPS += $$join(parts_to_join_barkbeetle)

LIBS += -L$$PLUGIN_PATH -liland_fire$$PLUGIN_SUFFIX -liland_wind$$PLUGIN_SUFFIX -liland_barkbeetle$$PLUGIN_SUFFIX

message("PRE_TARGETDEPS:" $$PRE_TARGETDEPS)


linux-g++ {
# The "FreeImage" library is used for processing GeoTIFF data files.
# FreeImage on Linux: see https://codeyarns.com/2014/02/11/how-to-install-and-use-freeimage/
# basically sudo apt-get install libfreeimage3 libfreeimage-dev

LIBS += -lfreeimage
}macx{ 
LIBS += -L/opt/homebrew/Cellar/freeimage/3.18.0/lib -lfreeimage
} else {
# external freeimage library (geotiff)
LIBS += -L$$THIRDPARTY_PATH\FreeImage -lFreeImage
}

# querying git repo
win32 {
 !defined(GIT_HASH) {
GIT_HASH="\\\"$$quote($$system(git rev-parse --short HEAD))\\\""
GIT_BRANCH="\\\"$$quote($$system(git rev-parse --abbrev-ref HEAD))\\\""
BUILD_TIMESTAMP="\\\"$$quote($$system(date /t))\\\""
DEFINES += GIT_HASH=$$GIT_HASH GIT_BRANCH=$$GIT_BRANCH BUILD_TIMESTAMP=$$BUILD_TIMESTAMP
}
} else {
!defined(GIT_HASH) {
GIT_HASH="\\\"$$system(git -C \""$$_PRO_FILE_PWD_"\" rev-parse --short HEAD)\\\""
GIT_BRANCH="\\\"$$system(git -C \""$$_PRO_FILE_PWD_"\" rev-parse --abbrev-ref HEAD)\\\""
BUILD_TIMESTAMP="\\\"$$system(date -u +\""%Y-%m-%dT%H:%M:%SUTC\"")\\\""
DEFINES += GIT_HASH=$$GIT_HASH GIT_BRANCH=$$GIT_BRANCH BUILD_TIMESTAMP=$$BUILD_TIMESTAMP
}
}

# to enable debug symbols in release code
# CONFIG += force_debug_info
# debug information in release-mode executable
#QMAKE_CXXFLAGS_RELEASE += -g
#QMAKE_LFLAGS_RELEASE -= -Wl,-s

## MSVC: debug symbols in release-mode
#QMAKE_CXXFLAGS_RELEASE += -Zi
#QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF /OPT:ICF

### Flag to allow 3GB on Win 32
### you also need to modify boot.ini ... not necessary for 64bit
#QMAKE_LFLAGS_WINDOWS += -Wl,--large-address-aware


# This is the UI version of iLand!
DEFINES += ILAND_GUI
# enable/disble DBGMODE messages: dbg messages are removed when the define is added
DEFINES += NO_DEBUG_MSGS

# for debugging only: print all qmake variables
for(var, $$list($$enumerate_vars())) {
    message($$var ": " $$eval($$var))
}

# Use Precompiled headers (PCH)
#PRECOMPILED_HEADER = stable.h
SOURCES += main.cpp \
    ../abe/fmdeadtreelist.cpp \
    ../abe/patch.cpp \
    ../abe/patches.cpp \
    ../core/deadtree.cpp \
    ../core/microclimate.cpp \
    ../core/permafrost.cpp \
    ../output/devstageout.cpp \
    ../output/ecovizout.cpp \
    ../output/svdindicatorout.cpp \
    ../tools/geotiff.cpp \
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
    ../output/customaggout.cpp \
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
    ../core/version.cpp \
    ../core/timeevents.cpp \
    ../core/seeddispersal.cpp \
    ../core/establishment.cpp \
    ../core/soil.cpp \
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
    ../tools/spatialanalysis.cpp \
    ../abe/activity.cpp \
    ../abe/forestmanagementengine.cpp \
    ../tools/statdata.cpp \
    ../tools/debugtimer.cpp \
    ../tools/viewport.cpp \
    ../abe/fomewrapper.cpp \
    ../abe/fmstand.cpp \
    ../abe/agent.cpp \
    ../abe/fmunit.cpp \
    ../abe/agenttype.cpp \
    ../abe/fomescript.cpp \
    ../output/landscapeout.cpp \
    ../abe/fmstp.cpp \
    ../abe/thinning.cpp \
    ../abe/actgeneral.cpp \
    ../abe/abegrid.cpp \
    ../abe/scheduler.cpp \
    ../tools/colors.cpp \
    ../abe/fmtreelist.cpp \
    ../abe/actscheduled.cpp \
    ../abe/actplanting.cpp \
    ../abe/actsalvage.cpp \
    ../abe/output/unitout.cpp \
    ../abe/output/abestandout.cpp \
    ../abe/output/abestandremovalout.cpp \
    ../abe/actthinning.cpp \
    ../core/grasscover.cpp \
    ../tools/scriptgrid.cpp \
    ../core/saplings.cpp \
    ../output/waterout.cpp \
    ../core/dbhdistribution.cpp \
    jstextarea.cpp \
    ../output/svdout.cpp \
    ../core/svdstate.cpp \
    ../output/soilinputout.cpp \
    ../tools/scripttree.cpp \
    ../tools/scriptresourceunit.cpp \
    ../bite/bitescript.cpp \
    ../bite/biteengine.cpp \
    ../bite/biteagent.cpp \
    ../bite/bitecell.cpp \
    ../bite/biteitem.cpp \
    ../bite/bitedispersal.cpp \
    ../bite/bitecolonization.cpp \
    ../bite/bitecellscript.cpp \
    ../bite/bitewrapper.cpp \
    ../bite/bitebiomass.cpp \
    ../bite/bitelifecycle.cpp \
    ../bite/biteimpact.cpp \
    ../bite/biteclimate.cpp \
    ../bite/biteoutput.cpp \
    ../abe/fmsaplinglist.cpp \
    ../bite/biteoutputitem.cpp \
    ui/dialogchangedvalues.cpp \
    ui/dialogcomment.cpp \
    ui/dialogfunctionplotter.cpp \
    ui/genericinputwidget.cpp \
    ui/linkxmlqt.cpp \
    ui/settingsdialog.cpp

HEADERS += mainwindow.h \
    ../3rdparty/FreeImage/FreeImage.h \
    ../abe/fmdeadtreelist.h \
    ../abe/patch.h \
    ../abe/patches.h \
    ../core/deadtree.h \
    ../core/microclimate.h \
    ../core/permafrost.h \
    ../output/devstageout.h \
    ../output/ecovizout.h \
    ../output/svdindicatorout.h \
    ../tools/geotiff.h \
    stable.h \
    paintarea.h \
    ../core/version.h \
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
    ../output/customaggout.h \
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
    ../tools/spatialanalysis.h \
    ../abe/activity.h \
    ../abe/forestmanagementengine.h \
    ../abe/abe_global.h \
    ../tools/statdata.h \
    ../tools/debugtimer.h \
    ../tools/viewport.h \
    ../abe/fomewrapper.h \
    ../abe/fmstand.h \
    ../abe/agent.h \
    ../abe/fmunit.h \
    ../abe/agenttype.h \
    ../abe/fomescript.h \
    ../output/landscapeout.h \
    ../abe/fmstp.h \
    ../abe/thinning.h \
    ../abe/actgeneral.h \
    ../abe/abegrid.h \
    ../abe/scheduler.h \
    ../tools/colors.h \
    ../abe/fmtreelist.h \
    ../abe/actscheduled.h \
    ../abe/actplanting.h \
    ../abe/actsalvage.h \
    ../abe/output/unitout.h \
    ../abe/output/abestandout.h \
    ../abe/output/abestandremovalout.h \
    ../abe/actthinning.h \
    ../core/grasscover.h \
    ../tools/scriptgrid.h \
    ../core/saplings.h \
    ../output/waterout.h \
    ../core/dbhdistribution.h \
    jstextarea.h \
    ../output/svdout.h \
    ../core/svdstate.h \
    ../output/soilinputout.h \
    ../tools/scripttree.h \
    ../tools/scriptresourceunit.h \
    ../bite/bitescript.h \
    ../bite/biteengine.h \
    ../bite/bite_global.h \
    ../bite/biteagent.h \
    ../bite/bitecell.h \
    ../bite/biteitem.h \
    ../bite/bitedispersal.h \
    ../bite/bitecolonization.h \
    ../bite/bitecellscript.h \
    ../bite/bitewrapper.h \
    ../bite/bitebiomass.h \
    ../bite/bitelifecycle.h \
    ../bite/biteimpact.h \
    ../bite/biteclimate.h \
    ../bite/biteoutput.h \
    ../abe/fmsaplinglist.h \
    ../bite/biteoutputitem.h \
    ui/dialogchangedvalues.h \
    ui/dialogcomment.h \
    ui/dialogfunctionplotter.h \
    ui/genericinputwidget.h \
    ui/linkxmlqt.h \
    ui/settingsdialog.h
FORMS += mainwindow.ui \
    aboutdialog.ui \
    ui/dialogchangedvalues.ui \
    ui/dialogcomment.ui \
    ui/dialogfunctionplotter.ui \
    ui/settingsTestDialog.ui
RESOURCES += ./res/iland.qrc \
    ../abe-lib/abe-library.qrc \
    abe-library.qrc \
    qml_res.qrc

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
OTHER_FILES += maindoc.cpp \
    qml/ruler.qml \
    ../apidoc/abe/abe_doc.js \
    ../apidoc/abe/treelist_doc.js \
    ../apidoc/iLand/globals_doc.js \
    ../apidoc/abe/abe_context_doc.js

DISTFILES += \
    ../3rdparty/FreeImage/FreeImage.dll \
    ../3rdparty/FreeImage/FreeImage.lib \
    ../abe-lib/ABE-library.js \
    ../abe-lib/harvest/femel.js \
    ../abe-lib/harvest/harvest.js \
    ../abe-lib/harvest/salvage.js \
    ../abe-lib/lib_helper.js \
    ../abe-lib/planting/planting.js \
    ../abe-lib/thinning/selective.js \
    ../abe-lib/thinning/thinning.js \
    ../apidoc/ABE/abe_patches.js \
    ../apidoc/ABE/deadtreelist_doc.js \
    ../apidoc/ABE/saplinglist_doc.js \
    ../apidoc/iLand/grid_doc.js \
    ../apidoc/iLand/map_doc.js \
    ../apidoc/iLand/factory_doc.js \
    ../apidoc/iLand/spatialAnalysis_doc.js \
    ../apidoc/iLand/management_doc.js \
    ../apidoc/iLand/iLand_doc.js \
    ../apidoc/ABE/abe_stand.js \
    ../apidoc/iLand/csvfie_doc.js \
    ../apidoc/iLand/tree_doc.js \
    ../apidoc/iLand/treeexpr_doc.js






