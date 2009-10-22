#ifndef STANDLOADER_H
#define STANDLOADER_H
#include <QtCore/QString>

class Model;
class ResourceUnit;

class StandLoader
{
public:
    StandLoader(Model *model): mModel(model) {}
    void processInit();
    void loadFromPicus(const QString &fileName, ResourceUnit *ru=NULL);
    void loadiLandFile(const QString &fileName, ResourceUnit *ru=NULL);

private:
    void loadInitFile(const QString &fileName, const QString &type, ResourceUnit *ru=NULL);
    void loadForUnit();
    void copyTrees();
    void evaluateDebugTrees();
    Model *mModel;
};

#endif // STANDLOADER_H
