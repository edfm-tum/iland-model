#include "fome_global.h"

#include "knowledgebase.h"

#include "forestmanagementengine.h"
#include "activity.h"


/** @class KnowledgeBase
*/

KnowledgeBase::KnowledgeBase()
{
}

// setup() creates the knowledge base
// it loads all javascript code from the given directory, parses the content (i.e. lets the Javascript engine evaluate)
void KnowledgeBase::setup(const QString &directory)
{
    clear();
    QJSEngine &engine = *ForestManagementEngine::scriptEngine();

    QDir dir(directory, "*.js");
    QStringList files = dir.entryList();

    foreach(QString file, files) {
        qDebug() << "evaluating file" << file;
        readF
        engine.evaluate()
    }
    QJSValueIterator it(prop);
    QJSValue  prop = engine.globalObject();
    it = prop; // restart iterator
    while (it.hasNext()) {
        it.next();
        if (it.value().hasOwnProperty("version")) {
            qDebug() << "activity: "<< it.name() << ": " << it.value().property("name").toString();
        }
    }

}

void KnowledgeBase::clear()
{
    foreach(Activity *act, mActivities) {
        delete act;
    }
    mActivities.clear();
}
