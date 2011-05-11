#ifndef STANDLOADER_H
#define STANDLOADER_H
#include <QtCore/QString>

class Model;
class ResourceUnit;
class RandomCustomPDF;
class Species;
class MapGrid;

class StandLoader
{
public:
    StandLoader(Model *model): mModel(model), mRandom(0), mCurrentMap(0) {}
    ~StandLoader();
    void setMap(const MapGrid *map) { mCurrentMap = map; }
    void processInit();
    /// load a single tree file (picus or iland style). return number of trees loaded.
    int loadPicusFile(const QString &fileName, ResourceUnit *ru=NULL);
    /// load a tree distribution based on dbh classes. return number of trees loaded.
    int loadiLandFile(const QString &fileName, ResourceUnit *ru=NULL, int stand_id=0);
    int loadSingleTreeList(const QString &content, ResourceUnit*ru = NULL, const QString &fileName="");
    int loadDistributionList(const QString &content, ResourceUnit *ru = NULL, int stand_id=0, const QString &fileName="");
    // load regeneration in stands
    int loadSaplings(const QString &content, int stand_id, const QString &fileName=QString());
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
    int loadInitFile(const QString &fileName, const QString &type, int stand_id=0, ResourceUnit *ru=NULL);
    void executeiLandInit(ResourceUnit *ru); ///< shuffle tree positions
    void executeiLandInitStand(int stand_id); ///< shuffle tree positions
    void copyTrees(); ///< helper function to quickly fill up the landscape by copying trees
    void evaluateDebugTrees(); ///< set debug-flag for trees by evaluating the param-value expression "debug_tree"
    Model *mModel;
    RandomCustomPDF *mRandom;
    QVector<InitFileItem> mInitItems;
    const MapGrid *mCurrentMap;
};

#endif // STANDLOADER_H
