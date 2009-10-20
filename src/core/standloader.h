#ifndef STANDLOADER_H
#define STANDLOADER_H
#include <QtCore>

class Model;
class ResourceUnit;

class StandLoader
{
public:
    StandLoader(Model *model): mModel(model) {}
    void processInit();
    void loadFromPicus(const QString &fileName, QPointF offset=QPointF(), ResourceUnit *ru=NULL);

private:
    void loadInitFile(const QString &fileName, const QString &type, QPointF offset=QPointF(), ResourceUnit *ru=NULL);
    void loadForUnit();
    void copyTrees();
    void evaluateDebugTrees();
    Model *mModel;
};

#endif // STANDLOADER_H
