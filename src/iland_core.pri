# ------------------------------------------------------------------------------
# Shared iLand core engine configuration (included by iland, ilandc, and tests)
# ------------------------------------------------------------------------------

QT += core xml sql qml concurrent

CONFIG += c++17 exceptions rtti

INCLUDEPATH += \
    $$PWD/core \
    $$PWD/tools \
    $$PWD/output \
    $$PWD/abe \
    $$PWD/abe/output \
    $$PWD/bite

DEPENDPATH += plugins

# Locate static disturbance plugin archives (fire, wind, barkbeetle)
PLUGIN_LOCATIONS = $$OUT_PWD/../plugins $$OUT_PWD/../../../plugins/build/plugins $$PWD/plugins/build/plugins
for(plugin_loc, PLUGIN_LOCATIONS) {
    !isEmpty(plugin_loc) {
        exists($$plugin_loc) {
            PLUGIN_PATH = $$plugin_loc
            break()
        }
    }
}

THIRDPARTY_LOCATIONS = $$OUT_PWD/../3rdparty $$OUT_PWD/../../../3rdparty $$PWD/3rdparty
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
    PLUGIN_SUFFIX = d
} else {
    BUILDS += release
    PLUGIN_SUFFIX =
}

*msvc*: {
    LIBPOST = .lib
    LIBPRAE = iland_
} else {
    LIBPRAE = libiland_
    LIBPOST = .a
    PLUGIN_SUFFIX =
}

parts_to_join_fire = $$PLUGIN_PATH / $$LIBPRAE fire $$PLUGIN_SUFFIX $$LIBPOST
parts_to_join_wind = $$PLUGIN_PATH / $$LIBPRAE wind $$PLUGIN_SUFFIX $$LIBPOST
parts_to_join_barkbeetle = $$PLUGIN_PATH / $$LIBPRAE barkbeetle $$PLUGIN_SUFFIX $$LIBPOST

PRE_TARGETDEPS += $$join(parts_to_join_fire)
PRE_TARGETDEPS += $$join(parts_to_join_wind)
PRE_TARGETDEPS += $$join(parts_to_join_barkbeetle)

LIBS += -L$$PLUGIN_PATH -liland_fire$$PLUGIN_SUFFIX -liland_wind$$PLUGIN_SUFFIX -liland_barkbeetle$$PLUGIN_SUFFIX

# FreeImage linking setup (for GeoTIFF image support)
linux-g++ {
    LIBS += -lfreeimage
} else:macx {
    INCLUDEPATH += /opt/homebrew/include /usr/local/include
    LIBS += -L/opt/homebrew/lib -L/usr/local/lib -lfreeimage
} else {
    LIBS += -L$$THIRDPARTY_PATH\FreeImage -lFreeImage
}

linux-icc*: {
    message("linux intel icc release")
    QMAKE_CXXFLAGS -= -O2
    QMAKE_CXXFLAGS += -O3
}

DEFINES += NO_DEBUG_MSGS

# --- Version & Build Info Generation ---
GIT_DIR = $$PWD

defineReplace(addStringDefine) {
    VAR_NAME = $$1
    VAR_VAL  = $$2
    return($$join(VAR_NAME, "", "", "=\\\"$$VAR_VAL\\\""))
}

win32 {
    BUILD_TIMESTAMP = $$system(powershell -noprofile -command "Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'")
    GIT_HASH = $$system(git -C "$$GIT_DIR" rev-parse --short HEAD)
    GIT_BRANCH = $$system(git -C "$$GIT_DIR" rev-parse --abbrev-ref HEAD)
} else {
    BUILD_TIMESTAMP = $$system(date -u "+%Y-%m-%dT%H:%M:%SUTC")
    GIT_HASH = $$system(git -C "$$GIT_DIR" rev-parse --short HEAD)
    GIT_BRANCH = $$system(git -C "$$GIT_DIR" rev-parse --abbrev-ref HEAD)
}

DEFINES += $$addStringDefine(GIT_HASH, $$GIT_HASH)
DEFINES += $$addStringDefine(GIT_BRANCH, $$GIT_BRANCH)
DEFINES += $$addStringDefine(BUILD_TIMESTAMP, $$BUILD_TIMESTAMP)

