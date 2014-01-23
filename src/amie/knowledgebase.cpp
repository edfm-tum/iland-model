#include "fome_global.h"

#include "knowledgebase.h"

#include "forestmanagementengine.h"
#include "activity.h"
#include "fmstand.h"
#include "fmunit.h"

#include "helper.h"


/** @class KnowledgeBase
*/

KnowledgeBase::KnowledgeBase()
{
}

KnowledgeBase::~KnowledgeBase()
{
    qDeleteAll(mActivities); // deletes the stands (not the keys)
    mActivities.clear();

}

// setup() creates the knowledge base
// it loads all javascript code from the given directory, parses the content (i.e. lets the Javascript engine evaluate)
// returns true if the setup was successful without errors.
bool KnowledgeBase::setup(const QString &directory)
{
    clear();
    QJSEngine &engine = *ForestManagementEngine::scriptEngine();

    // (1) load all javascript files from the target directory
    QDir dir(directory, "*.js");
    QStringList files = dir.entryList();

    bool success = true;
    foreach(QString file, files) {
        qDebug() << "evaluating file" << file << "...";
        QString code = Helper::loadTextFile(dir.filePath(file));
        QJSValue result = engine.evaluate(code, file);
        if (result.isError()) {
            qDebug() << "Javascript Error in file" << result.property("fileName").toString() << ":" << result.toString();
            success = false;
        }
    }

    // (2) scan the global Javascript scope for activities
    QJSValueIterator it(engine.globalObject());
    QJSValue  prop = engine.globalObject();
    it = prop; // restart iterator
    int n_activities = 0;
    while (it.hasNext()) {
        it.next();
        if (ActivityOld::verbose()) qDebug() << "checking element" << it.name() << "...";
        if (it.value().hasOwnProperty("version")) {
            qDebug() << "found activity: "<< it.name() << ": " << it.value().property("name").toString();
            n_activities++;
            // create a new activity
            ActivityOld *act = new ActivityOld();
            if (act->setupFromJavascript(it.value(), it.name())) {
                // setup ok
                mActivities.push_back(act);
            } else {
                success = false;
            }
        }
    }
    qDebug() << "KnowledgeBase summary: loaded " << files.count() << "files. Found" << n_activities << "activities and" << mActivities.count() << "were sucessfully setup.";

    return success;

}

/// evaluate()
bool KnowledgeBase::evaluate(const FMStand *stand)
{
    double best=-1;
    const ActivityOld *best_activity = NULL;

    foreach(const ActivityOld *act, mActivities) {
        double this_activity = act->evaluate(stand);
        if (this_activity > best) {
            best_activity = act;
            best = this_activity;
        }
    }
    // check total threshold
    if (best > 0.5) {
        // create ticket ...
        qDebug() << "create ticket....";
        return true;
    }
    return false;
}

void KnowledgeBase::clear()
{
    foreach(ActivityOld *act, mActivities) {
        delete act;
    }
    mActivities.clear();
}
