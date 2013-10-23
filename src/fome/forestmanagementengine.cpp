#include "fome_global.h"
#include "globalsettings.h"


#include "forestmanagementengine.h"
#include "activity.h"

/** @class ForestManagementEngine
*/
ForestManagementEngine::ForestManagementEngine()
{
}

void ForestManagementEngine::test()
{
    // test code
    try {
        Activity::setVerbose(true);
        mKnowledgeBase.setup("E:/Daten/iLand/modeling/abm/knowledge_base/test");
    } catch (const IException &e) {
        qDebug() << "An error occured:" << e.message();
    }
}

QJSEngine *ForestManagementEngine::scriptEngine()
{
    // use global engine from iLand
    return GlobalSettings::instance()->scriptEngine();
}