# --- Shared Engine Sources ---
SOURCES += \
    $$PWD/3rdparty/SimpleRNG.cpp \
    $$PWD/core/climate.cpp \
    $$PWD/core/dbhdistribution.cpp \
    $$PWD/core/deadtree.cpp \
    $$PWD/core/environment.cpp \
    $$PWD/core/establishment.cpp \
    $$PWD/core/grasscover.cpp \
    $$PWD/core/grid.cpp \
    $$PWD/core/management.cpp \
    $$PWD/core/microclimate.cpp \
    $$PWD/core/model.cpp \
    $$PWD/core/modelcontroller.cpp \
    $$PWD/core/modelsettings.cpp \
    $$PWD/core/permafrost.cpp \
    $$PWD/core/phenology.cpp \
    $$PWD/core/production3pg.cpp \
    $$PWD/core/resourceunit.cpp \
    $$PWD/core/resourceunitspecies.cpp \
    $$PWD/core/saplings.cpp \
    $$PWD/core/seeddispersal.cpp \
    $$PWD/core/snag.cpp \
    $$PWD/core/soil.cpp \
    $$PWD/core/species.cpp \
    $$PWD/core/speciesresponse.cpp \
    $$PWD/core/speciesset.cpp \
    $$PWD/core/stamp.cpp \
    $$PWD/core/stampcontainer.cpp \
    $$PWD/core/standloader.cpp \
    $$PWD/core/standstatistics.cpp \
    $$PWD/core/svdstate.cpp \
    $$PWD/core/threadrunner.cpp \
    $$PWD/core/timeevents.cpp \
    $$PWD/core/tree.cpp \
    $$PWD/core/version.cpp \
    $$PWD/core/watercycle.cpp \
    $$PWD/tools/climateconverter.cpp \
    $$PWD/tools/csvfile.cpp \
    $$PWD/tools/debugtimer.cpp \
    $$PWD/tools/dem.cpp \
    $$PWD/tools/expression.cpp \
    $$PWD/tools/expressionwrapper.cpp \
    $$PWD/tools/floatingaverage.cpp \
    $$PWD/tools/geotiff.cpp \
    $$PWD/tools/gisgrid.cpp \
    $$PWD/tools/globalsettings.cpp \
    $$PWD/tools/helper.cpp \
    $$PWD/tools/mapgrid.cpp \
    $$PWD/tools/modules.cpp \
    $$PWD/tools/random.cpp \
    $$PWD/tools/randomgenerator.cpp \
    $$PWD/tools/scriptglobal.cpp \
    $$PWD/tools/scriptgrid.cpp \
    $$PWD/tools/scriptresourceunit.cpp \
    $$PWD/tools/scripttree.cpp \
    $$PWD/tools/settingmetadata.cpp \
    $$PWD/tools/spatialanalysis.cpp \
    $$PWD/tools/sqlhelper.cpp \
    $$PWD/tools/statdata.cpp \
    $$PWD/tools/ticktack.cpp \
    $$PWD/tools/xmlhelper.cpp \
    $$PWD/output/carbonflowout.cpp \
    $$PWD/output/carbonout.cpp \
    $$PWD/output/customaggout.cpp \
    $$PWD/output/deadtreeout.cpp \
    $$PWD/output/devstageout.cpp \
    $$PWD/output/dynamicstandout.cpp \
    $$PWD/output/ecovizout.cpp \
    $$PWD/output/landscapeout.cpp \
    $$PWD/output/managementout.cpp \
    $$PWD/output/output.cpp \
    $$PWD/output/outputmanager.cpp \
    $$PWD/output/productionout.cpp \
    $$PWD/output/saplingout.cpp \
    $$PWD/output/snapshot.cpp \
    $$PWD/output/soilinputout.cpp \
    $$PWD/output/standdeadout.cpp \
    $$PWD/output/standout.cpp \
    $$PWD/output/svdindicatorout.cpp \
    $$PWD/output/svdout.cpp \
    $$PWD/output/treeout.cpp \
    $$PWD/output/waterout.cpp \
    $$PWD/abe/abegrid.cpp \
    $$PWD/abe/actgeneral.cpp \
    $$PWD/abe/activity.cpp \
    $$PWD/abe/actplanting.cpp \
    $$PWD/abe/actsalvage.cpp \
    $$PWD/abe/actscheduled.cpp \
    $$PWD/abe/actthinning.cpp \
    $$PWD/abe/agent.cpp \
    $$PWD/abe/agenttype.cpp \
    $$PWD/abe/fmdeadtreelist.cpp \
    $$PWD/abe/fmsaplinglist.cpp \
    $$PWD/abe/fmstand.cpp \
    $$PWD/abe/fmstp.cpp \
    $$PWD/abe/fmtreelist.cpp \
    $$PWD/abe/fmunit.cpp \
    $$PWD/abe/fomescript.cpp \
    $$PWD/abe/fomewrapper.cpp \
    $$PWD/abe/forestmanagementengine.cpp \
    $$PWD/abe/patch.cpp \
    $$PWD/abe/patches.cpp \
    $$PWD/abe/scheduler.cpp \
    $$PWD/abe/thinning.cpp \
    $$PWD/abe/output/abestandout.cpp \
    $$PWD/abe/output/abestandremovalout.cpp \
    $$PWD/abe/output/unitout.cpp \
    $$PWD/bite/biteagent.cpp \
    $$PWD/bite/bitebiomass.cpp \
    $$PWD/bite/bitecell.cpp \
    $$PWD/bite/bitecellscript.cpp \
    $$PWD/bite/biteclimate.cpp \
    $$PWD/bite/bitecolonization.cpp \
    $$PWD/bite/bitedispersal.cpp \
    $$PWD/bite/biteengine.cpp \
    $$PWD/bite/biteimpact.cpp \
    $$PWD/bite/biteitem.cpp \
    $$PWD/bite/bitelifecycle.cpp \
    $$PWD/bite/biteoutput.cpp \
    $$PWD/bite/biteoutputitem.cpp \
    $$PWD/bite/bitescript.cpp \
    $$PWD/bite/bitewrapper.cpp

