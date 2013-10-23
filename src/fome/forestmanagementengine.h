#ifndef FORESTMANAGEMENTENGINE_H
#define FORESTMANAGEMENTENGINE_H
class QJSEngine;

/// the FOrestManagementEngine is the container for the agent based forest management engine.
class ForestManagementEngine
{
public:
    ForestManagementEngine();
    static QJSEngine *scriptEngine() const;
};

#endif // FORESTMANAGEMENTENGINE_H
