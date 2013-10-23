#ifndef FORESTMANAGEMENTENGINE_H
#define FORESTMANAGEMENTENGINE_H

#include "knowledgebase.h"

class QJSEngine;

/// the FOrestManagementEngine is the container for the agent based forest management engine.
class ForestManagementEngine
{
public:
    // life cycle
    ForestManagementEngine();
    void setup();
    // properties

    // functions
    void test();

    //
    static QJSEngine *scriptEngine();
private:
    KnowledgeBase mKnowledgeBase;
};

#endif // FORESTMANAGEMENTENGINE_H
