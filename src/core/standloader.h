#ifndef STANDLOADER_H
#define STANDLOADER_H
#include <QtCore/QString>

class Model;
class ResourceUnit;
class RandomCustomPDF;
class Species;
class StandLoader
{
public:
    StandLoader(Model *model): mModel(model), mRandom(0) {}
    ~StandLoader();
    void processInit();
    void loadFromPicus(const QString &fileName, ResourceUnit *ru=NULL);
    void loadiLandFile(const QString &fileName, ResourceUnit *ru=NULL);

private:
    struct InitFileItem
    {
        Species *species;
        int count;
        double dbh_from, dbh_to;
        double hd;
        int age;
    };
    void loadInitFile(const QString &fileName, const QString &type, ResourceUnit *ru=NULL);
    void executeiLandInit(ResourceUnit *ru);
    void loadForUnit();
    void copyTrees();
    void evaluateDebugTrees();
    Model *mModel;
    RandomCustomPDF *mRandom;
     QVector<InitFileItem> mInitItems;
};

#endif // STANDLOADER_H