HEADERS += \
    $$PWD/3rdparty/MersenneTwister.h \
    $$PWD/3rdparty/SimpleRNG.h \
    $$PWD/core/climate.h \
    $$PWD/core/dbhdistribution.h \
    $$PWD/core/deadtree.h \
    $$PWD/core/environment.h \
    $$PWD/core/establishment.h \
    $$PWD/core/exception.h \
    $$PWD/core/global.h \
    $$PWD/core/grasscover.h \
    $$PWD/core/grid.h \
    $$PWD/core/layeredgrid.h \
    $$PWD/core/management.h \
    $$PWD/core/microclimate.h \
    $$PWD/core/model.h \
    $$PWD/core/modelcontroller.h \
    $$PWD/core/modelsettings.h \
    $$PWD/core/permafrost.h \
    $$PWD/core/phenology.h \
    $$PWD/core/plugin_interface.h \
    $$PWD/core/production3pg.h \
    $$PWD/core/resourceunit.h \
    $$PWD/core/resourceunitspecies.h \
    $$PWD/core/saplings.h \
    $$PWD/core/seeddispersal.h \
    $$PWD/core/snag.h \
    $$PWD/core/soil.h \
    $$PWD/core/species.h \
    $$PWD/core/speciesresponse.h \
    $$PWD/core/speciesset.h \
    $$PWD/core/stamp.h \
    $$PWD/core/stampcontainer.h \
    $$PWD/core/standloader.h \
    $$PWD/core/standstatistics.h \
    $$PWD/core/svdstate.h \
    $$PWD/core/threadrunner.h \
    $$PWD/core/timeevents.h \
    $$PWD/core/tree.h \
    $$PWD/core/version.h \
    $$PWD/core/watercycle.h \
    $$PWD/tools/climateconverter.h \
    $$PWD/tools/csvfile.h \
    $$PWD/tools/debugtimer.h \
    $$PWD/tools/dem.h \
    $$PWD/tools/expression.h \
    $$PWD/tools/expressionwrapper.h \
    $$PWD/tools/floatingaverage.h \
    $$PWD/tools/geotiff.h \
    $$PWD/tools/gisgrid.h \
    $$PWD/tools/globalsettings.h \
    $$PWD/tools/helper.h \
    $$PWD/tools/mapgrid.h \
    $$PWD/tools/modules.h \
    $$PWD/tools/random.h \
    $$PWD/tools/randomgenerator.h \
    $$PWD/tools/scriptglobal.h \
    $$PWD/tools/scriptgrid.h \
    $$PWD/tools/scriptresourceunit.h \
    $$PWD/tools/scripttree.h \
    $$PWD/tools/settingmetadata.h \
    $$PWD/tools/spatialanalysis.h \
    $$PWD/tools/sqlhelper.h \
    $$PWD/tools/statdata.h \
    $$PWD/tools/ticktack.h \
    $$PWD/tools/xmlhelper.h \
    $$PWD/output/carbonflowout.h \
    $$PWD/output/carbonout.h \
    $$PWD/output/customaggout.h \
    $$PWD/output/deadtreeout.h \
    $$PWD/output/devstageout.h \
    $$PWD/output/dynamicstandout.h \
    $$PWD/output/ecovizout.h \
    $$PWD/output/landscapeout.h \
    $$PWD/output/managementout.h \
    $$PWD/output/output.h \
    $$PWD/output/outputmanager.h \
    $$PWD/output/productionout.h \
    $$PWD/output/saplingout.h \
    $$PWD/output/snapshot.h \
    $$PWD/output/soilinputout.h \
    $$PWD/output/standdeadout.h \
    $$PWD/output/standout.h \
    $$PWD/output/svdindicatorout.h \
    $$PWD/output/svdout.h \
    $$PWD/output/treeout.h \
    $$PWD/output/waterout.h \
    $$PWD/abe/abe_global.h \
    $$PWD/abe/abegrid.h \
    $$PWD/abe/actgeneral.h \
    $$PWD/abe/activity.h \
    $$PWD/abe/actplanting.h \
    $$PWD/abe/actsalvage.h \
    $$PWD/abe/actscheduled.h \
    $$PWD/abe/actthinning.h \
    $$PWD/abe/agent.h \
    $$PWD/abe/agenttype.h \
    $$PWD/abe/fmdeadtreelist.h \
    $$PWD/abe/fmsaplinglist.h \
    $$PWD/abe/fmstand.h \
    $$PWD/abe/fmstp.h \
    $$PWD/abe/fmtreelist.h \
    $$PWD/abe/fmunit.h \
    $$PWD/abe/fomescript.h \
    $$PWD/abe/fomewrapper.h \
    $$PWD/abe/forestmanagementengine.h \
    $$PWD/abe/patch.h \
    $$PWD/abe/patches.h \
    $$PWD/abe/scheduler.h \
    $$PWD/abe/thinning.h \
    $$PWD/abe/output/abestandout.h \
    $$PWD/abe/output/abestandremovalout.h \
    $$PWD/abe/output/unitout.h \
    $$PWD/bite/bite_global.h \
    $$PWD/bite/biteagent.h \
    $$PWD/bite/bitebiomass.h \
    $$PWD/bite/bitecell.h \
    $$PWD/bite/bitecellscript.h \
    $$PWD/bite/biteclimate.h \
    $$PWD/bite/bitecolonization.h \
    $$PWD/bite/bitedispersal.h \
    $$PWD/bite/biteengine.h \
    $$PWD/bite/biteimpact.h \
    $$PWD/bite/biteitem.h \
    $$PWD/bite/bitelifecycle.h \
    $$PWD/bite/biteoutput.h \
    $$PWD/bite/biteoutputitem.h \
    $$PWD/bite/bitescript.h \
    $$PWD/bite/bitewrapper.h

RESOURCES += \
    $$PWD/iland/res/iland.qrc \
    $$PWD/abe-lib/abe-library.qrc
