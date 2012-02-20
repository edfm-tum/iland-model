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
    /// load a single tree file (picus or iland style). return number of trees loaded.
    int loadPicusFile(const QString &fileName, ResourceUnit *ru=NULL);
    /// load a tree distribution based on dbh classes. return number of trees loaded.
    int loadiLandFile(const QString &fileName, ResourceUnit *ru=NULL);
    int loadSingleTreeList(const QString &content, ResourceUnit*ru = NULL, const QString &fileName="");
    int loadDistributionList(const QString &content, ResourceUnit *ru = NULL, const QString &fileName="");
private:
    struct InitFileItem
    {
        Species *species;
        int count;
        double dbh_from, dbh_to;
        double hd;
        int age;
        double density;
    };
    /// load tree initialization from a file. return number of trees loaded.
    int loadInitFile(const QString &fileName, const QString &type, ResourceUnit *ru=NULL);
    void executeiLandInit(ResourceUnit *ru); ///< shuffle tree positions
    void copyTrees(); ///< helper function to quickly fill up the landscape by copying trees
    void evaluateDebugTrees(); ///< set debug-flag for trees by evaluating the param-value expression "debug_tree"
    Model *mModel;
    RandomCustomPDF *mRandom;
     QVector<InitFileItem> mInitItems;
};

#endif // STANDLOADER_H
