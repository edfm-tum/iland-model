#include "fome_global.h"
#include "globalsettings.h"


#include "forestmanagementengine.h"

/** @class ForestManagementEngine
*/
ForestManagementEngine::ForestManagementEngine()
{
}

QJSEngine *ForestManagementEngine::scriptEngine() const
{
    // use global engine from iLand
    return GlobalSettings::instance()->scriptEngine();
}
